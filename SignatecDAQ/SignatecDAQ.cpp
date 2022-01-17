#include "stdafx.h"
#include "SignatecDAQ.h"

#include <px14.h>

using namespace std;

// Signatec 보드로 들어오는 신호의 voltage 범위
// const int DATASOURCE_VOLTAGE_RANGE = PX14VOLTRNG_0_277_VPP;
const int DATASOURCE_VOLTAGE_RANGE = PX14VOLTRNG_1_388_VPP;

SignatecDAQ::SignatecDAQ() :
    nChannels("nChannels", 2), 
	nScans("nScans", 1190),
	nAlines("nAlines", 1024),
	UseVirtualDevice("UseVirtualDevice", false),
	UseInternalTrigger("UseInternalTrigger", false),
	PreTrigger("PreTrigger", 0), 

	Start("Start", this, &SignatecDAQ::startAcquisition), 
	Stop("Stop", this, &SignatecDAQ::stopAcquisition), 
	SwitchOn("SwitchOn", this, &SignatecDAQ::switchOn), 
	SwitchOff("SwitchOff", this, &SignatecDAQ::switchOff), 

    _dirty(true), 
	_running(false), 
	_board(PX14_INVALID_HANDLE),
	dma_bufp(nullptr)
{
	PreTrigger.valueChanged += [this](int value)
	{
		if (_board) SetPreTriggerSamplesPX14 (_board, PreTrigger); 
	};
}

SignatecDAQ::~SignatecDAQ()
{
	if (_thread.joinable())
	{
		_running = false;
		_thread.join();
	}

	if (dma_bufp) {	FreeDmaBufferPX14(_board, dma_bufp); dma_bufp = nullptr; }
	if (_board != PX14_INVALID_HANDLE) { DisconnectFromDevicePX14(_board); _board = PX14_INVALID_HANDLE; }
}

void SignatecDAQ::accept(property_visitor &visit)
{
	visit(nChannels);
	visit(nScans);
	visit(nAlines);
	visit(UseVirtualDevice);
	visit(UseInternalTrigger);
	visit(PreTrigger);

	visit(Start);
	visit(Stop);
	visit(SwitchOn);
	visit(SwitchOff);
}
	
bool SignatecDAQ::initialize()
{
	int result;

	if (UseVirtualDevice)
	{
		// Connect to virtual(fake) device for test without real board	
		result = ConnectToVirtualDevicePX14(&_board, 0, 0); 
	}
	else
	{
		// Connect to and initialize the PX14400 device
		const int SerialNumber = 1; // 1 : first card
		result = ConnectToDevicePX14(&_board, SerialNumber);
	}

	if (result != SIG_SUCCESS)
	{
		dumpError(result, "Failed to initialize PX14400 device: ");
		return false;
	}

	// Set all hardware settings into a known state
	SetPowerupDefaultsPX14(_board);

	// Set dual channel acquisition
    if (nChannels == 2)
        SetActiveChannelsPX14(_board, PX14CHANNEL_DUAL);
    else if (nChannels == 1)
        SetActiveChannelsPX14(_board, PX14CHANNEL_ONE);
    else
    {
        cout << "Failed to initialize PX14400 device: wrong channel number " << nChannels << endl;
        return false;
    }

	// Set input voltage range
	SetInputVoltRangeCh1PX14(_board, DATASOURCE_VOLTAGE_RANGE);
	SetInputVoltRangeCh2PX14(_board, DATASOURCE_VOLTAGE_RANGE);

	// Internal Clock: Set acquisition rate : 170 Mhz
	SetAdcClockSourcePX14(_board, PX14CLKSRC_INT_VCO);
	result = SetInternalAdcClockRatePX14(_board, 340.0);
	
	// External Clock: Set acquisition rate : 170 Mhz
	// SetAdcClockSourcePX14(_board, PX14CLKSRC_EXTERNAL);
	// result = SetExternalClockRatePX14(_board, 340.0);

	if (SIG_SUCCESS != result)
	{
		dumpError(result, "Failed to initialize PX14400 device: ");
		return false;
	}

	// Trigger parameters
	if (UseInternalTrigger)
	{
		SetTriggerSourcePX14(_board, PX14TRIGSRC_INT_CH1); // Internal trigger: for debugging 
		SetTriggerLevelAPX14(_board, 33024); // This value is changed according to environment
	    SetTriggerModePX14(_board, PX14TRIGMODE_POST_TRIGGER);
	}
	else
	{
		SetTriggerSourcePX14(_board, PX14TRIGSRC_EXT); 
	    SetTriggerModePX14(_board, PX14TRIGMODE_SEGMENTED);
	    SetSegmentSizePX14(_board, nChannels * nScans);
	}

   	SetTriggerDirectionExtPX14(_board, PX14TRIGDIR_POS); 

	// Enable digital I/O
	SetDigitalIoEnablePX14(_board, TRUE);
	SetDigitalIoModePX14(_board, PX14DIGIO_OUT_0V);

	// Allocate a DMA buffer that will receive PCI acquisition data. By 
	//  allocating a DMA buffer, we can use the "fast" PX14400 library
	//  transfer routines for highest performance
	result = AllocateDmaBufferPX14(_board, getDataBufferSize() * 4, &dma_bufp); // 4 buffers

	if (SIG_SUCCESS != result)
	{
		dumpError(result, "Failed to initialize PX14400 device: ");
		return false;
	}

	return true;
}

