#ifndef CAMERA_CAPTURE_H_
#define CAMERA_CAPTURE_H_

#include <objcpp/property.h>
#include <numcpp/array.h>

#include <thread>

class QPictureBox;

class CameraCapture
{
public:
	CameraCapture();
	~CameraCapture();

	void accept(property_visitor &visit);

	// properties
	property<int> CameraID;

	// operations
	operation Start, Stop;

	// signals
	signal<const np::Array<uint8_t, 2> &> DidCaptureFrame;

	QPictureBox *widget;

private:
	void start();
	void stop();

	std::thread _thread;
	bool _running;
	void run();
};

#endif // CAMERA_CAPTURE_H_