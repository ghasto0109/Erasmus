#include "MainWindow.Doppler.h"
#include "ui_MainWindow.h"

#include "SignatecDAQ/SignatecDAQ.h"
#include "DataAcquisition/DataRecording.h"
#include "DataAcquisition/MemoryManager.h"
#include "DataAcquisition/Calibration.h"
#include "OFDIProcess3/ImageProcess.h"
#include "NIDAQ/AnalogOutput.h"

#include "OFDIProcessViewer.h"
#include "QPictureBox.h"
#include "QScopeControl.h"

#include <objcpp/ui.h>

using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), 

    _dataSource(new SignatecDAQ), 
    _imageProcess(new ImageProcess), 
    _calibration(new Calibration), 
	_analogOutput(new AnalogOutput), 

    _ofdiViewer(nullptr), 
    _dataRecording(new DataRecording)
{
	// setup UI
    ui->setupUi(this);

    // add object property UI
    ui->layout_properties->addWidget(setupUi("SignatecDAQ", _dataSource.get()));
    ui->layout_properties->addWidget(setupUi("ImageProcess", _imageProcess.get()));
    ui->layout_properties->addWidget(setupUi("Calibration", _calibration.get()));
	ui->layout_properties->addWidget(setupUi("Galvanometer", _analogOutput.get()));

    // object properties
	const int nChannels = 1;
    const int nScans = 1300;
    const int nAlines = 1024;

    _dataSource->nChannels = nChannels;
    _dataSource->nScans = nScans;
    _dataSource->nAlines = nAlines;
    _dataSource->UseVirtualDevice = false;
    // _dataSource->UseInternalTrigger = true;
    _dataSource->PreTrigger = 0;

    _imageProcess->nChannels = nChannels;
    _imageProcess->nScans = nScans;
    _imageProcess->nAlines = nAlines;
    _imageProcess->WindowLevel = 160.f;
    _imageProcess->WindowWidth = 37.f;
    _imageProcess->DC = -20;
	_imageProcess->FrameRate = 5;

	_analogOutput->nChannels = 2;
	_analogOutput->nAlines = 1024;
	_analogOutput->nFrames1 = 10240;
	_analogOutput->ScanPatternFile1 = "scan.bin";
	_analogOutput->nFrames2 = 10240;
	_analogOutput->ScanPatternFile2 = "scan.bin";
	_analogOutput->Amp = 3.5f;
	_analogOutput->ClockPort = "/Dev1/PFI0";
	_analogOutput->ClockFrequency = 300;

	MemoryManager *memoryManager = _dataRecording->_memoryManager.get();

	memoryManager->nChannels = nChannels;
    memoryManager->nScans = nScans;
    memoryManager->nAlines = nAlines;
    //memoryManager->FrameCountToAllocate = 400;
    memoryManager->FrameCountToAllocate = 10500;

    // -----------

	using namespace std::placeholders;

	// Connect callback for data recording
	_dataSource->DidAcquireData += std::bind(&DataRecording::didAcquireData, _dataRecording.get(), _1, _2);

    // Connect memory manager signals
    connect(memoryManager, SIGNAL(managerDidUpdateMemoryAllocationProgress(int, int)), this, SLOT(managerDidUpdateMemoryAllocationProgress(int, int)));
    connect(memoryManager, SIGNAL(managerDidUpdateMemoryCapacity(int)), ui->progress_memory, SLOT(setValue(int)));
    connect(memoryManager, SIGNAL(managerDidFinishMemoryAllocation(int)), this, SLOT(managerDidFinishMemoryAllocation(int)));
    connect(memoryManager, SIGNAL(managerDidFailMemoryAllocation()), this, SLOT(managerDidFailMemoryAllocation()));
    connect(memoryManager, SIGNAL(managerDidGotMemoryFull()), this, SLOT(managerDidGotMemoryFull()));

    // Start allocating memory
    memoryManager->initialize();
    
    // Initialize image process to show acquired image
	_imageProcess->initialize();
	_dataSource->DidAcquireData += std::bind(&ImageProcess::didAcquireData, _imageProcess.get(), _1, _2);

    // Add image process viewer
    _ofdiViewer = new OFDIProcessViewer(_imageProcess->nScans, ui->panel_center);

    QHBoxLayout *layout_center = new QHBoxLayout(ui->panel_center);
    layout_center->setMargin(0);
    layout_center->setSpacing(0);
    layout_center->addWidget(_ofdiViewer);

    _imageProcess->DidFinishImageProcessing += std::bind(&MainWindow::didFinishImageProcessing, this, _1);

    // Initialize calibration tool
	if (_calibration)
		_dataSource->DidAcquireData += std::bind(&Calibration::didAcquireData, _calibration.get(), _1, _2);

    // ----------

    connect(ui->button_acquisition, &QAbstractButton::clicked, [this](bool clicked)
    {
        if (clicked)
        {
            // Start acquisition
            ui->button_acquisition->setText("Stop Acquisition");
	        _dataSource->Start();
        }
        else
        {
            // Stop acquisition
            ui->button_acquisition->setText("Start Acquisition");
            _dataSource->Stop();
        }
    });

    connect(ui->button_record, &QAbstractButton::clicked, [this](bool clicked)
    {
        if (clicked)
        {
            // Start recording
            ui->button_record->setText("Stop Recording");
            _dataRecording->setRecording(true);
        }
        else
        {
            // Stop recording
            ui->button_record->setText("Start Recording");
            _dataRecording->setRecording(false);
        }
    });
}

MainWindow::~MainWindow()
{
    // empty destructor for unique_ptr with incomplete types
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // deallocate thread objects first
    _dataSource = nullptr;
    _imageProcess = nullptr;
}

void MainWindow::didFinishImageProcessing(ImageProcess *process)
{
    // Update image
    _ofdiViewer->image_preview->setImage(process->resultImage);

    // Update fringe scope if exists
    uint16_t *diagnosisFringe = process->debugFringe.raw_ptr();
    if (diagnosisFringe)
    {
        _ofdiViewer->scope_fringe->SetChannelBuffer(0, diagnosisFringe, process->nScans);
        _ofdiViewer->scope_fringe->SetChannelBuffer(1, diagnosisFringe + process->nScans, process->nScans);
    }

    // Update debug data if exists
    uint16_t *diagnosisResult = process->debugImage.raw_ptr();
    if (diagnosisResult)
    {
        _ofdiViewer->scope_debug->SetChannelBuffer(0, diagnosisResult, process->nScans2n / 2);
        _ofdiViewer->scope_debug->SetChannelBuffer(1, diagnosisResult + process->nScans2n / 2, process->nScans2n / 2);
    }

    // (if recording) Update recorded count
    if (_dataRecording->isRecording())
        ui->label_record->setText(QString().sprintf("%d acquisitions are recorded..", _dataRecording->getRecordedFrameCount()));
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
	_dataRecording->setRecording(false);

    ui->button_record->setChecked(false);
    ui->button_record->setText("Start Recording");

	QMessageBox::warning(NULL, "Recording stopped", "Memory for data acquisition is full.");
}