void SignatecDAQ::startAcquisition()
{
	cout << "[SignatecDAQ] startAcquisition()" << endl;

    if (_dirty)
    {
        if (!initialize())
        {
            cout << "ERROR: failed to initialize device" << endl;
            return;
        }

        _dirty = false;
    }

	if (_thread.joinable())
	{
		cout << "ERROR: acquisition is already running." << endl;
		return;
	}

	_thread = std::thread(&SignatecDAQ::run, this);

	if (::SetThreadPriority(_thread.native_handle(), THREAD_PRIORITY_TIME_CRITICAL) == 0)
	{
		cout << "WARNING: failed to set acquisition thread priority: " << ::GetLastError() << endl;
	}
}

void SignatecDAQ::stopAcquisition()
{
	cout << "[SignatecDAQ] stopAcquisition()" << endl;

	if (_thread.joinable())
	{
		_running = false;
		_thread.join();
	}
}

void SignatecDAQ::switchOn()
{
	cout << "[SignatecDAQ] switchOn()" << endl;

	SetDigitalIoModePX14(_board, PX14DIGIO_OUT_3_3V);
}

void SignatecDAQ::switchOff()
{
	cout << "[SignatecDAQ] switchOff()" << endl;

	SetDigitalIoModePX14(_board, PX14DIGIO_OUT_0V);
}

