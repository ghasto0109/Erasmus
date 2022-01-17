#include "SignatecDAQ.h"
#include <numcpp/array.h>

void test_acq_rate()
{
    // buffer size = 1MB
    const int nScans = 1024;
    const int nAlines = 1024;

    const int RepeatCopy = 5;

    SignatecDAQ daq;

    daq.nScans = nScans;
    daq.nAlines = nAlines;
    daq.UseInternalTrigger = true;

    np::Array<uint16_t, 2> temp(2 * daq.nScans, daq.nAlines);
    daq.DidAcquireData += [&temp, RepeatCopy](int frame_count, const np::Array<uint16_t, 2> &frame)
    {
        for (int i = 0; i < RepeatCopy; i++)
            memcpy(temp.raw_ptr(), frame.raw_ptr(), np::byteSize(temp));
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

void test_fwrite();

int main()
{
    test_acq_rate();
    //test_fwrite();

    return 0;
}