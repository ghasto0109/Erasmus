#ifndef ALAZAR_DAQ_H_
#define ALAZAR_DAQ_H_

#include <objcpp/property.h>
#include <objcpp/signal.h>
#include <numcpp/array.h>

#include <array>
#include <thread>

typedef void * HANDLE;

// Select the number of DMA buffers to allocate.
const int BUFFER_COUNT = 4;

class AlazarDAQ
{
public:
    AlazarDAQ();
	~AlazarDAQ();

	void accept(property_visitor &visit);

	// alazar ī�忡�� �о���� ä�� ���� (1 = A, 2 = A, B)
    property<int> nChannels;

    // �� trigger �� ���� ��� sample count
	property<int> nScans;

	// �� �� acquisition ���� ��� recording ���� 
	// (= trigger hit count, image width)
	property<int> nAlines;

    // �ܺ� Ŭ�� ��ȣ�� ���ø�
    property<bool> ExternalClock;

	// AutoTrigger �� �����Ǹ� trigger timeout �� �����ؼ� 
	// trigger ��ȣ�� ��� acquisition �� �� �ֵ��� �Ѵ�. 
	// laser �� ���� �ʰ� noise �� �޾ƺ��� ���� ����� �뵵. 
	property<bool> AutoTrigger;

    // Trigger ��ġ ����
    property<int> TriggerDelay;

	// operations
	operation Start, Stop;

	// signals
	//signal2<int, const np::Array<uint16_t, 2> &> DidAcquireData;
	signal2<int, const np::Array<uint8_t, 2> &> DidAcquireData;

	bool saveData;

private:
	bool init();
	void start();
	void stop();

	// thread
	std::thread _thread;
	bool _running;
	void run();

    // Is initialized?
    bool _dirty;

	// Handle of Alazar board
	HANDLE boardHandle;

	// Array of buffer pointers
	std::array<uint16_t *, BUFFER_COUNT> BufferArray;
};

#if 0

class SignatecDAQ
{
public:
	SignatecDAQ();
	virtual ~SignatecDAQ();

    void accept(property_visitor &visit);

	// properties
	property<int> nScans, nAlines;
	property<bool> UseVirtualDevice, UseInternalTrigger; // for debugging purpose
    property<int> InputVoltageRange; // 1: 0.220V ~ 25: 3.487V
	property<int> PreTrigger;

	// operations
	operation Start, Stop, SwitchOn, SwitchOff;

	// signals
	signal2<int, uint16_t *> DidAcquireData;

private:
	bool init();
	void start();
	void stop();
	void switchOn();
	void switchOff();

	// thread
	std::thread _thread;
	bool _running;
	void run();

    // Is initialized?
    bool _dirty;

	// Data buffer size (2 channel, half size. merge two buffer to build 1 frame)
	inline int getDataBufferSize() { return 2 * nScans * nAlines / 2; } 

	// PX14400 board driver handle
	HPX14 _board;

	// DMA buffer pointer to acquire data
	unsigned short *dma_bufp;
};

#endif

#endif // ALAZAR_DAQ_H_