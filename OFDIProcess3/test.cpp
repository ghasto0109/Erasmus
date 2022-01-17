#include "OFDI.h"
#include "IntensityProcess.h"

#include <numcpp/array.h>
#include <numcpp/opencv.h>

#include <iostream>
#include <fstream>

using namespace std;

const float WindowLevel = 160.f;
const float WindowWidth = 60.f;

void test(
	const int nScans, 
	const int nAlines, 
	const string CalibrationFile, 
	const string DataFile, 
	const string ResultImage)
{
	OFDI ofdi(nScans, nAlines);
	ofdi.setCalibrationFilePath(CalibrationFile);
	ofdi.windowLevel = WindowLevel;
	ofdi.windowWidth = WindowWidth;

	np::Array<uint16_t, 2> fringe(2 * nScans, nAlines);
	
	fstream file(DataFile, ios::in | ios::binary);
	if (!file.is_open())
	{
		cout << "ERROR: Cannot open data file: " << DataFile << endl;
		return;
	}

	file.read((char *)fringe.raw_ptr(), 2 * nScans * nAlines * sizeof(uint16_t));
	file.close();

	IntensityProcess process(ofdi);
	process(fringe);

	np::imwrite(process.result, ResultImage);

	string command = string("start ") + ResultImage;
	system(command.c_str());
}

int main()
{
	cout << "Hello OFDI Test!" << endl;

	// testcase #1
	{
		const int nScans = 1190;
		const int nAlines = 1024;

		const string CalibrationFile = "testcase_ofdi/test_1frame.calibration";
		const string DataFile = "testcase_ofdi/test_1frame.data";
		const string ResultImage = "testcase_ofdi/0000.bmp";

		test(nScans, nAlines, CalibrationFile, DataFile, ResultImage);
	}

	// testcase #2
	{
		const int nScans = 1300;
		const int nAlines = 1024;
		
		const string CalibrationFile = "testcase_doppler/calibration.dat";
		const string DataFile = "testcase_doppler/d1.bin";
		const string ResultImage = "testcase_doppler/0000.bmp";

		test(nScans, nAlines, CalibrationFile, DataFile, ResultImage);
	}

	return 0;
}