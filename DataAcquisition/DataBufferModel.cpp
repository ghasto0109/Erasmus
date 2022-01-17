#include "stdafx.h"
#include "DataBufferModel.h"

#include "MemoryManager.h"
#include <ipps.h>

DataBufferModel::DataBufferModel(MemoryManager *memoryManager) :
	_memoryManager(memoryManager)
{
}

DataBufferModel::~DataBufferModel()
{
	//for (std::vector<uint16_t *>::iterator i = _frameBuffers.begin(); i != _frameBuffers.end(); ++i)
	//{
	//	uint16_t *frameBuffer = *i;
	//	_memoryManager->freeFrame(frameBuffer);
	//}
	for (std::vector<uint8_t *>::iterator i = _frameBuffers.begin(); i != _frameBuffers.end(); ++i)
	{
		uint8_t *frameBuffer = *i;
		_memoryManager->freeFrame(frameBuffer);
	}
}

//uint16_t *DataBufferModel::getFrame(int frame)
uint8_t *DataBufferModel::getFrame(int frame)
{
	if (frame < _frameBuffers.size())
		return _frameBuffers[frame];
	else
		return NULL; 
}

// Commentted out by MH
//bool DataBufferModel::addFrame(const uint16_t *frameBuffer)
//{
//	uint16_t *allocatedBuffer = _memoryManager->allocFrame();
//	if (NULL == allocatedBuffer)
//		return false;
//
//	// memcpy seem to be slow with >50GB memory
//	// memcpy(allocatedBuffer, frameBuffer, _memoryManager->sampleCountInFrame() * sizeof(uint16_t));
//	ippsCopy_16s((Ipp16s *)frameBuffer, (Ipp16s *)allocatedBuffer, _memoryManager->sampleCountInFrame());
//	_frameBuffers.push_back(allocatedBuffer);
//
//	return true;
//}

// Added by MH
bool DataBufferModel::addFrame(const uint8_t *frameBuffer)
{
	uint8_t *allocatedBuffer = _memoryManager->allocFrame();
	if (NULL == allocatedBuffer)
		return false;

	// memcpy seem to be slow with >50GB memory
	// memcpy(allocatedBuffer, frameBuffer, _memoryManager->sampleCountInFrame() * sizeof(uint16_t));
	ippsCopy_8u((Ipp8u *)frameBuffer, (Ipp8u *)allocatedBuffer, _memoryManager->sampleCountInFrame());
	_frameBuffers.push_back(allocatedBuffer);

	return true;
}
