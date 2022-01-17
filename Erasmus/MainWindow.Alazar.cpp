#include "MainWindow.Alazar.h"
#include "ui_MainWindow.h"

#include "AlazarDAQ/AlazarDAQ.h"
#include "OFDIProcess3/AlazarProcess.h"
#include "OFDIProcess3/ImageProcess.h"
#include "NIDAQ/AnalogOutput.h"

#include "DataAcquisition/DataRecording.h"
#include "DataAcquisition/MemoryManager.h"
#include "DataAcquisition/RecordResultView.h"
#include "DataAcquisition/DataBufferModel.h"

#include "OFDIProcessViewer.h"
#include "QPictureBox.h"
#include "QScopeControl.h"

#include <objcpp/ui.h>

using namespace std;
bool isRecording = false; // added by MH

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), 

    _dataSource(new AlazarDAQ), 
    _imageProcess(new AlazarProcess), 
	_dataRecording(new DataRecording), 
    _analogOutput(new AnalogOutput)
{
	// setup UI
    ui->setupUi(this);
	setWindowTitle("Erasmus.Alazar");

    // add object property UI
    ui->layout_properties->addWidget(setupUi("AlazarDAQ", _dataSource.get()));
    ui->layout_properties->addWidget(setupUi("AlazarProcess", _imageProcess.get()));
    ui->layout_properties->addWidget(setupUi("Galvanometer", _analogOutput.get()));

	// object properties
    const int nChannels = 1;    //20181031
    const int nScans = 384;  //20200630
    const int nAlines = 2500;

    _dataSource->nChannels = nChannels;
    _dataSource->nScans = nScans;
    _dataSource->nAlines = nAlines;
    _dataSource->ExternalClock = true;
    _dataSource->AutoTrigger = false;
    _dataSource->TriggerDelay = 16;

    _imageProcess->nChannels = nChannels;
    _imageProcess->nScans = nScans;
    _imageProcess->nAlines = nAlines;
	_imageProcess->WindowLevel = 110.f;
	_imageProcess->WindowWidth = 60.f;

    _analogOutput->nChannels = 2;
	_analogOutput->nAlines = 64300; //2500;  // 64300;  // 20210203
		//nAlines;
	_analogOutput->nFrames1 = 1000; //1001;  // 1000;  // 20210203
    _analogOutput->ScanPatternFile1 = "500fixy1Vscan.bin";
	//_analogOutput->ScanPatternFile1 = "2sp20160115prev.bin";
	_analogOutput->nFrames2 = 1000; //1001; //1000;  // 20210203
	_analogOutput->ScanPatternFile2 = "500fixy1Vscan.bin"; // "Pattern4.bin";
    _analogOutput->Amp = 10.0f;
    _analogOutput->ClockPort = "/Dev1/PFI1";
    _analogOutput->ClockFrequency = 500;

	MemoryManager *memoryManager = _dataRecording->_memoryManager.get();

	memoryManager->nChannels = nChannels;
    memoryManager->nScans = nScans; 
    memoryManager->nAlines = nAlines;
	memoryManager->FrameCountToAllocate = 100;  //  20021;  // = nVolume(real)*nScans(real)*nAlines(real)*nFrames(real)/nScans/nAlines + 1 

    // -----------

	using namespace std::placeholders;

    // Connect memory manager signals
    connect(memoryManager, SIGNAL(managerDidUpdateMemoryAllocationProgress(int, int)), this, SLOT(managerDidUpdateMemoryAllocationProgress(int, int)));
    connect(memoryManager, SIGNAL(managerDidUpdateMemoryCapacity(int)), ui->progress_memory, SLOT(setValue(int)));
    connect(memoryManager, SIGNAL(managerDidFinishMemoryAllocation(int)), this, SLOT(managerDidFinishMemoryAllocation(int)));
    connect(memoryManager, SIGNAL(managerDidFailMemoryAllocation()), this, SLOT(managerDidFailMemoryAllocation()));
    connect(memoryManager, SIGNAL(managerDidGotMemoryFull()), this, SLOT(managerDidGotMemoryFull()));

    connect(this, SIGNAL(signal_recordFinished()), this, SLOT(slot_recordFinished()));

    // Start allocating memory
    memoryManager->initialize();
    
    // Initialize image process to show acquired image
	_imageProcess->initialize();
	_dataSource->DidAcquireData += std::bind(&AlazarProcess::didAcquireData, _imageProcess.get(), _1, _2);

    // Add image process viewer
    _ofdiViewer.reset(new OFDIProcessViewer(nScans, ui->panel_center));

    QHBoxLayout *layout_center = new QHBoxLayout(ui->panel_center);
    layout_center->setMargin(0);
    layout_center->setSpacing(0);
	layout_center->addWidget(_ofdiViewer.get());

	_imageProcess->DidFinishImageProcessing += std::bind(&MainWindow::didFinishImageProcessing, this);

	// Connect callback for data recording
	_dataSource->DidAcquireData += std::bind(&DataRecording::didAcquireData, _dataRecording.get(), _1, _2);

	// ----------

	connect(ui->button_acquisition, &QAbstractButton::clicked, [this](bool clicked)
	{
		if (clicked)
		{
			// Start acquisition
			ui->button_acquisition->setText("Stop Acquisition");
			
            // Reset NI to zero voltage
            _analogOutput->ResetZero();

            // Sleep
            ::Sleep(300);

            // Stop zero voltage streamibng
            _analogOutput->Stop();

            // Start Alazar first and wait
            _dataSource->Start();

            // Sleep
            ::Sleep(300);

            // Start NI laster - it will trigger alazar
            _analogOutput->Start();
        }
		else
		{
			// Stop acquisition
			ui->button_acquisition->setText("Start Acquisition");
			
            _dataSource->Stop();
            _analogOutput->Stop();

            // Reset NI to zero voltage
            _analogOutput->ResetZero();
            ::Sleep(300);
            _analogOutput->Stop();
        }
	});

	// Commentted out by MH
	connect(ui->button_record, &QAbstractButton::clicked, [this](bool clicked)
	{
		if (clicked)
		{
			// Start recording
			ui->button_record->setText("Stop Recording");
			//////////////////////////////////////////////////////////////////////////////////////////////////////////// 20191104 insert
			// Reset NI to zero voltage
			_analogOutput->ResetZero();

			// Sleep
			::Sleep(300);

			// Stop zero voltage streamibng
			_analogOutput->Stop();

			// Start Alazar first and wait
			_dataSource->Start();

			// Sleep
			::Sleep(300);

			// Start NI laster - it will trigger alazar
			_analogOutput->Start();
			////////////////////////////////////////////////////////////////////////////////////////////////////////////
            _dataRecording->start([this](std::unique_ptr<DataBufferModel> recordedDataBuffer)
            {
                // After recording: show view window with recorded buffer
                _recordedDataBuffer = std::move(recordedDataBuffer);
                emit signal_recordFinished();
            });
        }
		else
		{
			// Stop recording
			ui->button_record->setText("Start Recording");

            _dataRecording->stop();
		}
	});

	// Added by MH
	//connect(ui->button_record, &QAbstractButton::clicked, [this](bool clicked)
	//{
	//	if (clicked)
	//	{
	//		// Start recording
	//		ui->button_record->setText("Stop Recording");
	//		isRecording = true;
	//		_dataSource->saveData = isRecording;
 //      }
	//	else
	//	{
	//		// Stop recording
	//		ui->button_record->setText("Start Recording");
	//		isRecording = false;
	//		_dataSource->saveData = isRecording;
	//	}
	//});
	
	connect(ui->button_yscan, &QAbstractButton::clicked, [this]()
    {
        // Stop all
        _dataSource->Stop();
        _analogOutput->Stop();

        // Reset NI to zero voltage
        _analogOutput->ResetZero();
        ::Sleep(300);

		// Start NI laster - it will trigger alazar
		_analogOutput->Start();
        
		// Start recording
		_dataRecording->start(_analogOutput->nFrames2, [this](std::unique_ptr<DataBufferModel> recordedDataBuffer)
        {
            // After recording: show view window with recorded buffer
            _recordedDataBuffer = std::move(recordedDataBuffer);
            emit signal_recordFinished();
        });

        // Stop zero voltage streaming
        _analogOutput->Stop();

        // Start Alazar first and wait
        _dataSource->Start();

        // Sleep
        ::Sleep(300);

        // Start NI with scan - it will trigger alazar
        _analogOutput->StartScan();
    });
}

