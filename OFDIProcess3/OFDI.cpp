#include "stdafx.h"
#include "OFDI.h"

#include <fstream>
#include <ipps.h>
#include <ippac.h>

using namespace std;

// Return least k, such as x <= k, k = 2^n
inline int getLeastPower2Over(int x)
{
	int k = 1;

	while (!(x <= k))
		k = k << 1;

	return k;
}

OFDI::OFDI(int nScans, int nAlines) :
	nScans(nScans), 
	nScans2n(getLeastPower2Over(nScans)), 
	nAlines(nAlines), 

	dopplerProcess(false), 
	windowFunction(nScans2n), 
	newIndex(nScans/2), 
	dispersion(nScans/2), 
	DC(0), 
	windowLevel(165.0f), 
	windowWidth(70.0f)
{
	// Set hanning window function
	ippsSet_32f(1.0f, windowFunction, nScans2n);
	ippsWinHann_32f(windowFunction, windowFunction, nScans);
	ippsZero_32f(windowFunction + nScans, nScans2n - nScans);

	// Initialize calibration data
	for (int i = 0; i < newIndex.length(); i++)
		newIndex(i) = (float)(i * 2);

	for (int i = 0; i < dispersion.length(); i++)
		dispersion(i) = 1;
}

void OFDI::setCalibrationFilePath(const std::string &calibration_file_path)
{
	// open calibration file
	fstream file(calibration_file_path, fstream::in | fstream::binary);
	if (!file.is_open())
	{
		cout << "Failed to open calibration file: " << calibration_file_path << endl;
		return;
	}

	// check calibration file size
	{
		streampos fsize = 0;
		fsize = file.tellg();
		file.seekg(0, ios::end);
		fsize = file.tellg() - fsize;
		file.seekg(0, ios::beg);

		// nScans/2 for oph. system, nScans/4 for doppler system
		if ((int)fsize == nScans/2 * 16)
		{
			dopplerProcess = false;

			newIndex = np::Array<float>(nScans/2);
			dispersion = np::Array<complex<float>>(nScans/2);
		}
		else if ((int)fsize == nScans/4 * 16)
		{
			dopplerProcess = true;

			newIndex = np::Array<float>(nScans/4);
			dispersion = np::Array<complex<float>>(nScans/4);

		}
		else
		{
			cout << "ERROR: Calibration file has wrong size: " << fsize << endl;
			return;
		}
	}

	np::Array<int> index(newIndex.length());
	np::Array<float> weight(newIndex.length());
	np::Array<float> temp_dispersion(2 * dispersion.length());

	int fsize = 0;
	file.read((char *)index.raw_ptr(), byteSize(index));
	file.read((char *)weight.raw_ptr(), byteSize(weight)); 
	file.read((char *)temp_dispersion.raw_ptr(), byteSize(dispersion));
	file.close();

	// k index calibration
	for (int i = 0; i < newIndex.length(); i++)
		newIndex(i) = index(i) + weight(i);

	// dispersion compensation
	ippsRealToCplx_32f(temp_dispersion, temp_dispersion + dispersion.length(), (Ipp32fc *)dispersion.raw_ptr(), dispersion.length());

	// adjust dispersion value according to DC value
	for (int i = 0; i < dispersion.length(); i++)
	{
		float A = DC * pow((float)(((float)i - dispersion.length()/2) / dispersion.length()), 2);
		float exp_iA_real = cos(A), exp_iA_imag = sin(A);

		// dispersion *= exp(iA)
		float a = dispersion[i].real(), b = dispersion[i].imag();
		float c = exp_iA_real, d = exp_iA_imag;

		dispersion[i] = complex<float>(a*c - b*d, b*c + a*d);
	}
}