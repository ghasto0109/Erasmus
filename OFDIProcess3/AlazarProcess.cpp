#include "stdafx.h"
#include "AlazarProcess.h"

// for FFT_R2C
#include "IntensityProcess.h"

#include <ipps.h>
#include <ippac.h>

using namespace std;
using namespace std::chrono;

AlazarProcess::AlazarProcess() : 
    nChannels("nChannels", 2), 
	nScans("nScans", 1184), 
	nAlines("nAlines", 1024), 
	WindowLevel("WindowLevel", 150.f),
	WindowWidth("WindowWidth", 80.f),
	_running(false), 
	_lastUpdate(system_clock::now())
{
}

AlazarProcess::~AlazarProcess()
{
	if (_thread.joinable())
	{
		_running = false;
		_cond.notify_one();
		_thread.join();
	}
}

void AlazarProcess::accept(property_visitor &visit)
{
    visit(nChannels);
	visit(nScans);
	visit(nAlines);
	visit(WindowLevel);
	visit(WindowWidth);
}

void AlazarProcess::initialize()
{
	// Stop thread
	if (_thread.joinable())
	{
		_running = false;
		_cond.notify_one();
		_thread.join();
	}

    // FFT size : least 2 ^ n number larger than nScans ex) 1182 -> 2048
    // nScans2n = 2 ^ ceil(log2(nScans));
    const int nScans2n = 1 << (int)ceil(log2((double)nScans));
    
    // Allocate buffers
	//np::setSize(_fringeBuffer, nChannels * nScans, nAlines);
	np::setSize(_fringeBuffer, nChannels * nScans * 3 / 2 + 0.5, nAlines);
	np::setSize(debugFringe, 2 * nScans);
	np::setSize(debugImage, 2 * nScans2n/2);
	//np::setSize(resultImage, nAlines, nScans2n / 2);
	np::setSize(resultImage, nAlines * 3 / 2 + 0.5, nScans2n / 2);

	// Create recording thread
	_thread = std::thread(&AlazarProcess::run, this);
}

//void AlazarProcess::didAcquireData(int tag, const np::Array<uint16_t, 2> &acquiredData)
void AlazarProcess::didAcquireData(int tag, const np::Array<uint8_t, 2> &acquiredData)
{
	// Pass preview until image processor is ready (skip frames while processing)
	if (isProcessingReady())
	{
		// cout << "[AlazarProcess] name: " << this->getName() << " frameIndex: " << tag << endl;

		// Process image (async call, result will be passed by delegate function)
		//startImageProcessing(acquiredData);
	}
}

bool AlazarProcess::isProcessingReady()
{
	// Keep frame rate
	auto now = system_clock::now();
	if (duration<double>(now - _lastUpdate).count() < 0.10)
		return false;

	if (_mutex.try_lock())
	{
		_mutex.unlock();
		return true;
	}
	else
		return false;
}

//void AlazarProcess::startImageProcessing(const uint16_t *fringeData)
void AlazarProcess::startImageProcessing(const uint8_t *fringeData)
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

