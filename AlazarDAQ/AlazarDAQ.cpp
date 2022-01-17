// khkim: Update AcqToDisk.cpp example to AlazarDAQ class

//---------------------------------------------------------------------------
//
// Copyright (c) 2008-2013 AlazarTech, Inc.
// 
// AlazarTech, Inc. licenses this software under specific terms and
// conditions. Use of any of the software or derivatives thereof in any
// product without an AlazarTech digitizer board is strictly prohibited. 
// 
// AlazarTech, Inc. provides this software AS IS, WITHOUT ANY WARRANTY,
// EXPRESS OR IMPLIED, INCLUDING, WITHOUT LIMITATION, ANY WARRANTY OF
// MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. AlazarTech makes no 
// guarantee or representations regarding the use of, or the results of the 
// use of, the software and documentation in terms of correctness, accuracy,
// reliability, currentness, or otherwise; and you rely on the software,
// documentation and results solely at your own risk.
// 
// IN NO EVENT SHALL ALAZARTECH BE LIABLE FOR ANY LOSS OF USE, LOSS OF 
// BUSINESS, LOSS OF PROFITS, INDIRECT, INCIDENTAL, SPECIAL OR CONSEQUENTIAL 
// DAMAGES OF ANY KIND. IN NO EVENT SHALL ALAZARTECH'S TOTAL LIABILITY EXCEED
// THE SUM PAID TO ALAZARTECH FOR THE PRODUCT LICENSED HEREUNDER.
// 
//---------------------------------------------------------------------------

// AcqToDisk.cpp : 
//
// This program demonstrates how to configure an ATS9360-FIFO to make a
// Traditional AutoDMA acquisition. 
//

#include "AlazarDAQ.h"

#include <stdio.h>
#include <conio.h>

#include <AlazarError.h>
#include <AlazarApi.h>
#include <AlazarCmd.h>

using namespace std;

AlazarDAQ::AlazarDAQ() :
    nChannels("nChannels", 2), 
	nScans("nScans", 640), 
	nAlines("nAlines", 512), 

    ExternalClock("ExternalClock", true),  //20200914
	AutoTrigger("AutoTrigger", false), 
    TriggerDelay("TriggerDelay", 0), 

	Start("Start", this, &AlazarDAQ::start), 
	Stop("Stop", this, &AlazarDAQ::stop), 

    _dirty(true), 
	_running(false), 
	boardHandle(nullptr)
{
}

AlazarDAQ::~AlazarDAQ()
{
	stop();
}

void AlazarDAQ::accept(property_visitor &visit)
{
    visit(nChannels); 
	visit(nScans);
	visit(nAlines);

    visit(ExternalClock);
	visit(AutoTrigger);
    visit(TriggerDelay);

	visit(Start);
	visit(Stop);
}

