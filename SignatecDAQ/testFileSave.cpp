#include "SignatecDAQ.h"

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

template <typename T>
class Queue
{
public:
    T pop()
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        while (queue_.empty())
        {
            cond_.wait(mlock);
        }
        auto item = queue_.front();
        queue_.pop();
        return item;
    }

    void pop(T& item)
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        while (queue_.empty())
        {
            cond_.wait(mlock);
        }
        item = queue_.front();
        queue_.pop();
    }

    void push(const T& item)
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        queue_.push(item);
        mlock.unlock();
        cond_.notify_one();
    }

    void push(T&& item)
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        queue_.push(std::move(item));
        mlock.unlock();
        cond_.notify_one();
    }

private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cond_;
};

#include <Windows.h>

const int nScans = 1300;
const int nAlines = 1024;

const char *FilePath = "F:\\ssd_test.dat";

class FileWriter
{
public:
    FileWriter();
    ~FileWriter();

    void write(const uint16_t *frame);

private:
    // file
    HANDLE hFile;

	// thread
	std::thread _thread;
	bool _running;
	void run();

    // queue
    std::queue<uint16_t *> _bufferPool;
    std::mutex _bufferPoolLock;
    Queue<uint16_t *> _writeQueue;
};

using namespace std;

FileWriter::FileWriter() : hFile(INVALID_HANDLE_VALUE)
{
	hFile = ::CreateFile(FilePath, GENERIC_WRITE, FILE_SHARE_READ, 
		NULL, CREATE_ALWAYS, FILE_FLAG_NO_BUFFERING | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        cout << "ERROR: Failed to open file" << endl;
        return;
    }

    for (int i = 0; i < 2048; i++)
    {
        uint16_t *buffer = new uint16_t[2 * nScans * nAlines];
        memset(buffer, 0, 2 * nScans * nAlines * sizeof(uint16_t));
        _bufferPool.push(buffer);
    }

	_thread = std::thread(&FileWriter::run, this);
}

FileWriter::~FileWriter()
{
    if (_thread.joinable())
    {
		_running = false;
        _writeQueue.push(nullptr);

		_thread.join();
    }

    CloseHandle(hFile);

    while (!_bufferPool.empty())
    {
        uint16_t *buffer = _bufferPool.front();
        _bufferPool.pop();
        delete[] buffer;
    }
}

void FileWriter::write(const uint16_t *frame)
{
    uint16_t *buffer = nullptr;

    {
        std::unique_lock<std::mutex> lock(_bufferPoolLock);

        buffer = _bufferPool.front();
        _bufferPool.pop();
    }

    memcpy(buffer, frame, 2 * nScans * nAlines * sizeof(uint16_t));

    _writeQueue.push(buffer);
}

void FileWriter::run()
{
	unsigned long long samples_recorded = 0, samples_per_update = 0;
	unsigned loop_counter = 0;

	ULONG dwTickStart = 0, dwTickLastUpdate;

	_running = true;
    while (_running)
    {
        uint16_t *buffer = _writeQueue.pop();
        if (!_running || buffer == nullptr)
            break;

        DWORD dwBytesWrote;
        WriteFile(hFile, buffer, 2 * nScans * nAlines * sizeof(uint16_t), &dwBytesWrote, NULL);

        {
            std::unique_lock<std::mutex> lock(_bufferPoolLock);
            _bufferPool.push(buffer);
        }

		// Update counters
		if (!dwTickStart) 
			dwTickStart = dwTickLastUpdate = GetTickCount();
        
        samples_recorded += 2 * nScans * nAlines;
        samples_per_update += 2 * nScans * nAlines;
		loop_counter++;

		// Periodically update progress
		ULONG dwTickNow = GetTickCount();
		if (dwTickNow - dwTickLastUpdate > 2000)
		{
			double dRate;

			ULONG dwTotalElapsed = dwTickNow - dwTickStart;
            ULONG dwElapsed = dwTickNow - dwTickLastUpdate;
			dwTickLastUpdate = dwTickNow;

            // Rate calculation per update
            dRate = (samples_per_update / 1000000.0) / (dwElapsed / 1000.0);

            printf("[FileWriter] Total data: %0.2f MS, Rate: %6.2f MS/s, Pool size: %u\n",
                samples_recorded / 1000000.0, dRate, _bufferPool.size());

            // reset
            samples_per_update = 0;
		}
    }
}

void test_fwrite()
{
    FileWriter writer;

    SignatecDAQ daq;

    daq.nScans = nScans;
    daq.nAlines = nAlines;
    daq.UseInternalTrigger = true;

    uint16_t *buffer = new uint16_t[2 * nScans * nAlines];
    daq.DidAcquireData += [&daq, &writer, buffer](int frame_count, const np::Array<uint16_t, 2> &frame)
    {
        memcpy(buffer, frame.raw_ptr(), 2 * nScans * nAlines * sizeof(uint16_t));
        writer.write(buffer);
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