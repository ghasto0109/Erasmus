#include "AnalogOutput.h"

void test_analog_output()
{
    AnalogOutput ao;

    ao.nAlines = 512;
    ao.nFrames1 = 8;
    ao.ScanPatternFile1 = "scanpattern_yfix.bin";
    ao.nFrames2 = 512;
    ao.ScanPatternFile2 = "scanpattern_yscan.bin";
    ao.Amp = 5.0f;

    // Start with scan pattern 1 (fix)
    ao.Start();
    puts("Press enter to start y scan...");
    getchar();
    ao.Stop();

    // Start with scan pattern 2 (scan)
    ao.StartScan();
    puts("Press enter to stop...");
    getchar();
    ao.Stop();
}

void test_analog_output_alazar()
{
    AnalogOutput ao;

    // important: Alazar system use 3 channels, two for galvanometer, 1 for switch
    ao.nChannels = 3; 

    ao.nAlines = 1024;
    ao.nFrames1 = 8;
    ao.ScanPatternFile1 = "scanpattern_alazar_yfix.bin";
    ao.nFrames2 = 8;
    ao.ScanPatternFile2 = "scanpattern_alazar_yfix.bin";
    ao.Amp = 5.0f;

    // Start with scan pattern 1 (fix)
    ao.Start();
    puts("Press enter to reset zero...");
    getchar();
    ao.Stop();

    ao.ResetZero();
    puts("Press enter to stop...");
    getchar();
    ao.Stop();
}

int main()
{
	// test_analog_output();
    test_analog_output_alazar();
    return 0;
}