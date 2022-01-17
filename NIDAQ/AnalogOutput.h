#ifndef ANALOG_OUTPUT_H_
#define ANALOG_OUTPUT_H_

#include <objcpp/property.h>

typedef void *TaskHandle;

class AnalogOutput
{
public:
	AnalogOutput();
	~AnalogOutput();

	void accept(property_visitor &visit);

    property<int> nChannels;
	property<int> nAlines;
	property<int> nFrames1;
	property<std::string> ScanPatternFile1;
	property<int> nFrames2;
	property<std::string> ScanPatternFile2;

	property<float> Amp; // FIXME: Use property<double>
	property<std::string> ClockPort;
	property<int> ClockFrequency;

	operation Start, StartScan, ResetZero, Stop;
	operation UseScanPattern1, UseScanPattern2;

private:
	TaskHandle _taskHandle;

	void start1() { start(true); }
	void start2() { start(false); }
	void start(bool useScanPattern1);
    void resetZero();
	void stop();
	void useScanPattern1();
	void useScanPattern2();

	void updateScanPattern(const std::string &filename, int nFrames);
};

#endif // ANALOG_OUTPUT_H_