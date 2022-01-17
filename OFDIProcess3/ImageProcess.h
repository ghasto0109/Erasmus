#ifndef IMAGE_PROCESS_H_
#define IMAGE_PROCESS_H_

#include <objcpp/property.h>
#include <numcpp/array.h>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

class ImageProcess
{
public:
	ImageProcess();
	~ImageProcess();

	void accept(property_visitor &visit);

	// ...
	void initialize();
	bool isProcessingReady();
	//void startImageProcessing(const uint16_t *fringeData); 
	void startImageProcessing(const uint8_t *fringeData);

	// properties
	property<int> nChannels, nScans, nAlines;
	property<std::string> CalibrationFile;
	property<int> DC;
	property<float> WindowLevel, WindowWidth;
	property<bool> Transpose;
	property<int> DiagnosisAline;
	property<int> ProcessID;
	property<int> FrameRate;

	// operations
	operation Restart;

	// slots for SignatecDAQ
	//void didAcquireData(int tag, const np::Array<uint16_t, 2> &acquiredData);
	void didAcquireData(int tag, const np::Array<uint8_t, 2> &acquiredData);

	// signals
	signal<ImageProcess *> DidFinishImageProcessing;

	// result
	np::Array<uint8_t, 2> resultImage;
	np::Array<uint16_t> debugFringe, debugImage;

private:
	//np::Array<uint16_t, 2> _fringeBuffer;
	np::Array<uint8_t, 2> _fringeBuffer;

	std::thread _thread;
	std::mutex _mutex;
	std::condition_variable _cond;
	std::chrono::time_point<std::chrono::system_clock> _lastUpdate;

	bool _running;
	void run();
};

#endif // IMAGE_PROCESS_H_