bool AlazarDAQ::init()
{
	saveData = false;

	// Select a board
	U32 systemId = 1;
	U32 boardId = 1;

	// Get a handle to the board
	boardHandle = AlazarGetBoardBySystemID(systemId, boardId);
	if (boardHandle == NULL)
	{
		printf("Error: Unable to open board system Id %u board Id %u\n", systemId, boardId);
		return false;
	}

	RETURN_CODE retCode;

	// Specify the sample rate (see sample rate id below)
	// double samplesPerSec = 500.e6;
	// const U32 SamplingRate = SAMPLE_RATE_500MSPS;
	const U32 SamplingRate = PLL_10MHZ_REF_3948MSPS_BASE;  //20181031
	//const U32 SamplingRate = SAMPLE_RATE_4000MSPS; // only for internal clock
	const U32 InputVoltageRange = INPUT_RANGE_PM_500_MV;

	// Select clock parameters as required to generate this sample rate.
	//
	// For example: if samplesPerSec is 100.e6 (100 MS/s), then:
	// - select clock source INTERNAL_CLOCK and sample rate SAMPLE_RATE_100MSPS
	// - select clock source FAST_EXTERNAL_CLOCK, sample rate SAMPLE_RATE_USER_DEF,
	//   and connect a 100 MHz signalto the EXT CLK BNC connector.
	retCode =
		AlazarSetCaptureClock(
		boardHandle,			// HANDLE -- board handle
		ExternalClock ? FAST_EXTERNAL_CLOCK : EXTERNAL_CLOCK_10MHz_REF,		// U32 -- clock source id
		ExternalClock ? SAMPLE_RATE_USER_DEF : SamplingRate,	// U32 -- sample rate id  // 20200914
		CLOCK_EDGE_RISING,		// U32 -- clock edge id
		0						// U32 -- clock decimation 
		);
	//retCode = 
	//	AlazarSetCaptureClock(
	//		boardHandle,			// HANDLE -- board handle
    //        ExternalClock ? EXTERNAL_CLOCK_AC : INTERNAL_CLOCK,		// U32 -- clock source id
    //        ExternalClock ? SAMPLE_RATE_USER_DEF : SamplingRate,	// U32 -- sample rate id
	//		CLOCK_EDGE_RISING,		// U32 -- clock edge id
	//		0						// U32 -- clock decimation 
	//		);
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarSetCaptureClock failed -- %s\n", AlazarErrorToText(retCode));
		return false;
	}



    // External Clock Settings
    retCode =
        AlazarSetExternalClockLevel(
        boardHandle,			// HANDLE -- board handle
        50.f                    // Voltage level in percentage
        );
    if (retCode != ApiSuccess)
    {
        printf("Error: AlazarSetExternalClockLevel failed -- %s\n", AlazarErrorToText(retCode));
        return false;
    }


	// Select CHA input parameters as required
	retCode = 
		AlazarInputControl(
			boardHandle,			// HANDLE -- board handle
			CHANNEL_A,			    // U8 -- input channel 
            DC_COUPLING,			// U32 -- input coupling id
			InputVoltageRange,		// U32 -- input range id
			IMPEDANCE_50_OHM		// U32 -- input impedance id
			);
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarInputControl failed -- %s\n", AlazarErrorToText(retCode));
		return false;
	}

	// Select CHB input parameters as required
	retCode = 
		AlazarInputControl(
			boardHandle,			// HANDLE -- board handle
			CHANNEL_B,				// U8 -- channel identifier
			DC_COUPLING,			// U32 -- input coupling id
			InputVoltageRange,		// U32 -- input range id
			IMPEDANCE_50_OHM		// U32 -- input impedance id
			);
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarInputControl failed -- %s\n", AlazarErrorToText(retCode));
		return false;
	}

	// Select trigger inputs and levels as required
	retCode = 
		AlazarSetTriggerOperation(
			boardHandle,			// HANDLE -- board handle
			TRIG_ENGINE_OP_J,		// U32 -- trigger operation 
			TRIG_ENGINE_J,			// U32 -- trigger engine id
			TRIG_EXTERNAL,			// U32 -- trigger source id
			TRIGGER_SLOPE_POSITIVE,	// U32 -- trigger slope id
			128 + (int)(128 * 0.5),// U32 -- trigger level from 0 (-range) to 255 (+range)
			TRIG_ENGINE_K,			// U32 -- trigger engine id
			TRIG_DISABLE,			// U32 -- trigger source id for engine K
			TRIGGER_SLOPE_POSITIVE,	// U32 -- trigger slope id
			128						// U32 -- trigger level from 0 (-range) to 255 (+range)
			);
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarSetTriggerOperation failed -- %s\n", AlazarErrorToText(retCode));
		return false;
	}

	// Select external trigger parameters as required
	retCode =
		AlazarSetExternalTrigger( 
			boardHandle,			// HANDLE -- board handle
			DC_COUPLING,			// U32 -- external trigger coupling id
			ETR_TTL				// U32 -- external trigger range id   ETR_2V5 20210807
			);

	// Set trigger delay as required. 
	U32 triggerDelay_samples = (U32)TriggerDelay;
	retCode = AlazarSetTriggerDelay(boardHandle, triggerDelay_samples);
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarSetTriggerDelay failed -- %s\n", AlazarErrorToText(retCode));
		return FALSE;
	}

	// Set trigger timeout as required. 
	// NOTE:
	// The board will wait for a for this amount of time for a trigger event. 
	// If a trigger event does not arrive, then the board will automatically 
	// trigger. Set the trigger timeout value to 0 to force the board to wait 
	// forever for a trigger event.
	//
	// IMPORTANT: 
	// The trigger timeout value should be set to zero after appropriate 
	// trigger parameters have been determined, otherwise the 
	// board may trigger if the timeout interval expires before a 
	// hardware trigger event arrives.

	//double triggerTimeout_sec = 0.;
	//U32 triggerTimeout_clocks = (U32)(triggerTimeout_sec / 10.e-6 + 0.5);
	//U32 triggerTimeout_clocks = AutoTrigger ? 1U : 0U; // ns
	U32 triggerTimeout_clocks = 0U; // ns

	retCode = 
		AlazarSetTriggerTimeOut(
			boardHandle,			// HANDLE -- board handle
			triggerTimeout_clocks	// U32 -- timeout_sec / 10.e-6 (0 means wait forever)
			);
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarSetTriggerTimeOut failed -- %s\n", AlazarErrorToText(retCode));
		return false;
	}

	// Configure AUX I/O connector as required
	//retCode =
	//	AlazarConfigureAuxIO(
	//	boardHandle, // HANDLE -- board handle
	//	AUX_IN_TRIGGER_ENABLE, // U32 -- mode
	//	TRIGGER_SLOPE_POSITIVE // U32 -- parameter
	//	);
	//if (retCode != ApiSuccess)
	//{
	//	printf("Error: AlazarConfigureAuxIO failed -- %s\n", AlazarErrorToText(retCode));
	//	return FALSE;
	//}

	return true;
}