// Acquisition Thread
void SignatecDAQ::run()
{
	int result;

	cout << "[SignatecDAQ] Start data acquisition thread." << endl;

	// Update pretrigger setting
	SetPreTriggerSamplesPX14 (_board, PreTrigger); 

	// Arm recording - Acquisition will begin when the PX14400 receives a trigger event.
	result = BeginBufferedPciAcquisitionPX14(_board);
	if (SIG_SUCCESS != result)
	{
		dumpError(result, "Failed to arm recording: ");
		return;
	}

	unsigned long long samples_recorded = 0;
	unsigned loop_counter = 0;
	px14_sample_t *cur_chunkp;

	ULONG dwTickStart = 0, dwTickLastUpdate;

	int frameIndex = 0;

	_running = true;
	while (_running)
	{
		// Determine where new data transfer data will go. 
		cur_chunkp = dma_bufp + (loop_counter % 4) * getDataBufferSize(); 

		// Start asynchronous DMA transfer of new data; this function starts
		//  the transfer and returns without waiting for it to finish. This
		//  gives us a chance to process the last batch of data in parallel
		//  with this transfer.
		result = GetPciAcquisitionDataFastPX14(_board, getDataBufferSize(), cur_chunkp, TRUE);

		if (SIG_SUCCESS != result)
		{
			dumpError(result, "Failed to obtain PCI acquisition data: ");
			break;
		}

		// Process previous chunk data while we're transfering to
		// loop_counter > 1000 : to prevent FIFO overflow
		// if ((loop_counter % 4 == 0 || loop_counter % 4 == 2) && loop_counter > 1000)
		if (loop_counter % 4 == 0 || loop_counter % 4 == 2)
		{
			px14_sample_t *prev_chunkp;

			if (loop_counter % 4 == 0)
				prev_chunkp = dma_bufp + 2 * getDataBufferSize(); // last half of buffer;
			else if (loop_counter % 4 == 2)
				prev_chunkp = dma_bufp; // first half of buffer

			// Callback
            np::Array<uint16_t, 2> frame(prev_chunkp, nChannels * nScans, nAlines);
			DidAcquireData(frameIndex++, frame);
		}

		// Wait for the asynchronous DMA transfer to complete so we can loop 
		//  back around to start a new one. Calling thread will sleep until
		//  the transfer completes

		while (true)
		{
			result = WaitForTransferCompletePX14(_board, 100); // No timeout
			if (result == SIG_SUCCESS)
				break;

			SwitchToThread();
			printf(".");
		}

		// result = WaitForTransferCompletePX14(_board, 100); // Timeout : 100 msec
		// result = WaitForTransferCompletePX14(_board); // No timeout

		if (SIG_SUCCESS != result)
		{
			dumpError(result, "Failed to WaitForTransferCompletePX14: ");
			break;
		}

		if (!dwTickStart) 
			dwTickStart = dwTickLastUpdate = GetTickCount();

		// Update counters
		samples_recorded += getDataBufferSize();
		loop_counter++;

		// Periodically update progress
		ULONG dwTickNow = GetTickCount();
		if (dwTickNow - dwTickLastUpdate > 5000)
		{
			double dRate;

			dwTickLastUpdate = dwTickNow;
			ULONG dwElapsed = dwTickNow - dwTickStart;

			if (dwElapsed)
			{
				dRate = (samples_recorded / 1000000.0) / (dwElapsed / 1000.0);

				unsigned h = 0, m = 0, s = 0;
				if (dwElapsed >= 1000)
				{
					if ((s = dwElapsed / 1000) >= 60)	// Seconds
					{
						if ((m = s / 60) >= 60)			// Minutes
						{
							if (h = m / 60)				// Hours
								m %= 60;
						}

						s %= 60;
					}
				}

				printf("Total data: %0.2f MS, Rate: %6.2f MS/s, Elapsed Time: %u:%02u:%02u\n",
					samples_recorded / 1000000.0, dRate, h, m, s);
			}
		}
	}

	// End the acquisition. Always do this since in ensures the board is 
	//  cleaned up properly
	EndBufferedPciAcquisitionPX14(_board);

	// Notify delegate to acquisition ended
	// DidStopAcquisition();

	cout << "[Signatec] Data acquisition thread is finished normally." << endl;
}

// Dump a PX14400 library error
void SignatecDAQ::dumpError(int res, LPCSTR pPreamble)
{
	char *pErr;
	int my_res;

	if (pPreamble)
		printf (pPreamble);

	pErr = NULL;
	my_res = GetErrorTextAPX14(res, &pErr, 0, _board);
	if ((SIG_SUCCESS == my_res) && pErr)
	{
		printf("%s\n", (LPCSTR)pErr);
		FreeMemoryPX14(pErr);
	}
	else
	{
        printf("(cannot obtain error message)\n");
	}
}