#ifndef DATA_ACQUISITION_H_
#define DATA_ACQUISITION_H_

#include <objcpp/property.h>
#include <stdint.h>
#include <QtWidgets>

class SignatecDAQ;
class MemoryManager;
class AnalogOutput;

class MainWindow;
class DataBufferModel;

class DataAcquisition : public QObject
{
	Q_OBJECT

public:
    DataAcquisition(); 
	~DataAcquisition();

	void accept(property_visitor &visit);

	void initialize();

	bool isRecording() { return _isRecording; }
	void setRecording(bool isRecording);

	// operations
	operation StartAcq, StopAcq;
	operation StartRecording, StopRecording;
	operation CaptureOneFrame, RecordYScan;

    // object references
    MainWindow *_mainWindow;
	SignatecDAQ *_dataSource;
	AnalogOutput *_scanMirror;

signals:
	void signal_didFinishRecording();

private slots:
	void slot_didFinishRecording();

	// SignatecDAQ signals
	void didAcquireData(int tag, uint16_t *acquiredData);

private:
	void startAcq();
	void stopAcq();
	void startRecording();
	void stopRecording();
	void captureOneFrame();
	void recordYscan();

	// Recording
	bool _isRecording;
	DataBufferModel *_recordingDataBuffer;
	int _frameCountToRecord, _frameCountToDrop;
};

#endif // DATA_ACQUISITION_VIEW_H_