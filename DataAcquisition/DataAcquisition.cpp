#include "stdafx.h"
#include "DataAcquisition.h"

#include "DataBufferModel.h"
#include "RecordResultView.h"

#include "SignatecDAQ/SignatecDAQ.h"
#include "AnalogOutput.h"
#include "AnalogOutput2.h"
#include "MemoryManager.h"
#include "CameraCapture.h"
#include "ZabberStage.h"

#include "Erasmus/MainWindow.h"
#include "Erasmus/ui_MainWindow.h"

using namespace std;

DataAcquisition::DataAcquisition() : 
	StartAcq("Start Acquisition", this, &DataAcquisition::startAcq), 
	StopAcq("Stop Acquisition", this, &DataAcquisition::stopAcq), 
	StartRecording("Start Recording", this, &DataAcquisition::startRecording), 
	StopRecording("Stop Recording", this, &DataAcquisition::stopRecording), 
	CaptureOneFrame("Capture One Frame", this, &DataAcquisition::captureOneFrame), 
	RecordYScan("Record Single Y Scan", this, &DataAcquisition::recordYscan), 

    _dataSource(nullptr), 
	_scanMirror(nullptr), 

    _mainWindow(nullptr), 

	_isRecording(false), 
	_recordingDataBuffer(nullptr), 
	_frameCountToRecord(-1), 
	_frameCountToDrop(0)
{
}

DataAcquisition::~DataAcquisition()
{
	//if (isConnected())
	//	setConnected(false);

	if (_recordingDataBuffer) { delete _recordingDataBuffer; _recordingDataBuffer = nullptr; }
}

void DataAcquisition::accept(property_visitor &visit)
{
	visit(StartAcq);
	visit(StopAcq);
	visit(StartRecording);
	visit(StopRecording);
	visit(CaptureOneFrame);
	visit(RecordYScan);
}

void DataAcquisition::initialize()
{
	using namespace std::placeholders;

	// Initialize data acquiring device
	// _dataSource = _context.getObject<SignatecDAQ>("dataSource");
	_dataSource->DidAcquireData += std::bind(&DataAcquisition::didAcquireData, this, _1, _2);
	
	if (!_dataSource->initialize())
		QMessageBox::critical(NULL, "Error", _dataSource->getErrorMessage().c_str());

	// Initialize scan mirror
	// _scanMirror = _context.getObject<AnalogOutput>("scanMirror");

	// Use Qt signal-slot to call function across threads. 
	connect(this, SIGNAL(signal_didFinishRecording()), this, SLOT(slot_didFinishRecording()));
}

// Operations

void DataAcquisition::startAcq()
{
	// start first
	_dataSource->Start();

#ifdef USE_NI_DAQ
	auto *scanMirror = _context.getObject<AnalogOutput>("scanMirror");
	scanMirror->Start();
#endif

	// wait
	::Sleep(500);

	// switch on later
	_dataSource->SwitchOn();

	//// start camera if exists
	//auto *cameraCapture = _context.getObject<CameraCapture>("cameraCapture");
	//if (cameraCapture)
	//	cameraCapture->Start();
}

void DataAcquisition::stopAcq()
{
	// stop and switch off
	_dataSource->Stop();

#ifdef USE_NI_DAQ
	auto *scanMirror = _context.getObject<AnalogOutput>("scanMirror");
	scanMirror->Stop();
#endif

	_dataSource->SwitchOff();

	//// stop camera if exists
	//auto *cameraCapture = _context.getObject<CameraCapture>("cameraCapture");
	//if (cameraCapture)
	//	cameraCapture->Stop();
}

void DataAcquisition::startRecording()
{
	if (isRecording())
	{
		cout << "Error: Recording is already in progress." << endl;
		return;
	}

	setRecording(true);
}

void DataAcquisition::stopRecording()
{
	if (!isRecording())
	{
		cout << "Error: Recording is not in progress." << endl;
		return;
	}

	setRecording(false);
}

void DataAcquisition::captureOneFrame()
{
	_frameCountToRecord = 1;
	setRecording(true);
}

void DataAcquisition::recordYscan()
{
	// Stop all
	_scanMirror->Stop();
	_dataSource->Stop();
	_dataSource->SwitchOff();

	// Start recording
	_frameCountToDrop = 0;
	_frameCountToRecord = _scanMirror->nFrames2;

	setRecording(true);

	// Start device first
	_scanMirror->StartScan();
	_dataSource->Start(); // with scan pattern 2

	// wait
	::Sleep(500);

	// switch on later
	_dataSource->SwitchOn();
}

void DataAcquisition::setRecording(bool isRecording)
{
	if (_isRecording == isRecording)
		return;

	if (isRecording)
	{
		// Allocate data buffer model
		if (_recordingDataBuffer) { delete _recordingDataBuffer; _recordingDataBuffer = NULL; }
		_recordingDataBuffer = new DataBufferModel(_mainWindow->_memoryManager.get());

		// (Sync problem) set flag on later
		_isRecording = isRecording;
	}
	else
	{
		// (Sync problem) set flag off first
		_isRecording = isRecording;

		// Update recording count
		_mainWindow->ui->label_record->setText(QString().sprintf("%d acquisitions are recorded..", _recordingDataBuffer->getFrameCount()));

		// Show view window with recorded buffer
		RecordResultView *resultView = new RecordResultView(_recordingDataBuffer, _mainWindow->_imageProcess.get());
		resultView->setGeometry(400, 150, 900, 700);
		resultView->show();

		// Recorded data buffer will be released in viewer window
		_recordingDataBuffer = NULL;
	}
}

// Implements DeviceDataAcquisitionDelegate

void DataAcquisition::didAcquireData(int tag, uint16_t *acquiredData)
{
	// Save frame if it is recording
	if (isRecording())
	{
		if (_frameCountToDrop > 0)
		{
			_frameCountToDrop--;
			return;
		}

		if (_frameCountToRecord != -1)
			_frameCountToRecord--;

		if (false == _recordingDataBuffer->addFrame(acquiredData))
		{
			printf("# Failed addFrame : memory buffer full\n");

			// managerDidGotMemoryFull notification will call this. 
			emit signal_didFinishRecording();
		}

		// Update recorded count
		if (_recordingDataBuffer->getFrameCount() % 10 == 0)
			_mainWindow->ui->label_record->setText(QString().sprintf("%d acquisitions are recorded..", _recordingDataBuffer->getFrameCount()));

		if (_frameCountToRecord == 0)
		{
			// Stop recording
			_frameCountToRecord = -1;
			emit signal_didFinishRecording();

			return;
		}
	}
}

void DataAcquisition::slot_didFinishRecording()
{
	puts("slot_didFinishRecording()");

	// Stop recording
	setRecording(false);

	// FIXME: this is only for oph. system with scan y
	{
		// Stop all
		if (_scanMirror) _scanMirror->Stop();
		_dataSource->Stop();
		_dataSource->SwitchOff();

		// Start device first
		if (_scanMirror) _scanMirror->Start(); // with scan pattern 1
		_dataSource->Start(); 

		// wait
		::Sleep(500);

		// switch on later
		_dataSource->SwitchOn();
	}
}