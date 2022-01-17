#include "AnalogOutput.h"

#include "NIDAQmx.h"
#include <fstream>

using namespace std;

AnalogOutput::AnalogOutput() : 
	_taskHandle(nullptr), 

    nChannels("nChannels", 3), 
	nAlines("nAlines", 1024), 
	nFrames1("nFrames1", 1024), 
	ScanPatternFile1("ScanPatternFile1"), 
	nFrames2("nFrames2", 1024), 
	ScanPatternFile2("ScanPatternFile2"), 
	Amp("Amp", 2.0f), 
	ClockPort("ClockPort", "/Dev1/PFI1"), 
    ClockFrequency("ClockFrequency",500),

	Start("Start", this, &AnalogOutput::start1), 
	StartScan("Start Scan", this, &AnalogOutput::start2), 
	ResetZero("Reset to Zero Volt", this, &AnalogOutput::resetZero), 
	Stop("Stop", this, &AnalogOutput::stop), 
	UseScanPattern1("Use scan pattern #1", this, &AnalogOutput::useScanPattern1), 
	UseScanPattern2("Use scan pattern #2", this, &AnalogOutput::useScanPattern2)
{
}

AnalogOutput::~AnalogOutput()
{
	stop();
}

void AnalogOutput::accept(property_visitor &visit)
{
	visit(nChannels);
	visit(Amp);
	visit(nAlines);
	visit(nFrames1);
	visit(ScanPatternFile1);
	visit(nFrames2);
	visit(ScanPatternFile2);

	visit(Start);
	visit(StartScan);
    visit(ResetZero);
    visit(Stop);
	visit(UseScanPattern1);
	visit(UseScanPattern2);
}

#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else

int32 CVICALLBACK DoneCallback(TaskHandle taskHandle, int32 status, void *callbackData);

