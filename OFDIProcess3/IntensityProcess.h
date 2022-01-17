#ifndef INTENSITY_PROCESS_H_
#define INTENSITY_PROCESS_H_

#include "OFDI.h"

#include <numcpp/array.h>
#include <ipps.h>

struct FFT_R2C
{
	IppsFFTSpec_R_32f *pFFTSpec;
	Ipp8u *pMemSpec, *pMemInit, *pMemBuffer;

	np::Array<float> temp;

	FFT_R2C() : pFFTSpec(nullptr), pMemSpec(nullptr), pMemInit(nullptr), pMemBuffer(nullptr)
	{
	}

	~FFT_R2C()
	{
		if (pMemSpec) { ippsFree(pMemSpec); pMemSpec = nullptr; }
		if (pMemInit) { ippsFree(pMemInit); pMemInit = nullptr; }
		if (pMemBuffer) { ippsFree(pMemBuffer); pMemBuffer = nullptr; }
	}

	void operator() (np::Array<std::complex<float>> &dst, const np::Array<float> &src)
	{
		if (temp.size() != src.size())
		{
			temp = np::Array<float>(src.length());

			// init FFT spec
            const int ORDER = (int)(ceil(log2(src.size(0))));

			int sizeSpec, sizeInit, sizeBuffer;
			ippsFFTGetSize_R_32f(ORDER, IPP_FFT_DIV_INV_BY_N, ippAlgHintNone, &sizeSpec, &sizeInit, &sizeBuffer);

			pMemSpec = ippsMalloc_8u(sizeSpec);

			if (sizeInit > 0) 
			   pMemInit = ippsMalloc_8u(sizeInit);
   
			pMemBuffer = ippsMalloc_8u(sizeBuffer);

			ippsFFTInit_R_32f (&pFFTSpec, ORDER, IPP_FFT_DIV_INV_BY_N, ippAlgHintNone, pMemSpec, pMemInit);
		}

		ippsFFTFwd_RToPerm_32f(src, temp, pFFTSpec, NULL);
		ippsConjPerm_32fc(temp, (Ipp32fc *)dst.raw_ptr(), temp.length());
	}

	void operator() (np::Array<std::complex<float>, 2> &dst, const np::Array<float, 2> &src)
	{
		if (temp.size(0) != src.size(0))
		{
			// init FFT spec
            const int ORDER = (int)(ceil(log2(src.size(0))));
			temp = np::Array<float>(1 << ORDER);

			int sizeSpec, sizeInit, sizeBuffer;
			ippsFFTGetSize_R_32f(ORDER, IPP_FFT_DIV_INV_BY_N, ippAlgHintNone, &sizeSpec, &sizeInit, &sizeBuffer);

			pMemSpec = ippsMalloc_8u(sizeSpec);

			if (sizeInit > 0) 
			   pMemInit = ippsMalloc_8u(sizeInit);
   
			pMemBuffer = ippsMalloc_8u(sizeBuffer);

			ippsFFTInit_R_32f (&pFFTSpec, ORDER, IPP_FFT_DIV_INV_BY_N, ippAlgHintNone, pMemSpec, pMemInit);
		}

		for (int i = 0; i < src.size(1); i++)
		{
			ippsFFTFwd_RToPerm_32f(&src(0, i), temp, pFFTSpec, NULL);
			ippsConjPerm_32fc(temp, (Ipp32fc *)&dst(0, i), temp.length());
		}
	}
};

struct FFT_C2C
{
	IppsFFTSpec_C_32fc *pFFTSpec;
	Ipp8u *pMemSpec, *pMemInit, *pMemBuffer;

	int length;

	FFT_C2C() : pFFTSpec(nullptr), pMemSpec(nullptr), pMemInit(nullptr), pMemBuffer(nullptr), length(-1)
	{
	}

	~FFT_C2C()
	{
		if (pMemSpec) { ippsFree(pMemSpec); pMemSpec = nullptr; }
		if (pMemInit) { ippsFree(pMemInit); pMemInit = nullptr; }
		if (pMemBuffer) { ippsFree(pMemBuffer); pMemBuffer = nullptr; }
	}

	void forward(np::Array<std::complex<float>> &dst, const np::Array<std::complex<float>> &src)
	{
		if (length != src.length()) initialize(src.length());

		ippsFFTFwd_CToC_32fc((Ipp32fc *)src.raw_ptr(), (Ipp32fc *)dst.raw_ptr(), pFFTSpec, NULL);
	}

	void inverse(np::Array<std::complex<float>> &dst, const np::Array<std::complex<float>> &src)
	{
		if (length != src.length()) initialize(src.length());

		ippsFFTInv_CToC_32fc((Ipp32fc *)src.raw_ptr(), (Ipp32fc *)dst.raw_ptr(), pFFTSpec, NULL);
	}

private:
	void initialize(int length)
	{
		// update initialized length
		this->length = length;

		// init FFT spec
        const int ORDER = (int)(ceil(log2(length)));

		int sizeSpec, sizeInit, sizeBuffer;
		ippsFFTGetSize_C_32fc(ORDER, IPP_FFT_NODIV_BY_ANY, ippAlgHintNone, &sizeSpec, &sizeInit, &sizeBuffer);

		pMemSpec = ippsMalloc_8u(sizeSpec);

		if (sizeInit > 0) 
		   pMemInit = ippsMalloc_8u(sizeInit);
   
		pMemBuffer = ippsMalloc_8u(sizeBuffer);

		ippsFFTInit_C_32fc(&pFFTSpec, ORDER, IPP_FFT_NODIV_BY_ANY, ippAlgHintNone, pMemSpec, pMemInit);
	}
};

struct OFDIProcess
{
	OFDIProcess(const OFDI &ofdi);
	void operator() (np::Array<std::complex<float>> &phase, const np::Array<uint16_t> &signal, const np::Array<float> &bg_signal);

private:
	const OFDI &g;

	// IPP FFT spec
	FFT_R2C fft1;
	FFT_C2C fft2, fft3;

	// Temp buffers
	np::Array<float> fringes;
	np::Array<std::complex<float>> depth, depth_zpad, fringes_c, fringes_interp;
};

class IntensityProcess
{
public:
	IntensityProcess(const OFDI &ofdi);

	np::Array<uint8_t, 2> result;
	np::Array<uint16_t> debug_fringe, debug_result;

	// Function object
	void operator() (const np::Array<uint16_t, 2> &fringes);

	// Properties
	void setDebugAline(int debugAline) { this->debugAline = debugAline; }

private:
	const OFDI &g;
	OFDIProcess ofdi_process;

	// Diagnosis aline & buffers
	int debugAline;

	// Temp buffers
	np::Array<float, 2> bgX, bgY, temp;
	np::Array<uint16_t, 2> fringeX, fringeY;
	np::Array<std::complex<float>> phaseX, phaseY;
	np::Array<float> intensityX, intensityY;
};

#endif // INTENSITY_PROCESS_H_