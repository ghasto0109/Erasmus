#include "stdafx.h"
#include "DataRecording.h"

#include "MemoryManager.h"
#include "DataBufferModel.h"

using namespace std;

DataRecording::DataRecording() : 
    _memoryManager(new MemoryManager), 

	_isRecording(false), 
	_frameCountToRecord(-1), 
	_frameCountToDrop(0)
{
}

DataRecording::~DataRecording()
{
    // empty destructor for unique_ptr
}

size_t DataRecording::getRecordedFrameCount() 
{ 
	return _recordingDataBuffer ? _recordingDataBuffer->getFrameCount() : 0; 
}

void DataRecording::start(std::function<void(std::unique_ptr<DataBufferModel> dataBuffer)> afterRecording)
{
    if (_isRecording) return; // already recording

    _afterRecording = afterRecording;

    // Allocate data buffer model
    _recordingDataBuffer.reset(new DataBufferModel(_memoryManager.get()));

    // (Sync problem) set flag on later
    _isRecording = true;
}

void DataRecording::start(int frameCountToRecord, std::function<void(std::unique_ptr<DataBufferModel> dataBuffer)> afterRecording)
{
	_frameCountToDrop = 0;
	_frameCountToRecord = frameCountToRecord;

	start(afterRecording);
}

void DataRecording::stop()
{
    if (!_isRecording) return; // already not recording

    // (Sync problem) set flag off first
    _isRecording = false;

    // Pass data buffer to after recording functor
    _afterRecording(std::move(_recordingDataBuffer));
}

//void DataRecording::captureOneFrame()
//{
//	_frameCountToRecord = 1;
//	setRecording(true);
//}

// Implements DeviceDataAcquisitionDelegate

// Commentted out by MH
//void DataRecording::didAcquireData(int tag, const np::Array<uint16_t, 2> &acquiredData)
//{
//	// Save frame if it is recording
//	if (isRecording())
//	{
//		if (_frameCountToDrop > 0)
//		{
//			_frameCountToDrop--;
//			return;
//		}
//
//		if (_frameCountToRecord != -1)
//			_frameCountToRecord--;
//
//		if (false == _recordingDataBuffer->addFrame(acquiredData.raw_ptr()))
//		{
//			printf("# Failed addFrame : memory buffer full\n");
//
//            stop();
//            return;
//		}
//
//		if (_frameCountToRecord == 0)
//		{
//			_frameCountToRecord = -1;
//
//            stop();
//			return;
//		}
//	}
//}

// Added by MH
void DataRecording::didAcquireData(int tag, const np::Array<uint8_t, 2> &acquiredData)
{
	// Save frame if it is recording
	if (isRecording())
	{
		if (_frameCountToDrop > 0)
		{
			_frameCountToDrop--;
			return;
		}

		if (_frameCountToRecord != -1)
			_frameCountToRecord--;

		if (false == _recordingDataBuffer->addFrame(acquiredData.raw_ptr()))
		{
			printf("# Failed addFrame : memory buffer full\n");

			stop();
			return;
		}

		if (_frameCountToRecord == 0)
		{
			_frameCountToRecord = -1;

			stop();
			return;
		}
	}
}