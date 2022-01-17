#ifndef SIGNATEC_DAQ_H_
#define SIGNATEC_DAQ_H_

#include <objcpp/property.h>
#include <numcpp/array.h>

#include <thread>

typedef struct _px14hs_* HPX14;

class SignatecDAQ
{
public:
	SignatecDAQ();
	virtual ~SignatecDAQ();

	void accept(property_visitor &visit);

	// properties
	property<int> nChannels, nScans, nAlines;
	property<bool> UseVirtualDevice, UseInternalTrigger; // for debugging purpose
	property<int> PreTrigger;

	// operations
	operation Start, Stop, SwitchOn, SwitchOff;

	// signals
	signal2<int, const np::Array<uint16_t, 2> &> DidAcquireData;

private:
	bool initialize();
	void startAcquisition();
	void stopAcquisition();
	void switchOn();
	void switchOff();

    bool _dirty;

	// thread
	std::thread _thread;
	bool _running;
	void run();

private:
	// PX14400 board driver handle
	HPX14 _board;

	// DMA buffer pointer to acquire data
	unsigned short *dma_bufp;

	// Data buffer size (2 channel, half size. merge two buffer to build 1 frame)
	int getDataBufferSize() { return nChannels * nScans * nAlines / 2; } 

	// Dump a PX14400 library error
	void dumpError(int res, const char *pPreamble);
};

#endif // SIGNATEC_DAQ_H_