void AlazarDAQ::start()
{
	cout << "[AlazarDAQ] start()" << endl;

    if (_dirty && !init())
    {
        cout << "ERROR: Cannot start acquisition. Failed to initialize." << endl;
        return;
    }

	if (_thread.joinable())
	{
		cout << "ERROR: acquisition is already running." << endl;
		return;
	}

	_thread = std::thread(&AlazarDAQ::run, this);

	if (::SetThreadPriority(_thread.native_handle(), THREAD_PRIORITY_TIME_CRITICAL) == 0)
		cout << "WARNING: failed to set acquisition thread priority: " << ::GetLastError() << endl;
}

void AlazarDAQ::stop()
{
	cout << "[AlazarDAQ] stop()" << endl;

	if (_thread.joinable())
	{
		_running = false;
		_thread.join();
	}
	else
	{
		cout << "ERROR: acquisition is not running." << endl;
		return;
	}
}

void AlazarDAQ::run()
{
	FILE *fpData = NULL;

	fpData = fopen("d:/TSKim/Data/170804/test/data.bin", "wb");
	if (fpData == NULL)
	{
		printf("Error: Unable to create data file -- %u\n", GetLastError());
	}

	// Select the number of pre-trigger samples per record 
    U32 preTriggerSamples = TriggerDelay;

    // Select the number of post-trigger samples per record 
    U32 postTriggerSamples = nScans - TriggerDelay;

    // Specify the number of records per DMA buffer
    U32 recordsPerBuffer = nAlines;

    // Select which channels to capture (A, B, or both)
    U32 channelMask;

    // Calculate the number of enabled channels from the channel mask 
    int channelCount = nChannels;

    if (nChannels == 1)
    {
        channelMask = CHANNEL_A;
    }
    else if (nChannels == 2)
    {
        channelMask = CHANNEL_A | CHANNEL_B;
    }
    else
    {
        printf("Error: Invalid channel count %d\n", nChannels);
        return;
    }

	// *********** For packing 12 bits (Original)
	//// Get the sample size in bits, and the on-board memory size in samples per channel
	//U8 bitsPerSample;
	//U32 maxSamplesPerChannel;
	//RETURN_CODE retCode = AlazarGetChannelInfo(boardHandle, &maxSamplesPerChannel, &bitsPerSample);
	//if (retCode != ApiSuccess)
	//{
	//	printf("Error: AlazarGetChannelInfo failed -- %s\n", AlazarErrorToText(retCode));
	//	return;
	//} 

	//// Calculate the size of each DMA buffer in bytes
	//U32 bytesPerSample = (bitsPerSample + 7) / 8;
	//U32 samplesPerRecord = preTriggerSamples + postTriggerSamples;
	//U32 bytesPerRecord = bytesPerSample * samplesPerRecord;
	//U32 bytesPerBuffer = bytesPerRecord * recordsPerBuffer * channelCount;  

	//std::cout << bitsPerSample + 7 << std::endl;
	//std::cout << bytesPerSample << std::endl;
	// *********** For packing 12 bits (Original / END)

	// *********** For packing 12 bits (12 bits)
	BOOL packing = true;
	int bitsPerSample = packing ? 12 : 16;
	U32 samplesPerRecord = preTriggerSamples + postTriggerSamples;
	U32 samplesPerBuffer = samplesPerRecord * recordsPerBuffer * channelCount;
	U32 bytesPerRecord = bitsPerSample * samplesPerRecord / 8; // 0.5 compensates for double to integer conversion
	U32 bytesPerBuffer = bytesPerRecord * recordsPerBuffer * channelCount;
	// *********** For packing 12 bits (12 bits  / END)

	// Allocate memory for DMA buffers
	BOOL success = TRUE;

	U32 bufferIndex;
	for (bufferIndex = 0; (bufferIndex < BUFFER_COUNT) && success; bufferIndex++)
	{
#ifdef _WIN32	// Allocate page aligned memory
		BufferArray[bufferIndex] = (U16*) VirtualAlloc(NULL, bytesPerBuffer, MEM_COMMIT, PAGE_READWRITE);
#else
		BufferArray[bufferIndex] = (U16*) malloc(bytesPerBuffer);
#endif
		if (BufferArray[bufferIndex] == NULL)
		{
			printf("Error: Alloc %u bytes failed\n", bytesPerBuffer);
			success = FALSE;
		}
	}

	// *********** For packing 12 bits (Original ADC mode)
	//For Dual-edge samping in 10 MHz PLL mode
	RETURN_CODE retCode;
	retCode = AlazarSetParameterUL(
		boardHandle, // HANDLE -- board handle
		channelMask, // U8 -- channel to acquire
		SET_ADC_MODE,
		ADC_MODE_DES   //ADC_MODE_DES ADC_MODE_DEFAULT 20210805
		);
	if (retCode != ApiSuccess)
		{
			printf("Error: AlazarSetParameter failed -- %s\n", AlazarErrorToText(retCode));
			success = FALSE;
		}
	// *********** For packing 12 bits (Original ADC mode / END)

	// *********** For packing 12 bits (12 bits)
	//RETURN_CODE retCode;

	if (success)
	{
		retCode = AlazarSetParameter(
			boardHandle, 0, PACK_MODE, packing ? PACK_12_BITS_PER_SAMPLE : PACK_DEFAULT);
		if (retCode != ApiSuccess)
		{
			printf("Error: AlazarSetParameter failed -- %s\n", AlazarErrorToText(retCode));
			success = FALSE;
		}
	}
	// *********** For packing 12 bits (12 bits / END)

	// Configure the record size 
	if (success)
	{
		retCode = 
			AlazarSetRecordSize (
				boardHandle,			// HANDLE -- board handle
				preTriggerSamples,		// U32 -- pre-trigger samples
				postTriggerSamples		// U32 -- post-trigger samples
				);
		if (retCode != ApiSuccess)
		{
			printf("Error: AlazarSetRecordSize failed -- %s\n", AlazarErrorToText(retCode));
			success = FALSE;
		}
	}

	// Configure the board to make a traditional AutoDMA acquisition
	if (success)
	{
		U32 admaFlags;

		if (AutoTrigger)
		{
			// Continuous mode (for debugging)
			admaFlags = ADMA_EXTERNAL_STARTCAPTURE | // Start acquisition when we call AlazarStartCapture 
				ADMA_TRIGGERED_STREAMING |		 // Acquire a continuous stream of sample data with trigger
				//ADMA_CONTINUOUS_MODE |		 // Acquire a continuous stream of sample data without trigger
				//ADMA_INTERLEAVE_SAMPLES;	 // For ATS9370
				ADMA_FIFO_ONLY_STREAMING;	 // The ATS9360-FIFO does not have on-board memory
		}
		else
		{
			// Acquire records per each trigger
			admaFlags = ADMA_EXTERNAL_STARTCAPTURE |	// Start acquisition when AlazarStartCapture is called
				ADMA_FIFO_ONLY_STREAMING |		// The ATS9360-FIFO does not have on-board memory
				ADMA_TRADITIONAL_MODE;			// Acquire multiple records optionally with pretrigger 
		}

		// samples and record headers
		 retCode = 
		 	AlazarBeforeAsyncRead(
		 		boardHandle,			// HANDLE -- board handle
		 		channelMask,			// U32 -- enabled channel mask
		 		-(long)preTriggerSamples,	// long -- offset from trigger in samples
		      samplesPerRecord,		// U32 -- samples per record
		 		recordsPerBuffer,		// U32 -- records per buffer 
		 		0x7fffffff,					// U32 -- records per acquisition (infinitly)
		 		admaFlags				// U32 -- AutoDMA flags
		 		); 
		//retCode =
		//	AlazarBeforeAsyncRead(
		//		boardHandle,			// HANDLE -- board handle
		//		channelMask,			// U32 -- enabled channel mask
		//		0,	// long -- offset from trigger in samples
		//	    1024000,		// U32 -- samples per record
		//		1,		// U32 -- records per buffer 
		//		0x7fffffff,				// U32 -- records per acquisition (infinitly)
		//		admaFlags				// U32 -- AutoDMA flags
		//		);
		if (retCode != ApiSuccess)
		{
			printf("Error: AlazarBeforeAsyncRead failed -- %s\n", AlazarErrorToText(retCode));
			success = FALSE;
		}
	}

	// Add the buffers to a list of buffers available to be filled by the board
	for (bufferIndex = 0; (bufferIndex < BUFFER_COUNT) && success; bufferIndex++)
    {
		U16* pBuffer = BufferArray[bufferIndex];
		retCode = AlazarPostAsyncBuffer(boardHandle, pBuffer, bytesPerBuffer);
		if (retCode != ApiSuccess)
		{
			printf("Error: AlazarPostAsyncBuffer %u failed -- %s\n", bufferIndex, AlazarErrorToText(retCode));
			success = FALSE;
		}
	}

	// Arm the board system to wait for a trigger event to begin the acquisition 
	if (success)
	{
  //     double triggerCycleTime, triggerPulseWidth;
  //     AlazarOCTIgnoreBadClock(boardHandle, TRUE, 1.2e-6, 0.4e-6, &triggerCycleTime, &triggerPulseWidth);
		retCode = AlazarStartCapture(boardHandle);
		if (retCode != ApiSuccess)
		{
			printf("Error: AlazarStartCapture failed -- %s\n", AlazarErrorToText(retCode));
			success = FALSE;
		}
	}
	
	// Wait for each buffer to be filled, process the buffer, and re-post it to the board.
	if (success)
	{
		DWORD tickStart, tickLastUpdate;
		U32 buffersCompleted = 0;
		INT64 bytesTransferred = 0, bytesTransferredPerUpdate = 0;

		tickStart = tickLastUpdate = ::GetTickCount();
		// *********** For packing 12 bits (12 bits)
		std::vector<U16> unpacked_buffer(samplesPerBuffer);
		// *********** For packing 12 bits (12 bits / END)

		_running = true;
		int count = 0;
		while (_running)
		{
			//if (count++ > 7000)
			//	break;
			// Set a buffer timeout that is longer than the time 
			// required to capture all the records in one buffer.
			DWORD timeout_ms = 5000;

			// Wait for the buffer at the head of the list of available buffers
			// to be filled by the board.
			bufferIndex = buffersCompleted % BUFFER_COUNT;
			U16* pBuffer = BufferArray[bufferIndex];
			
			retCode = AlazarWaitAsyncBufferComplete(boardHandle, pBuffer, timeout_ms);
			if (retCode != ApiSuccess)
			{
				printf("Error: AlazarWaitAsyncBufferComplete failed -- %s\n", AlazarErrorToText(retCode));
				success = FALSE;
			}

			if (success)
			{
				// The buffer is full and has been removed from the list
				// of buffers available for the board
				buffersCompleted++;
				bytesTransferred += bytesPerBuffer;
				bytesTransferredPerUpdate += bytesPerBuffer;

				// Process sample data in this buffer. 

				// NOTE: 
				//
				// While you are processing this buffer, the board is already
				// filling the next available buffer(s). 
				// 
				// You MUST finish processing this buffer and post it back to the 
				// board before the board fills all of its available DMA buffers
				// and on-board memory. 
				// 
				// Records are arranged in the buffer as follows:
				// R0[AB], R1[AB], R2[AB] ... Rn[AB]
				//
				// Samples values are arranged contiguously in each record.
				// A 12-bit sample code is stored in the most significant 
				// bits of each 16-bit sample value.
				//
				// Sample codes are unsigned by default. As a result:
				// - a sample code of 0x000 represents a negative full scale input signal.
				// - a sample code of 0x800 represents a ~0V signal.
				// - a sample code of 0xFFF represents a positive full scale input signal.

				// *********** For packing 12 bits (12 bits)
				//if (packing)
				//{
				//	U8 *cBuffer = (U8 *)pBuffer;
				//	// Extract 3 bytes (2 samples) at a time
				//	for (int i = 0, j = 0; i < bytesPerBuffer;)
				//	{
				//		const U16 uByte0 = cBuffer[i++];
				//		const U16 uByte1 = cBuffer[i++];
				//		const U16 uByte2 = cBuffer[i++];

				//		// convert to two 12-bit samples
				//		const U16 uSample0 = uByte0 | (((uByte1 & 0x0F)) << 8);
				//		const U16 uSample1 = ((uByte1 & 0xF0) >> 4) | (uByte2 << 4);
				//		unpacked_buffer[j++] = uSample0 << 4;
				//		unpacked_buffer[j++] = uSample1 << 4;
				//	}
				//}
				// *********** For packing 12 bits (12 bits / END)

				// Callback
				// *********** For packing 12 bits (Original)
				//np::Array<uint16_t, 2> frame(pBuffer, channelCount * nScans, nAlines);
				np::Array<uint8_t, 2> frame((uint8_t*)pBuffer, channelCount * nScans * 3 / 2 + 0.5, nAlines);
				// *********** For packing 12 bits (Original / END)

				//std::cout << (int)(channelCount * nScans * 3 / 2) << std::endl;
				//std::cout << (int)(nChannels * nScans * nAlines * sizeof(uint8_t)* 3 / 2 + 0.5) << std::endl;
				//exit(1);

				// *********** For packing 12 bits (12 bits)
				//np::Array<uint16_t, 2> frame(&unpacked_buffer[0], channelCount * nScans, nAlines); 
				// *********** For packing 12 bits (12 bits / END)
				DidAcquireData(buffersCompleted, frame);

				if (saveData)
				{
					// Write record to file
					size_t bytesWritten = fwrite(pBuffer, sizeof(BYTE), bytesPerBuffer, fpData);
					if (bytesWritten != bytesPerBuffer)
					{
						printf("Error: Write buffer %u failed -- %u\n", buffersCompleted,
							GetLastError());
						success = FALSE;
					}
				}
			}

			// Add the buffer to the end of the list of available buffers.
			if (success)
			{
				retCode = AlazarPostAsyncBuffer(boardHandle, pBuffer, bytesPerBuffer);
				if (retCode != ApiSuccess)
				{
					printf("Error: AlazarPostAsyncBuffer failed -- %s\n", AlazarErrorToText(retCode));
					success = FALSE;
				}
			}

			// If the acquisition failed, exit the acquisition loop
			if (!success)
				break;

			// Periodically update progress
			ULONG tickNow = GetTickCount();
			//std::cout << tickNow << ", " << tickLastUpdate << std::endl;
            // if (tickNow - tickLastUpdate > 2000)
            if (tickNow - tickLastUpdate > 1000)
			{
				double dRate;

				ULONG dwTotalElapsed = tickNow - tickStart;
				ULONG dwElapsed = tickNow - tickLastUpdate;
				tickLastUpdate = tickNow;

				// Rate calculation per update
				dRate = (bytesTransferredPerUpdate / 1000000.0) / (dwElapsed / 1000.0);

				//printf("[AlazarDAQ] Total data: %0.2f MB, Rate: %6.2f MB/s, Elapsed Time: %d sec\n",
				//	bytesTransferred / 1000000.0, dRate, dwTotalElapsed / 1000);

                printf("%d %f\n", 
                    bytesTransferredPerUpdate / bytesPerBuffer, 
                    bytesTransferredPerUpdate / bytesPerBuffer * nAlines / 1000.f);

                // printf("%f kHz\n", bytesTransferredPerUpdate / bytesPerBuffer * nAlines / 1000.f);

				// reset
				bytesTransferredPerUpdate = 0;
			}
		}

		// Display results
		double transferTime_sec = (GetTickCount() - tickStart) / 1000.;
		printf("Capture completed in %.2lf sec\n", transferTime_sec);

		U32 recordsTransferred = recordsPerBuffer * buffersCompleted;

		double buffersPerSec;
		double bytesPerSec;
		double recordsPerSec;

		if (transferTime_sec > 0.)
		{
			buffersPerSec = buffersCompleted / transferTime_sec;
			bytesPerSec = bytesTransferred / transferTime_sec;
			recordsPerSec = recordsTransferred / transferTime_sec;
		}
		else
		{
			buffersPerSec = 0.;
			bytesPerSec = 0.;
			recordsPerSec = 0.;
		}

		printf("Captured %u buffers (%.4g buffers per sec)\n", buffersCompleted, buffersPerSec);
		printf("Captured %u records (%.4g records per sec)\n", recordsTransferred, recordsPerSec);
		printf("Transferred %I64d bytes (%.4g bytes per sec)\n", bytesTransferred, bytesPerSec);
	}

	if (fpData != NULL && !saveData)
		fclose(fpData);

	// Abort the acquisition
	retCode = AlazarAbortAsyncRead(boardHandle);
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarAbortAsyncRead failed -- %s\n", AlazarErrorToText(retCode));
		success = FALSE;
	}

	// Free all memory allocated
	for (bufferIndex = 0; bufferIndex < BUFFER_COUNT; bufferIndex++)
	{
		if (BufferArray[bufferIndex] != NULL)
		{
#ifdef _WIN32
			VirtualFree(BufferArray[bufferIndex], 0, MEM_RELEASE);
#else
			free(BufferArray[bufferIndex]);
#endif
		}
	}
}
