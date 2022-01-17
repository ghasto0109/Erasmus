#ifndef DATA_RECORDING_H_
#define DATA_RECORDING_H_

#include <numcpp/array.h>

#include <memory>
#include <functional>

class MemoryManager;
class DataBufferModel;

class DataRecording
{
public:
    DataRecording(); 
    ~DataRecording();

    void start(std::function<void(std::unique_ptr<DataBufferModel> dataBuffer)> afterRecording);
    void start(int frameCountToRecord, std::function<void(std::unique_ptr<DataBufferModel> dataBuffer)> afterRecording);
    void stop();
	//void captureOneFrame();

	bool isRecording() { return _isRecording; }
	size_t getRecordedFrameCount();

    // object references
	std::unique_ptr<MemoryManager> _memoryManager;

	// callback from SignatecDAQ
	//void didAcquireData(int tag, const np::Array<uint16_t, 2> &acquiredData);
	void didAcquireData(int tag, const np::Array<uint8_t, 2> &acquiredData);

private:
	// Recording
	bool _isRecording;

	std::unique_ptr<DataBufferModel> _recordingDataBuffer;
	int _frameCountToRecord, _frameCountToDrop;

    // functor after recording
    std::function<void(std::unique_ptr<DataBufferModel> dataBuffer)> _afterRecording;
};

#endif // DATA_RECORDING_H_