void AlazarProcess::run()
{
	//using namespace np;

 //   // FFT size : least 2 ^ n number larger than nScans ex) 1182 -> 2048
 //   // nScans2n = 2 ^ ceil(log2(nScans));
 //   const int nScans2n = 1 << (int)ceil(log2((double)nScans));
 //   
 //   Array<uint16_t, 2> fringeX(nScans, nAlines), fringeY(nScans, nAlines);
	//FFT_R2C fft;
	//Array<float, 2> fringe(nScans, nAlines);
	//Array<complex<float>, 2> depth(nScans2n, nAlines);
	//Array<float, 2> intensity(nScans2n/2, nAlines);
	//Array<float> temp(nScans2n/2);

 //   Array<float> window(nScans);
 //   for (int i = 0; i < nScans; i++)
 //   {
 //       const int N = nScans;
 //       const float widthratio = 0.8f;
 //       const int n = i + 1 + int(N/widthratio/2) - int(N/2) - 70;
 //       const float pi = 3.141592f;
 //       

 //       window(i) = 0.35875f - 0.48829f*cos(widthratio * 2 * pi*n / (N - 1)) + 0.14128f*cos(widthratio * 4 * pi*n / (N - 1)) - 0.01168f*cos(widthratio * 6 * pi*n / (N - 1));
 //   }

 //   Array<float> zerowindow(nScans);
 //   for (int i = 0; i < nScans; i++)
 //   {
 //       const int N = nScans;
 //       const int n = i + 1;
 //       const int nZerosstart = 0;
 //       const int nZerosend = nScans - 140;

 //       if (i < nZerosstart)
 //           zerowindow(i) = 0;
 //       else if (i > nZerosend)
 //           zerowindow(i) = 0;
 //       else
 //           zerowindow(i) = 1;
 //   }

	//_running = true;
	//while (_running)
	//{
	//	// Wait until other threads send data
	//	std::unique_lock<std::mutex> lock(_mutex);
	//	_cond.wait(lock);

	//	if (!_running) break;

	//	// ===== start processing code =====

 //       if (nChannels == 2)
 //       {
 //           // Deinterlace fringe
 //           Ipp16u *deinterlaced_fringe[2] = { fringeX, fringeY };
 //           ippsDeinterleave_16s((Ipp16s *)_fringeBuffer.raw_ptr(), 2, nAlines * nScans, (Ipp16s **)deinterlaced_fringe);

 //           // Convert from uint16_t to float
 //           ippsConvert_16u32f(fringeX, fringe, fringe.length());
 //       }
 //       else if (nChannels == 1)
 //       {
 //           // Convert from uint16_t to float
 //           ippsConvert_16u32f(_fringeBuffer, fringe, fringe.length());
 //       }
 //       
 //       // Subtract 32768 to use singed 16bit value
 //       ippsSubC_32f(fringe, 32768.f, fringe, fringe.length());

 //        // Windowing
 //       for (int line = 0; line < fringe.size(1); line++)
 //           ippsMul_32f(&fringe(0, line), window, &fringe(0, line), window.length());
 //      
 //       // Zeroing first nZeros samples of an A-scan
 //       for (int line = 0; line < fringe.size(1); line++)
 //           ippsMul_32f(&fringe(0, line), zerowindow, &fringe(0, line), zerowindow.length());
	//	
 //       // FFT
	//	fft(depth, fringe);

	//	// intensity
	//	for (int line = 0; line < intensity.size(1); line++)
	//		for (int scan = 0; scan < intensity.size(0); scan++)
	//		{
	//			const complex<float> &phase = depth(scan, line);
	//			intensity(scan, line) = phase.real() * phase.real() + phase.imag() * phase.imag();
	//		}

	//	// image
 //       
 //       // ex) WindowLevel = (20 + 150) / 2, WindowWidth = (150 - 20) / 2
 //       float LowLevel = WindowLevel - WindowWidth / 2, HighLevel = WindowLevel + WindowWidth / 2;

 //       for (int line = 0; line < intensity.size(1); line++)
	//		for (int scan = 0; scan < intensity.size(0); scan++)
	//		{
 //               // temp have to be in 0.0 ~ 1.0
 //               float temp = 4.3429f * log(intensity(scan, line));
 //               //if (temp > max) max = temp;
 //               //if (temp < min) min = temp;

 //               temp = (temp - LowLevel) / (HighLevel - LowLevel);
 //               temp = temp * 255.f;
 //               if (temp > 255.f) temp = 255.f;
 //               if (temp < 0.f) temp = 0.f;

	//			// transposed image
	//			resultImage(line, scan) = (uint8_t)temp;
	//		}

	//	// ===== end processing code =====

 //       const int diagnosisAline = nAlines / 2;
 //       const int Magnitude = 1;
 //       
	//	for (int i = 0; i < nScans; i++)
	//	{
 //           // float fx = fringeX(i, 0), fy = fringeY(i, 0);
 //           float fx = fringe(i, diagnosisAline);
 //           
 //           debugFringe(i) = (uint16_t)(fx * Magnitude + 32768);
 //           debugFringe(i + nScans) = 0;
 //           // debugFringe(i + nScans) = (uint16_t)((fy - 32768) * Magnitude + 32768);
 //       }

 //       float max = std::numeric_limits<float>::min(), min = std::numeric_limits<float>::max();
 //       
 //       for (int i = 0; i < nScans2n / 2; i++)
	//	{
	//		//float temp = 4.3429f * log(intensity(i, 0));
	//		//temp -= WindowLevel - (WindowWidth / 2);
	//		//temp *= 65535 / WindowWidth;

 //           // temp have to be in 0.0 ~ 1.0
 //           float temp = 4.3429f * log(intensity(i, diagnosisAline));
 //           //if (temp > max) max = temp;
 //           //if (temp < min) min = temp;

 //           temp = (temp - LowLevel) / (HighLevel - LowLevel);
 //           temp = temp * 65535.f;
 //           if (temp > 65535.f) temp = 65535.f;
 //           if (temp < 0.f) temp = 0.f;

 //           debugImage(i) = (uint16_t)temp;
 //           debugImage(i + nScans2n/2) = 0;
	//	}

 //       //std::cout << "max: " << max << " min: " << min << std::endl;

 //       //float max = std::numeric_limits<float>::min(), min = std::numeric_limits<float>::max();
 //       //for (int i = 0; i < intensity.length(); i++)
 //       //{
 //       //    if (intensity(i) > max) max = intensity(i);
 //       //    if (intensity(i) < min) min = intensity(i);
 //       //}

 //       //std::cout << "max: " << max << " min: " << min << std::endl;

	//	//ippsLn_32f(&intensity(0, 0), temp, nScans2n/2);
	//	//ippsMulC_32f(temp, 4.3429f, temp, nScans2n/2);
	//	//ippsSubC_32f(temp, WindowLevel - (WindowWidth / 2), temp, nScans2n/2);
	//	//ippsMulC_32f(temp, 65535 / WindowWidth, temp, nScans2n/2);
	//	//ippsConvert_32f16u_Sfs(temp, debugImage, nScans2n/2, ippRndNear, 0);

	//	//for (int i = 0; i < nScans2n / 2; i++)
	//	//{
	//	//	debugImage(i) = i * 500 % 65535;
	//	//	debugImage(i + nScans2n/2) = (i * 500 + 10000) % 65535;
	//	//}

	//	// Notify callback to update preview
	//	DidFinishImageProcessing();

	//	// Update processing time
	//	_lastUpdate = system_clock::now();
	//}

	//std::cout << "[AlazarProcess] Image processing thread is finished normally.\n" << std::endl;
}