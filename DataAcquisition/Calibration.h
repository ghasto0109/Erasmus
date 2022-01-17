#ifndef CALIBRATION_H_
#define CALIBRATION_H_

#include <objcpp/property.h>
#include <numcpp/array.h>

class Calibration
{
public:
	Calibration();

	void accept(property_visitor &visit);

	// operations
	operation CaptureBackground;
	operation CaptureD1;
	operation CaptureD2;
	operation GenerateCalibration;

	// SignatecDAQ signals
	void didAcquireData(int tag, const np::Array<uint16_t, 2> &acquiredData);

private:
	bool _waitingForFrameCapture;
	std::string _captureFileName;

	void captureBackground();
	void captureD1();
	void captureD2();
	void generateCalibration();
};

#endif // CALIBRATION_H_