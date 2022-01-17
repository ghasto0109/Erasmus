#include "stdafx.h"
#include "ImageProcess.h"

#include "OFDI.h"
#include "IntensityProcess.h"

using namespace std;

ImageProcess::ImageProcess() : 
    nChannels("nChannels", 2), 
	nScans("nScans", 1184), 
	nAlines("nAlines", 1024), 

	CalibrationFile("CalibrationFile", "calibration.dat"), 
	DC("DC", 0), 
	WindowLevel("WindowLevel", 149.f), 
	WindowWidth("WindowWidth", 40.f), 
    Transpose("Transpose", true), 
	DiagnosisAline("DiagnosisAline", 0), 

	ProcessID("ProcessID", -1), 
	FrameRate("FrameRate", 10), 

	Restart("Restart", this, &ImageProcess::initialize), 

	_running(false), 
	_lastUpdate(std::chrono::system_clock::now())
{
}

ImageProcess::~ImageProcess()
{
	if (_thread.joinable())
	{
		_running = false;
		_cond.notify_one();
		_thread.join();
	}
}

void ImageProcess::accept(property_visitor &visit)
{
	visit(nChannels);
	visit(nScans);
	visit(nAlines);

	visit(CalibrationFile);
	visit(DC);
	visit(WindowLevel);
	visit(WindowWidth);
    visit(Transpose); 

	visit(DiagnosisAline);
    visit(FrameRate);
	visit(ProcessID);

	visit(Restart);
}

void ImageProcess::initialize()
{
	// Stop thread
	if (_thread.joinable())
	{
		_running = false;
		_cond.notify_one();
		_thread.join();
	}

	// Allocate buffer to copy fringe data
	//_fringeBuffer = np::Array<uint16_t, 2>(nChannels * nScans, nAlines);
	_fringeBuffer = np::Array<uint8_t, 2>(nChannels * nScans * 3 / 2 + 0.5, nAlines);

	// Create recording thread
	_thread = std::thread(&ImageProcess::run, this);
}

//void ImageProcess::didAcquireData(int tag, const np::Array<uint16_t, 2> &acquiredData)
void ImageProcess::didAcquireData(int tag, const np::Array<uint8_t, 2> &acquiredData)
{
	// Pass preview until image processor is ready (skip frames while processing)
	if ((ProcessID == -1 || tag % 4 == ProcessID) && isProcessingReady())
	{
		// cout << "[ImageProcess] name: " << this->getName() << " frameIndex: " << tag << endl;

		// Process image (async call, result will be passed by delegate function)
		//startImageProcessing(acquiredData);
	}
}

bool ImageProcess::isProcessingReady()
{
	using namespace std::chrono;

	// Keep frame rate
	auto now = system_clock::now();
	if (duration<double>(now - _lastUpdate).count() < (1.0 / FrameRate))
	{
		return false;
	}

	if (_mutex.try_lock())
	{
		_mutex.unlock();
		
		// Update frame rate time
		_lastUpdate = std::chrono::system_clock::now();

		return true;
	}
	else
		return false;
}

//void ImageProcess::startImageProcessing(const uint16_t *fringeData)
void ImageProcess::startImageProcessing(const uint8_t *fringeData)
{
	{
		// Wait here if imageProcessThread is working on fringeBuffer
		// (Does this lock blocking acquisition thread?)
		// std::lock_guard<std::mutex> lock(_mutex);

		// Copy fringe data to frame buffer to process
        //memcpy(_fringeBuffer.raw_ptr(), fringeData, nChannels * nScans * nAlines * sizeof(uint16_t));
		memcpy(_fringeBuffer.raw_ptr(), fringeData, nChannels * nScans * nAlines * sizeof(uint8_t) * 3 / 2 + 0.5);
	}

	// Awake the process thread
	_cond.notify_one();
}

#include <ippi.h>

template <typename T>
inline IppiSize ippiSize(const np::Array<T, 2> &image)
{
	IppiSize result = { image.size(0), image.size(1) };
	return result;
}

template <typename T>
typename int stepBytes(const np::Array<T, 2> &image)
{
	return image.size(0) * image.itemSize();
}

inline IppiRect ippiRect(int width, int height)
{
	IppiRect result = { 0, 0, width, height };
	return result;
}

template <typename T>
inline IppiRect ippiRect(const np::Array<T, 2> &image)
{
	return ippiRect(image.size(0), image.size(1));
}

void rot90(np::Array<uint8_t, 2> &dst, const np::Array<uint8_t, 2> &src, int k = 1)
{
	if (k % 2 == 1) // if rotate 90 or 270, width and height are transposed
		np::setSize(dst, src.size(1), src.size(0));
	else // if rotate 180 or 360, with and height are not changed
		np::setSize(dst, src.size(0), src.size(1));

	// Shift rotated result
	// k = 0: (0, 0)
	// k = 1: (0, +height)
	// k = 2: (+width, +height)
	// k = 3: (+width, 0)

	static const int shift_x[4] = { 0, 0, 1, 1 }, shift_y[4] = { 0, 1, 1, 0 };

	ippiRotate_8u_C1R(
		src, ippiSize(src), stepBytes(src), ippiRect(src), 
		dst, stepBytes(dst), ippiRect(dst), 
		k * 90., 
		dst.size(0) * shift_x[k % 4], 
		dst.size(1) * shift_y[k % 4], 
		IPPI_INTER_NN);
}

void ImageProcess::run()
{
	//OFDI ofdi(nScans, nAlines);

	//// Update properties
	//ofdi.windowLevel = WindowLevel;
	//ofdi.windowWidth = WindowWidth;
	//ofdi.DC = DC;
	//ofdi.setCalibrationFilePath(CalibrationFile);

	//// Initialize intensity processing
	//IntensityProcess process(ofdi);

	//_running = true;
	//while (_running)
	//{
	//	// Wait until other threads send data
	//	std::unique_lock<std::mutex> lock(_mutex);
	//	_cond.wait(lock);

	//	if (!_running) break;

	//	// Call processor
	//	process(_fringeBuffer);

	//	// Transpose result
 //       if (Transpose)
 //       {
 //           rot90(resultImage, process.result);
 //       }
 //       else
 //       {
 //           np::setSize(resultImage, process.result.size(0), process.result.size(1));
 //           memcpy(resultImage, process.result, byteSize(process.result));
 //       }

	//	// a-lines for debugging
	//	debugFringe = process.debug_fringe;
	//	debugImage = process.debug_result;

	//	// Notify callback to update preview
	//	DidFinishImageProcessing(this);
	//}

	//std::cout << "Image processing thread is finished normally.\n" << std::endl;
}