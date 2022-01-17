#ifndef ALAZAR_PROCESS_H_
#define ALAZAR_PROCESS_H_

#include <objcpp/property.h>
#include <numcpp/array.h>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

class AlazarProcess
{
public:
	AlazarProcess();
	~AlazarProcess();

	void accept(property_visitor &visit);

	// ...
	void initialize();
	bool isProcessingReady();
	//void startImageProcessing(const uint16_t *fringeData); // async func, notification will be send to delegate. 
	void startImageProcessing(const uint8_t *fringeData); // async func, notification will be send to delegate. 

	// properties
	property<int> nChannels, nScans, nAlines;
	property<float> WindowLevel, WindowWidth;

	// slots for AlazarDAQ
	//void didAcquireData(int tag, const np::Array<uint16_t, 2> &acquiredData);
	void didAcquireData(int tag, const np::Array<uint8_t, 2> &acquiredData);

	// signals
	signal<void> DidFinishImageProcessing;

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

#endif // ALAZAR_PROCESS_H_