#include "stdafx.h"
#include "AnalogOutput2.h"

#include "NIDAQmx.h"
#include <fstream>

using namespace std;

AnalogOutput2::AnalogOutput2() : 
	_taskHandle(nullptr), 

	ScanPatternFile("ScanPatternFile"), 
	nAlines("nAlines", 1024), 
	
	Start("Start", this, &AnalogOutput2::start), 
	Stop("Stop", this, &AnalogOutput2::stop)
{
}

AnalogOutput2::~AnalogOutput2()
{
	stop();
}

void AnalogOutput2::accept(property_visitor &visit)
{
	visit(ScanPatternFile);
	visit(nAlines);

	visit(Start);
	visit(Stop);
}

#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else

int32 CVICALLBACK OnEveryNSamplesEvent (TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void *callbackData);
void 	GenerateBuffer (void);

const double Amp = 2.0;

void AnalogOutput2::start()
{
	// scan pattern
	_buffer.reset(new double[2 * bufferSize()]);

	fstream file(ScanPatternFile, ios::in || ios::binary);
	if (!file.is_open())
	{
		cout << "Error: cannot read scanpattern file: " << endl;
		return;
	}

	file.read((char *)_buffer.get(), 2 * bufferSize() * sizeof(double));
	file.close();

	/*********************************************/
	// DAQmx Configure Code
	/*********************************************/
	static int32   error=0;
	static char    errBuff[2048]={'\0'};

	DAQmxErrChk (DAQmxCreateTask("",&_taskHandle));
	DAQmxErrChk(DAQmxCreateAOVoltageChan(_taskHandle, "Dev1/ao0:1", "", -Amp, Amp, DAQmx_Val_Volts, NULL));
	DAQmxErrChk (DAQmxCfgSampClkTiming(_taskHandle, "", 300.0 * 10000, DAQmx_Val_Rising, DAQmx_Val_ContSamps, bufferSize()));

	// Disable generation to be warned if the output buffer is not refreshed fast enough
	// It prevents from generating discontinuity on the signal
	DAQmxErrChk (DAQmxSetWriteAttribute(_taskHandle, DAQmx_Write_RegenMode, DAQmx_Val_DoNotAllowRegen));

	// Register a callback on Every N Samples generated so that we can be trigged to refresh the output buffer.
	DAQmxErrChk (DAQmxRegisterEveryNSamplesEvent(_taskHandle, DAQmx_Val_Transferred_From_Buffer, bufferSize(), 0, OnEveryNSamplesEvent, this));

	// Double the size of the output buffer to provide enough sample at the generation start
	// (usefull for high an update rate)
	DAQmxCfgOutputBuffer(_taskHandle, 2*bufferSize());
	
	/*********************************************/
	// DAQmx Write Code
	/*********************************************/
	GenerateBuffer();
	DAQmxErrChk (DAQmxWriteAnalogF64(_taskHandle, bufferSize(), 0, 10.0, DAQmx_Val_GroupByChannel, _buffer.get(), NULL, NULL));
	GenerateBuffer();
	DAQmxErrChk (DAQmxWriteAnalogF64(_taskHandle, bufferSize(), 0, 10.0, DAQmx_Val_GroupByChannel, _buffer.get(), NULL, NULL));

	/*********************************************/
	// DAQmx Start Code
	/*********************************************/
	DAQmxErrChk (DAQmxStartTask(_taskHandle));

	return;

Error:
	if (DAQmxFailed(error))
		DAQmxGetExtendedErrorInfo(errBuff, 2048);

	if (_taskHandle != 0) 
	{
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		DAQmxStopTask(_taskHandle);
		DAQmxClearTask(_taskHandle);

		_taskHandle = nullptr;
	}
	
	if (DAQmxFailed(error))
		printf("DAQmx Error: %s\n",errBuff);
}

void AnalogOutput2::stop()
{
	if (_taskHandle != 0) 
	{
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		DAQmxStopTask(_taskHandle);
		DAQmxClearTask(_taskHandle);

		_taskHandle = nullptr;
	}
}

/*********************************************/
// OnEveryNSamplesEvent
/*********************************************/

int32 CVICALLBACK OnEveryNSamplesEvent (TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void *callbackData)
{
	((AnalogOutput2 *)callbackData)->callback();

	return 0;	
}

void AnalogOutput2::callback()
{
	static int32   	error=0;
	static char    	errBuff[2048]={'\0'};
	static int32	NbOfSamplesWritten=0, totalNbOfSamplesWritten=0;

	/*********************************************/
	// DAQmx Write Code
	/*********************************************/
	if (_taskHandle != 0)
	{
		GenerateBuffer();	
		  
		DAQmxErrChk (DAQmxWriteAnalogF64(_taskHandle, bufferSize(), 0, 10.0, DAQmx_Val_GroupByChannel, _buffer.get(), &NbOfSamplesWritten, NULL));
		totalNbOfSamplesWritten += NbOfSamplesWritten;
		//printf("%d samples written per channel.\n", totalNbOfSamplesWritten);
	}
	
Error:
	if (DAQmxFailed(error))
	{
		DAQmxGetExtendedErrorInfo(errBuff,2048);
		printf("DAQmx Error: %s\n",errBuff); 

		DAQmxClearTask(_taskHandle);
		_taskHandle = nullptr;
	}
}

/*********************************************/
// Waveform generation
/*********************************************/

void GenerateBuffer (void)
{
}