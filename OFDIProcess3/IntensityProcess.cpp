#include "stdafx.h"
#include "IntensityProcess.h"

#include <ipps.h>
#include <ippac.h>

using namespace std;

IntensityProcess::IntensityProcess(const OFDI &g) :
	g(g), 
	ofdi_process(g), 
	
	result(g.nScans2n/2, g.nAlines), 
	debug_fringe(2 * g.nScans), 
	debug_result(2 * g.nScans2n/2), 
	debugAline(0), 
	
	bgX(g.nScans, 4), 
	bgY(g.nScans, 4), 
	temp(g.nScans, 4), 
	fringeX(g.nScans, g.nAlines), 
	fringeY(g.nScans, g.nAlines), 
	phaseX(g.nScans2n/2), 
	phaseY(g.nScans2n/2), 
	intensityX(g.nScans2n/2), 
	intensityY(g.nScans2n/2)
{
}

// FIXME: slice for const Array&
namespace np 
{
	template <typename T>
	inline Array<T, 1> slice(const Array<T, 2> &array, const WholeRange &range0, int index1)
	{
		Array<T, 1> result;

		result._length = array.size(0);
		result._size = make_array(array.size(0));
		result._stride = make_array(array.stride(0));
		result._address = array._address; // add reference count here
		result._origin = const_cast<T *>(&array.at(0, index1));

		return result;
	}
}

void IntensityProcess::operator() (const np::Array<uint16_t, 2> &fringes_interlaced)
{
	const int nScans = g.nScans, nAlines = g.nAlines, nScans2n = g.nScans2n;

    if (fringes_interlaced.size(0) == 2 * nScans)
    {
        // Two channel, deinterlace fringe
        Ipp16u *deinterlaced_fringe[2] = { fringeX, fringeY };
        ippsDeinterleave_16s((Ipp16s *)fringes_interlaced.raw_ptr(), 2, nAlines * nScans, (Ipp16s **)deinterlaced_fringe);
    }
    else
    {
        // One channel, copy channel to both x and y
        // TODO: process only one channel
        memcpy(fringeX, fringes_interlaced, np::byteSize(fringeX));
        memcpy(fringeY, fringes_interlaced, np::byteSize(fringeY));
    }

	// Extract bg signal from fringe
	ippsZero_32f(bgX, bgX.length());
	for (int i = 0; i < nAlines; i += 4)
	{
		ippsConvert_16u32f(fringeX + (nScans * i), temp, temp.length());
		ippsAdd_32f(temp, bgX, bgX, bgX.length());
	}
	ippsMulC_32f(bgX, 1.0f / (float)(nAlines/4), bgX, bgX.length());

	ippsZero_32f(bgY, bgY.length());
	for (int i = 0; i < nAlines; i += 4)
	{
		ippsConvert_16u32f(fringeY + (nScans * i), temp, temp.length());
		ippsAdd_32f(temp, bgY, bgY, bgY.length());
	}
	ippsMulC_32f(bgY, 1.0f / (float)(nAlines/4), bgY, bgY.length());

	// for each a-lines
	for (int aline = 0; aline < nAlines; aline++)
	{	
		// Process X polarization
		ofdi_process(
			phaseX, 
			np::slice(fringeX, np::_colon(), aline), 
			np::slice(bgX, np::_colon(), aline % 4));

		for (int i = 0; i < phaseX.length(); i++)
			intensityX(i) = phaseX(i).real() * phaseX(i).real() + phaseX(i).imag() * phaseX(i).imag();

		// Process Y polarization
		ofdi_process(
			phaseY, 
			np::slice(fringeY, np::_colon(), aline), 
			np::slice(bgY, np::_colon(), aline % 4));

		for (int i = 0; i < phaseY.length(); i++)
			intensityY(i) = phaseY(i).real() * phaseY(i).real() + phaseY(i).imag() * phaseY(i).imag();

		if (aline == debugAline)
		{
			// Calculate data for input buffer in scope control
			for (int i = 0; i < nScans; i++)
			{
				debug_fringe(i) = (uint16_t)(32767 + fringeX(i, aline) - bgX(i));
				debug_fringe(i + nScans) = (uint16_t)(32767 + fringeY(i, aline) - bgY(i));
			}

			// Calculate data for output buffer in scope control
			np::Array<float> temp(nScans2n/2);

			// 0 ~ 1023 : output debug data of X channel
			ippsLn_32f(intensityX, temp, nScans2n/2);
			ippsMulC_32f(temp, 4.3429f, temp, nScans2n/2);
			ippsSubC_32f(temp, g.windowLevel - (g.windowWidth / 2), temp, nScans2n/2);
			ippsMulC_32f(temp, 65535 / g.windowWidth, temp, nScans2n/2);
			ippsConvert_32f16u_Sfs(temp, debug_result, nScans2n/2, ippRndNear, 0);

			// 1024 ~ 2047 : output debug data of Y channel
			ippsLn_32f(intensityY, temp, nScans2n/2);
			ippsMulC_32f(temp, 4.3429f, temp, nScans2n/2);
			ippsSubC_32f(temp, g.windowLevel - (g.windowWidth / 2), temp, nScans2n/2);
			ippsMulC_32f(temp, 65535 / g.windowWidth, temp, nScans2n/2);
			ippsConvert_32f16u_Sfs(temp, debug_result + nScans2n/2, nScans2n/2, ippRndNear, 0);
		}

		for (int i = 0; i < intensityX.length(); i++)
			intensityX.at(i) = ((4.3429f * log(intensityX(i) + intensityY(i))) 
					- (g.windowLevel - (g.windowWidth / 2)))
					* (255 / g.windowWidth);

		ippsConvert_32f8u_Sfs(intensityX, np::slice(result, np::_colon(), aline), nScans2n/2, ippRndNear, 0);

	} // for each a-lines
}

