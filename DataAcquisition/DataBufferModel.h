#ifndef DATA_BUFFER_MODEL_H_
#define DATA_BUFFER_MODEL_H_

class MemoryManager;

class DataBufferModel
{
public:
	DataBufferModel(MemoryManager *memoryManager);
	~DataBufferModel();

public:
	//bool addFrame(const uint16_t *frameBuffer);
	//uint16_t *getFrame(int frame);
	bool addFrame(const uint8_t *frameBuffer);
	uint8_t *getFrame(int frame);
	size_t getFrameCount() { return _frameBuffers.size(); }

private:
	MemoryManager *_memoryManager;
	//std::vector<uint16_t *> _frameBuffers;
	std::vector<uint8_t *> _frameBuffers;
};

#endif // DATA_BUFFER_MODEL_H_