#ifndef MEMORY_MANAGER_H_
#define MEMORY_MANAGER_H_

#include <objcpp/property.h>
#include <queue>
#include <QtCore>

class MemoryManager : public QObject
{
	Q_OBJECT

public:
	MemoryManager();
	~MemoryManager();

	void accept(property_visitor &visit);

	// properties
	::property<int> nChannels, nScans, nAlines;
	::property<int> FrameCountToAllocate;

	// ...
	void initialize();

	//uint16_t *allocFrame();
	uint8_t *allocFrame();
	//void freeFrame(uint16_t *frame);
	void freeFrame(uint8_t *frame);

	int getAvailableFrameCount() { return (int)_frameBuffers.size(); }
	int getAllocatedFrameCount() { return FrameCountToAllocate - getAvailableFrameCount(); }
	//int sampleCountInFrame() { return nChannels * nScans * nAlines; }
	int sampleCountInFrame() { return nChannels * nScans * nAlines * 3 / 2; }

signals:
	void managerDidUpdateMemoryAllocationProgress(int currentFrameCount, int frameCountToAllocate);
	void managerDidFinishMemoryAllocation(int allocatedFrameCount);
	void managerDidFailMemoryAllocation();
	void managerDidUpdateMemoryCapacity(int availableFrameCount);
	void managerDidGotMemoryFull(); 

private:
	//std::queue<uint16_t *> _frameBuffers;
	std::queue<uint8_t *> _frameBuffers;

	class MemoryAllocationThread : public QThread
	{
	public:
		MemoryAllocationThread(MemoryManager *manager);
		bool isRunning() { return _isRunning; }
		void stop() { _isRunning = false; }

	protected:
		void run();

	private:
		MemoryManager *_manager;
		bool _isRunning;
	};

	MemoryAllocationThread *_allocationThread;
};

#endif // MEMORY_MANAGER_H_