// ## OFDIProcess

OFDIProcess::OFDIProcess(const OFDI &g) :
	g(g), 
	fringes(g.nScans2n), 
	depth(g.nScans2n), 
	depth_zpad(g.dopplerProcess ? g.nScans2n/2 : g.nScans2n), 
	fringes_c(g.dopplerProcess ? g.nScans2n/2 : g.nScans2n), 
	fringes_interp(g.nScans2n/2)
{
}

template <typename X, typename Y>
struct linear_interp
{
	const X x0, x1;
	const Y y0, y1;

	linear_interp(X x0, X x1, Y y0, Y y1) : x0(x0), x1(x1), y0(y0), y1(y1) { }

	Y operator() (X x)
	{
		// Linear interpolation
		// http://en.wikipedia.org/wiki/Linear_interpolation

		const float weight0 = x - x0;
		const float weight1 = x1 - x;

		return y0 * weight0 + y1 * weight1;
	}
};

template <typename T>
void remap(np::Array<T> &dst, const np::Array<T> &src, const np::Array<float> &index_map)
{
	std::transform(index_map.raw_ptr(), index_map.raw_ptr() + index_map.length(), dst.raw_ptr(), 
		[&src](float index)
		{
			const float	x = index;
			const int	x0 = (int)x;
			const int	x1 = x0 + 1;

			auto interpolation = linear_interp<float, T>((float)x0, (float)x1, src[x0], src[x1]);
			return interpolation(x);
		});
}

void OFDIProcess::operator()(
	np::Array<std::complex<float>> &phase, 
	const np::Array<uint16_t> &signal, 
	const np::Array<float> &bg_signal)
{
	const int nScans = g.nScans, nAlines = g.nAlines, nScans2n = g.nScans2n;

	// 1. Background subtract & apply window function
	for (int i = 0; i < signal.length(); i++)
		fringes(i) = ((float)signal(i) - bg_signal(i)) * g.windowFunction(i);

	// 2. FFT for zero padding
	fft1(depth, fringes);

	// 3. Zero pad
	for (int i = 0; i < depth_zpad.length()/2; i++)
		depth_zpad(i) = depth(i);

	for (int i = depth_zpad.length()/2; i < depth_zpad.length(); i++)
		depth_zpad(i) = 0;

	// Circular shift n/4 (only when using frequency shifter)
	std::rotate(depth_zpad.raw_ptr(), 
		depth_zpad.raw_ptr() + depth_zpad.length()/4, 
		depth_zpad.raw_ptr() + depth_zpad.length());

	// 4. Inverse FFT
	fft2.inverse(fringes_c, depth_zpad);

	// 5. Interpolation
	for (int i = 0; i < fringes_interp.length(); i++)
		fringes_interp(i) = 0;
	remap(fringes_interp, fringes_c, g.newIndex);

	// 6. Numerical dispersion compensation
	for (int i = 0; i < g.dispersion.length(); i++)
		fringes_interp(i) *= g.dispersion(i);

	// 7. FFT for imaging
	fft3.forward(phase, fringes_interp);

	// Circular shift n/2 (only when using frequency shifter)
	std::rotate(phase.raw_ptr(), 
		phase.raw_ptr() + phase.length()/2, 
		phase.raw_ptr() + phase.length());
}