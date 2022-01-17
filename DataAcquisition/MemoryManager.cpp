#include "stdafx.h"
#include "MemoryManager.h"
#include "moc_MemoryManager.cpp"

#include <ipps.h>

MemoryManager::MemoryManager() : 
	nChannels("nChannels", 2), 
	nScans("nScans", 1190), 
	nAlines("nAlines", 1024), 
	FrameCountToAllocate("FrameCountToAllocate", 400), 

	_allocationThread(NULL)
{
}

MemoryManager::~MemoryManager()
{
	if (_allocationThread)
	{
		if (_allocationThread->isRunning())
		{
			_allocationThread->stop();

			if (false == _allocationThread->wait(3000))
			{
				_allocationThread->terminate();
				printf("## MemoryManager: allocation thread has finished by force.\n");
			}
		}

		delete _allocationThread;
		_allocationThread = NULL;
	}
}

void MemoryManager::accept(property_visitor &visit)
{
	visit(nScans);
	visit(nAlines);
	visit(FrameCountToAllocate);
}

void MemoryManager::initialize()
{
	if (_allocationThread)
	{
		if (_allocationThread->isRunning())
			return; // Already running

		delete _allocationThread;
		_allocationThread = NULL;
	}

	_allocationThread = new MemoryAllocationThread(this);
	_allocationThread->start();
}

//uint16_t *MemoryManager::allocFrame()
uint8_t *MemoryManager::allocFrame()
{
	if (getAvailableFrameCount() == 0)
	{
		emit managerDidGotMemoryFull();
		return NULL;
	}

	//uint16_t *result = _frameBuffers.front();
	uint8_t *result = _frameBuffers.front();
	_frameBuffers.pop();

	emit managerDidUpdateMemoryCapacity(getAllocatedFrameCount());

	return result;
}

//void MemoryManager::freeFrame(Ipp16u *frame)
void MemoryManager::freeFrame(Ipp8u *frame)
{
	_frameBuffers.push(frame);

	emit managerDidUpdateMemoryCapacity(getAllocatedFrameCount());
}

MemoryManager::MemoryAllocationThread::MemoryAllocationThread(MemoryManager *manager) :
	_manager(manager), 
	_isRunning(false)
{
}

void MemoryManager::MemoryAllocationThread::run()
{
	_isRunning = true;

	for (int count = 0; count < _manager->FrameCountToAllocate; count++)
	{
		if (false == _isRunning)
			break;

		//Ipp16u *frameBuffer = ippsMalloc_16u(_manager->sampleCountInFrame());
		Ipp8u *frameBuffer = ippsMalloc_8u(_manager->sampleCountInFrame());
		if (NULL == frameBuffer)
		{
			printf("## MemoryManager: Failed to allocate memory.\n");
			emit _manager->managerDidFailMemoryAllocation();

			_isRunning = false;
			return;
		}

		// memset to allocate memory but only page
		//ippsSet_16s(0, (Ipp16s *)frameBuffer, _manager->sampleCountInFrame());
		ippsSet_8u(0, (Ipp8u *)frameBuffer, _manager->sampleCountInFrame());

		_manager->_frameBuffers.push(frameBuffer);

		if (count % 50 == 0)
			emit _manager->managerDidUpdateMemoryAllocationProgress(count + 1, _manager->FrameCountToAllocate);
	}

	if (_isRunning) // if not stopped by flag
	{
		emit _manager->managerDidUpdateMemoryAllocationProgress(_manager->FrameCountToAllocate, _manager->FrameCountToAllocate);
		emit _manager->managerDidFinishMemoryAllocation(_manager->FrameCountToAllocate);
		emit _manager->managerDidUpdateMemoryCapacity(_manager->getAllocatedFrameCount());
	}

	_isRunning = false;
}