MainWindow::~MainWindow()
{
    // empty destructor for unique_ptr with incomplete types
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Reset NI to zero voltage
    _analogOutput->Stop();
    _analogOutput->ResetZero();
    ::Sleep(300);
    _analogOutput->Stop();

    // deallocate thread objects first
    _dataSource = nullptr;
    _imageProcess = nullptr;
}

void MainWindow::slot_recordFinished()
{
    // TODO: Pass AlazarProcess to RecordResultView
    ImageProcess dummy;
    dummy.nChannels = _imageProcess->nChannels;
    dummy.nScans = _imageProcess->nScans;
    dummy.nAlines = _imageProcess->nAlines;

    RecordResultView *resultView = new RecordResultView(std::move(_recordedDataBuffer), &dummy);
    resultView->setGeometry(400, 150, 900, 700);
    resultView->show();
}

void MainWindow::didFinishImageProcessing()
{
	// Update image
	//_ofdiViewer->image_preview->setImage(_imageProcess->resultImage);
	
	// Update fringe scope if exists
	uint16_t *diagnosisFringe = _imageProcess->debugFringe.raw_ptr();
	const int nScans = _imageProcess->debugFringe.length() / 2;
	if (diagnosisFringe)
	{
		_ofdiViewer->scope_fringe->SetChannelBuffer(0, diagnosisFringe, nScans);
		_ofdiViewer->scope_fringe->SetChannelBuffer(1, diagnosisFringe + nScans, nScans);
	}

	// Update debug data if exists
	uint16_t *diagnosisResult = _imageProcess->debugImage.raw_ptr();
    const int imageDepth = _imageProcess->debugImage.length() / 2;
	if (diagnosisResult)
	{
		_ofdiViewer->scope_debug->SetChannelBuffer(0, diagnosisResult, imageDepth);
		_ofdiViewer->scope_debug->SetChannelBuffer(1, diagnosisResult + imageDepth, imageDepth);
	}

	// (if recording) Update recorded count
    if (_dataRecording->isRecording())
        ui->label_record->setText(QString().sprintf("Recorded %d frames..", _dataRecording->getRecordedFrameCount()));
}

