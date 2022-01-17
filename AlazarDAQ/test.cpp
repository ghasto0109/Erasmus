#include "AlazarDAQ.h"
#include <numcpp/array.h>

void test_acq_rate()
{
    const int RepeatCopy = 4;

	AlazarDAQ daq;
	daq.nScans = 640;
	daq.nAlines = 512;
	daq.AutoTrigger = true;

    np::Array<uint16_t, 2> temp(daq.nScans, daq.nAlines);
    daq.DidAcquireData += [&daq, &temp, RepeatCopy](int frame_count, uint16_t *frame)
    {
        for (int i = 0; i < RepeatCopy; i++)
            memcpy(temp.raw_ptr(), frame, np::byteSize(temp));
    };

    // Run acquisition until press enter key
    daq.Start();
    puts("Press enter to stop...");
    getchar();

    // Stop acquisition
    daq.Stop();
    puts("Press enter to exit...");
    getchar();
}

int main()
{
	test_acq_rate();
    return 0;
}