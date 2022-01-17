#include "stdafx.h"
#include "Calibration.h"

#include <fstream>
#include <filesystem>

using namespace std;

Calibration::Calibration() : 
	CaptureBackground("Capture Background", this, &Calibration::captureBackground), 
	CaptureD1("Capture D1", this, &Calibration::captureD1), 
	CaptureD2("Capture D2", this, &Calibration::captureD2), 
	GenerateCalibration("Generate Calibration", this, &Calibration::generateCalibration), 

	_waitingForFrameCapture(false)
{
}

void Calibration::accept(property_visitor &visit)
{
	visit(CaptureBackground);
	visit(CaptureD1);
	visit(CaptureD2);
	visit(GenerateCalibration);
}

void Calibration::captureBackground()
{
	_captureFileName = "bg.bin";
	_waitingForFrameCapture = true;
}

void Calibration::captureD1()
{
	_captureFileName = "d1.bin";
	_waitingForFrameCapture = true;
}

void Calibration::captureD2()
{
	_captureFileName = "d2.bin";
	_waitingForFrameCapture = true;
}

#include <Windows.h>

void Calibration::generateCalibration()
{
	STARTUPINFO si = { 0, };
	si.dwFlags = STARTF_USESHOWWINDOW;
	// si.wShowWindow = SW_HIDE;

	PROCESS_INFORMATION pi = { 0, };

	using namespace std::tr2::sys;

	std::string calibPath = current_path<path>() / path("Calib.m");
	printf("Calibration MATLAB File: %s\n", calibPath.c_str());

	std::string command = "matlab -automation -noFigureWindows -r run('" + calibPath + "'),quit";

	BOOL succeeded = CreateProcess(NULL,
		const_cast<char *>(command.c_str()),
		NULL, NULL, FALSE,
		NORMAL_PRIORITY_CLASS,
		NULL, NULL, &si, &pi);

	if (succeeded) 
	{
		// Wait until process is finished
		WaitForSingleObject(pi.hProcess, INFINITE);
	}
	else
	{
		cout << "Error: cannot run the matlab command: " << command << endl;
		return;
	}
}

// signal from SignatecDAQ
void Calibration::didAcquireData(int tag, const np::Array<uint16_t, 2> &acquiredData)
{
	// Save frame if waiting for capture
	if (_waitingForFrameCapture)
	{
		_waitingForFrameCapture = false; 

		std::fstream file(_captureFileName, ios::out | ios::binary);
		if (!file.is_open())
		{
			cout << "Cannot open file to save captured frame: " << _captureFileName << endl;
			return;
		}

		// FIXME: Use np::Array<uint16_t, 2> acquiredData
		file.write((char *)acquiredData.raw_ptr(), np::byteSize(acquiredData));

		file.close();

		cout << "Capture frame: " << _captureFileName << endl;
	}
}