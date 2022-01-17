#ifndef ANALOG_OUTPUT2_H_
#define ANALOG_OUTPUT2_H_

#include <objcpp/property.h>

typedef void* TaskHandle;

class AnalogOutput2
{
public:
	AnalogOutput2();
	~AnalogOutput2();

	void accept(property_visitor &visit);

	property<std::string> ScanPatternFile;
	property<int> nAlines;
	operation Start, Stop;

	void callback();
	int bufferSize() { return nAlines * 8; }

private:
	TaskHandle _taskHandle;
	std::unique_ptr<double[]> _buffer;

	void start();
	void stop();
};

#endif // ANALOG_OUTPUT2_H_