// signals from MemoryManager

void MainWindow::managerDidUpdateMemoryAllocationProgress(int currentFrameCount, int frameCountToAllocate)
{
	ui->progress_memory->setEnabled(true);
	ui->progress_memory->setRange(0, 0); // Show busy state

	ui->label_record->setText(QString().sprintf("%d%% allocated..", currentFrameCount * 100 / frameCountToAllocate));
}

void MainWindow::managerDidFinishMemoryAllocation(int allocatedFrameCount)
{
	ui->progress_memory->setRange(0, _dataRecording->_memoryManager->getAvailableFrameCount());
	ui->label_record->setText("Ready to record..");
}

void MainWindow::managerDidFailMemoryAllocation()
{
	ui->label_record->setText("Failed to allocate memory.");
	ui->label_record->setStyleSheet("QLabel { color : red; }");
}

void MainWindow::managerDidGotMemoryFull()
{
	_dataRecording->stop();

    ui->button_record->setChecked(false);
    ui->button_record->setText("Start Recording");

	// QMessageBox::warning(NULL, "Recording stopped", "Memory for data acquisition is full.");
}

//////////

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);

    HWND console = ::GetConsoleWindow();
    ::SetWindowPos(console, NULL, 20, 20, 0, 0, SWP_NOSIZE);

    MainWindow mainWindow;
    mainWindow.move(200, 40);
    mainWindow.show();

    return application.exec();
}