void AnalogOutput::start(bool useScanPattern1 = true)
{
	int32       error=0;
	char        errBuff[2048]={'\0'};

	/*********************************************/
	// DAQmx Configure Code
	/*********************************************/
	DAQmxErrChk (DAQmxCreateTask("",&_taskHandle));

    if (nChannels == 2)
    {
        // First channel
        DAQmxErrChk(DAQmxCreateAOVoltageChan(_taskHandle, "Dev1/ao0", "", -Amp, Amp, DAQmx_Val_Volts, NULL));

        // Second channel
        DAQmxErrChk(DAQmxCreateAOVoltageChan(_taskHandle, "Dev1/ao1", "", -Amp, Amp, DAQmx_Val_Volts, NULL));
    }
    else if (nChannels == 3)
    {
        // First channel
        DAQmxErrChk(DAQmxCreateAOVoltageChan(_taskHandle, "Dev1/ao0", "", -Amp, Amp, DAQmx_Val_Volts, NULL));

        // Second channel
        DAQmxErrChk(DAQmxCreateAOVoltageChan(_taskHandle, "Dev1/ao1", "", -Amp, Amp, DAQmx_Val_Volts, NULL));

        // Third channel
        DAQmxErrChk(DAQmxCreateAOVoltageChan(_taskHandle, "Dev1/ao2", "", -Amp, Amp, DAQmx_Val_Volts, NULL));
    }
    else
    {
        cout << "ERROR: AnalogOutput supports only 2 or 3 channels." << endl;
        return;
    }

	// Clock
	// DAQmxErrChk (DAQmxCfgSampClkTiming(_taskHandle,"/Dev1/PFI0",240*1000.0,DAQmx_Val_Rising,DAQmx_Val_ContSamps,1000));
	
	// External clock
	DAQmxErrChk(DAQmxCfgSampClkTiming(_taskHandle, ClockPort.get().c_str(), ClockFrequency * 1000., DAQmx_Val_Rising, DAQmx_Val_ContSamps, nAlines*nFrames1));
	
	// Internal clock
    //DAQmxErrChk(DAQmxCfgSampClkTiming(_taskHandle, "", 100 * 1000.0, DAQmx_Val_Rising, DAQmx_Val_ContSamps, nAlines*nFrames1));

    // Register event
	DAQmxErrChk (DAQmxRegisterDoneEvent(_taskHandle,0,DoneCallback,NULL));

	if (useScanPattern1)
		updateScanPattern(ScanPatternFile1, nFrames1);
	else
		updateScanPattern(ScanPatternFile2, nFrames2);

	/*********************************************/
	// DAQmx Start Code
	/*********************************************/
	DAQmxErrChk (DAQmxStartTask(_taskHandle));

	return;

Error:
	if (DAQmxFailed(error))
		DAQmxGetExtendedErrorInfo(errBuff,2048);

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

void AnalogOutput::resetZero()
{
    int32       error = 0;
    char        errBuff[2048] = { '\0' };

    // TODO: do not initialize again

    /*********************************************/
    // DAQmx Configure Code
    /*********************************************/
    DAQmxErrChk(DAQmxCreateTask("", &_taskHandle));

    if (nChannels == 2)
    {
        // First channel
        DAQmxErrChk(DAQmxCreateAOVoltageChan(_taskHandle, "Dev1/ao0", "", -Amp, Amp, DAQmx_Val_Volts, NULL));

        // Second channel
        DAQmxErrChk(DAQmxCreateAOVoltageChan(_taskHandle, "Dev1/ao1", "", -Amp, Amp, DAQmx_Val_Volts, NULL));
    }
    else if (nChannels == 3)
    {
        // First channel
        DAQmxErrChk(DAQmxCreateAOVoltageChan(_taskHandle, "Dev1/ao0", "", -Amp, Amp, DAQmx_Val_Volts, NULL));

        // Second channel
        DAQmxErrChk(DAQmxCreateAOVoltageChan(_taskHandle, "Dev1/ao1", "", -Amp, Amp, DAQmx_Val_Volts, NULL));

        // Third channel
        DAQmxErrChk(DAQmxCreateAOVoltageChan(_taskHandle, "Dev1/ao2", "", -Amp, Amp, DAQmx_Val_Volts, NULL));
    }
    else
    {
        cout << "ERROR: AnalogOutput supports only 2 or 3 channels." << endl;
        return;
    }

    // Clock
    // DAQmxErrChk (DAQmxCfgSampClkTiming(_taskHandle,"/Dev1/PFI0",240*1000.0,DAQmx_Val_Rising,DAQmx_Val_ContSamps,1000));

    // Internal clock
    DAQmxErrChk(DAQmxCfgSampClkTiming(_taskHandle, "", 100 * 1000.0, DAQmx_Val_Rising, DAQmx_Val_ContSamps, nAlines*nFrames1));

    // Register event
    DAQmxErrChk(DAQmxRegisterDoneEvent(_taskHandle, 0, DoneCallback, NULL));

    // Write zero to buffer
    
    // this functions doesn't work
    // DAQmxWriteAnalogScalarF64(_taskHandle, false, 10.0, 0.0, NULL);

    double *scan = new double[nChannels * nFrames1 * nAlines];
    memset(scan, 0, nChannels * nFrames1 * nAlines * sizeof(double));

    puts("WriteAnalogF64 Start");
    DAQmxErrChk(DAQmxWriteAnalogF64(_taskHandle, nFrames1*nAlines, 0, 10.0, DAQmx_Val_GroupByChannel, scan, NULL, NULL));
    puts("WriteAnalogF64 Stop");

    delete[] scan;

    /*********************************************/
    // DAQmx Start Code
    /*********************************************/
    DAQmxErrChk(DAQmxStartTask(_taskHandle));

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
        printf("DAQmx Error: %s\n", errBuff);
}

void AnalogOutput::stop()
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

void AnalogOutput::useScanPattern1()
{
	cout << "[AnalogOutput] scan pattern #1" << endl;
	updateScanPattern(ScanPatternFile1, nFrames1);
}

void AnalogOutput::useScanPattern2()
{
	cout << "[AnalogOutput] scan pattern #2" << endl;
	updateScanPattern(ScanPatternFile2, nFrames2);
}

// FIXME: read scan file size
void AnalogOutput::updateScanPattern(const std::string &filename, int nFrames)
{
	int32       error=0;
	char        errBuff[2048]={'\0'};

	/*********************************************/
	// DAQmx Write Code
	/*********************************************/
	double *scan = new double[nChannels * nFrames * nAlines];

	fstream file(filename, ios::in | ios::binary);
	if (!file.is_open())
	{
		cout << "Error: cannot open scanpattern file!" << endl;
		return;
	}

    file.read((char *)scan, nChannels * nFrames * nAlines * sizeof(double));
	file.close();

	puts("WriteAnalogF64 Start");
	DAQmxErrChk (DAQmxWriteAnalogF64(_taskHandle, nFrames*nAlines, 0, 10.0, DAQmx_Val_GroupByChannel, scan, NULL, NULL));
	puts("WriteAnalogF64 Stop");

	delete[] scan;

	return;

Error:
	if (DAQmxFailed(error))
		DAQmxGetExtendedErrorInfo(errBuff,2048);

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

int32 CVICALLBACK DoneCallback(TaskHandle taskHandle, int32 status, void *callbackData)
{
	int32   error=0;
	char    errBuff[2048]={'\0'};

	// Check to see if an error stopped the task.
	DAQmxErrChk (status);

Error:
	if( DAQmxFailed(error) ) {
		DAQmxGetExtendedErrorInfo(errBuff,2048);
		DAQmxClearTask(taskHandle);
		printf("DAQmx Error: %s\n",errBuff);
	}
	return 0;
}