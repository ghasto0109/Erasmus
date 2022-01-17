#ifndef OFDI_H_
#define OFDI_H_

#include <numcpp/array.h>
#include <complex>

class OFDI
{
public:
	OFDI(int nScans, int nAlines);

	// Constant sizes
	const int nScans, nScans2n, nAlines;

	// Processing parameters - for process
	np::Array<float> windowFunction;
	void setCalibrationFilePath(const std::string &calibration_file_path);
	bool dopplerProcess;
	np::Array<float> newIndex; // indexMap + weightMap
	np::Array<std::complex<float>> dispersion;
	int DC;
	float windowLevel, windowWidth;
};

#endif // OFDI_H_