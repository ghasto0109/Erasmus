#include "MainWindow.Rodent.h"
#include "ui_MainWindow.h"

#include "SignatecDAQ/SignatecDAQ.h"
#include "DataAcquisition/DataRecording.h"
#include "DataAcquisition/MemoryManager.h"
#include "DataAcquisition/Calibration.h"
#include "OFDIProcess3/ImageProcess.h"
#include "NIDAQ/AnalogOutput.h"

#include "DataAcquisition/RecordResultView.h"
#include "DataAcquisition/DataBufferModel.h"

#include "QPictureBox.h"
#include "QScopeControl.h"

#include <objcpp/ui.h>

using namespace std;

MainWindow::MainWindow(QWidget *parent) :
QMainWindow(parent),
ui(new Ui::MainWindow),

_dataSource(new SignatecDAQ),
_imageProcess0(new ImageProcess),
_imageProcess1(new ImageProcess),
_calibration(new Calibration),
_analogOutput(new AnalogOutput),

_ofdiViewer(nullptr),
_dataRecording(new DataRecording)
{
	// setup UI
	ui->setupUi(this);

	// add object property UI
	ui->layout_properties->addWidget(setupUi("SignatecDAQ", _dataSource.get()));
	ui->layout_properties->addWidget(setupUi("ImageProcess0", _imageProcess0.get()));
	ui->layout_properties->addWidget(setupUi("ImageProcess1", _imageProcess1.get()));
	ui->layout_properties->addWidget(setupUi("Calibration", _calibration.get()));
	ui->layout_properties->addWidget(setupUi("Galvanometer", _analogOutput.get()));

	// object properties
	const int nChannels = 1;
	const int nScans = 1184;
	const int nAlines = 1024;

	_dataSource->nChannels = nChannels;
	_dataSource->nScans = nScans;
	_dataSource->nAlines = nAlines;
	_dataSource->UseVirtualDevice = false;
	_dataSource->UseInternalTrigger = false;
	_dataSource->PreTrigger = 800;

	_imageProcess0->nChannels = nChannels;
	_imageProcess0->nScans = nScans;
	_imageProcess0->nAlines = nAlines;
	_imageProcess0->WindowLevel = 140.f;
	_imageProcess0->WindowWidth = 38.f;
	_imageProcess0->DC = -60;

	_imageProcess0->ProcessID = 2;
    _imageProcess0->Transpose = true;

	_imageProcess1->nChannels = nChannels;
	_imageProcess1->nScans = nScans;
	_imageProcess1->nAlines = nAlines;
	_imageProcess1->WindowLevel = 140.f;
	_imageProcess1->WindowWidth = 38.f;
	_imageProcess1->DC = -60;

	_imageProcess1->ProcessID = 0;
    _imageProcess1->Transpose = false;

	_analogOutput->nChannels = 2;
	_analogOutput->nAlines = nAlines;
	_analogOutput->nFrames1 = 8;
	_analogOutput->ScanPatternFile1 = "scan1024_x6ypreview.bin";
	_analogOutput->nFrames2 = 9024;
	_analogOutput->ScanPatternFile2 = "scan9024_5pattern.bin";
	_analogOutput->Amp = 9.0f;
	_analogOutput->ClockPort = "/Dev1/PFI0";
	_analogOutput->ClockFrequency = 300;

	MemoryManager *memoryManager = _dataRecording->_memoryManager.get();

	memoryManager->nChannels = nChannels;
	memoryManager->nScans = nScans;
	memoryManager->nAlines = nAlines;
	memoryManager->FrameCountToAllocate = 9100;

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

	connect(this, SIGNAL(signal_recordFinished()), this, SLOT(slot_recordFinished()));

	// Start allocating memory
	memoryManager->initialize();

	// Initialize image process to show acquired image
	_imageProcess0->initialize();
	_dataSource->DidAcquireData += std::bind(&ImageProcess::didAcquireData, _imageProcess0.get(), _1, _2);
	_imageProcess1->initialize();
	_dataSource->DidAcquireData += std::bind(&ImageProcess::didAcquireData, _imageProcess1.get(), _1, _2);

	// Add image process viewer
	_ofdiViewer = new OFDIProcessViewerRodent(nScans, ui->panel_center);

	QHBoxLayout *layout_center = new QHBoxLayout(ui->panel_center);
	layout_center->setMargin(0);
	layout_center->setSpacing(0);
	layout_center->addWidget(_ofdiViewer);

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

			// start first
			_dataSource->Start();
			_analogOutput->Start();

			// wait
			::Sleep(500);

			// switch on later
			_dataSource->SwitchOn();
		}
		else
		{
			// Stop acquisition
			ui->button_acquisition->setText("Start Acquisition");

			// stop and switch off/
			_dataSource->Stop();
			_analogOutput->Stop();

			_dataSource->SwitchOff();
		}
	});

	connect(ui->button_record, &QAbstractButton::clicked, [this](bool clicked)
	{
		if (clicked)
		{
			// Start recording
			ui->button_record->setText("Stop /Recording");

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

	connect(ui->button_yscan, &QAbstractButton::clicked, [this]()
	{
		// Stop all
		_analogOutput->Stop();
		_dataSource->Stop();
		_dataSource->SwitchOff();

		// Start recording
		_dataRecording->start(_analogOutput->nFrames2, [this](std::unique_ptr<DataBufferModel> recordedDataBuffer)
		{
			// After recording: show view window with recorded buffer
			_recordedDataBuffer = std::move(recordedDataBuffer);
			emit signal_recordFinished();
		});

		// Start device first
		_analogOutput->StartScan();
		_dataSource->Start(); // with scan pattern 2

		// wait
		puts("3");
		::Sleep(1000);
		puts("2");
		::Sleep(1000);
		puts("1");
		::Sleep(1000);
		puts("Go!");

		// switch on later
		_dataSource->SwitchOn();
	});

	auto didFinishImageProcessing = [this](ImageProcess *process)
	{
		// Update image
		if (process == _imageProcess0.get())
			_ofdiViewer->image_preview0->setImage(process->resultImage);
		else if (process == _imageProcess1.get())
			_ofdiViewer->image_preview1->setImage(process->resultImage);

		// Update fringe scope if exists
		uint16_t *diagnosisFringe = process->debugFringe.raw_ptr();
		if (diagnosisFringe)
		{
			_ofdiViewer->scope_fringe->SetChannelBuffer(0, diagnosisFringe, process->nScans);
			_ofdiViewer->scope_fringe->SetChannelBuffer(1, diagnosisFringe + process->nScans, process->nScans);
		}

		// nScans2n = 2 ^ ceil(log2(nScans));
		const int nScans2n = 1 << ((int)(log2((double)process->nScans) + 1.));

		// Update debug data if exists
		uint16_t *diagnosisResult = process->debugImage.raw_ptr();
		if (diagnosisResult)
		{
			_ofdiViewer->scope_debug->SetChannelBuffer(0, diagnosisResult, nScans2n / 2);
			_ofdiViewer->scope_debug->SetChannelBuffer(1, diagnosisResult + nScans2n / 2, nScans2n / 2);
		}

		// (if recording) Update recorded count
		if (_dataRecording->isRecording())
			ui->label_record->setText(QString().sprintf("Recorded %d acquisitions..", _dataRecording->getRecordedFrameCount()));
	};

	_imageProcess0->DidFinishImageProcessing += didFinishImageProcessing;
	_imageProcess1->DidFinishImageProcessing += didFinishImageProcessing;
}

MainWindow::~MainWindow()
{
    // empty destructor for unique_ptr with incomplete types
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // deallocate thread objects first
    _dataSource = nullptr;
    _imageProcess0 = nullptr;
    _imageProcess1 = nullptr;
}

void MainWindow::slot_recordFinished()
{
	RecordResultView *resultView = new RecordResultView(std::move(_recordedDataBuffer), _imageProcess0.get());
	resultView->setGeometry(400, 150, 900, 700);
	resultView->show();
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

	QMessageBox::warning(NULL, "Recording stopped", "Memory for data acquisition is full.");
}

//////////

OFDIProcessViewerRodent::OFDIProcessViewerRodent(int nScans, QWidget *parent) : QWidget(parent),
_showLargeImage(false)
{
    // nScans2n = 2 ^ ceil(log2(nScans));
    const int nScans2n = 1 << ((int)(log2((double)nScans) + 1.));

    image_preview0 = new QPictureBox(this);
	image_preview0->setInverted(true);
    image_preview0->setMinimumSize(200, 200);

    image_preview1 = new QPictureBox(this);
	image_preview1->setInverted(true);
	image_preview1->setMinimumSize(200, 200);

    layout_content = new QStackedLayout;
    layout_content->setMargin(0);
    layout_content->setSpacing(0);
    {
        // Index 0 : panel_diagnosis
        QWidget *panel_diagnosis = new QWidget(this);
        QVBoxLayout *layout_diagnosis = new QVBoxLayout;
        layout_diagnosis->setMargin(0);
        layout_diagnosis->setSpacing(0);
        {
            QHBoxLayout *layout_diagnosisToolbox = new QHBoxLayout;
            layout_diagnosisToolbox->setMargin(0);
            layout_diagnosisToolbox->setSpacing(0);
            {
                QWidget *panel_previewSmall = new QWidget(panel_diagnosis);
                layout_previewSmall = new QHBoxLayout;
                {
                    // image_preview will be added here when _showLargeImage == false 
                    layout_previewSmall->addWidget(image_preview0);
                    layout_previewSmall->addWidget(image_preview1);
                }
                panel_previewSmall->setLayout(layout_previewSmall);
                layout_diagnosisToolbox->addWidget(panel_previewSmall);

                layout_diagnosisToolbox->addStretch();
            }
            layout_diagnosis->addLayout(layout_diagnosisToolbox, 0);

            scope_fringe = new QScopeControl;
            layout_diagnosis->addWidget(scope_fringe, 1);

            scope_debug = new QScopeControl;
            layout_diagnosis->addWidget(scope_debug, 1);
        }
        panel_diagnosis->setLayout(layout_diagnosis);
        layout_content->addWidget(panel_diagnosis);

        // Index 1 : panel_previewLarge
        QWidget *panel_previewLarge = new QWidget(this);
        layout_previewLarge = new QGridLayout;
        {
            // image_preview will be added here when _showLargeImage == true
            // layout_previewLarge->addWidget(image_preview0, 1, 0, 1, 2);
            // layout_previewLarge->addWidget(image_preview1, 0, 2, 2, 1);
        }
        panel_previewLarge->setLayout(layout_previewLarge);
        layout_content->addWidget(panel_previewLarge);
    }
    this->setLayout(layout_content);

    // Add scope channels
    scope_fringe->AddChannel("fringe1", nScans);
    scope_fringe->AddChannel("fringe2", nScans);
    scope_debug->AddChannel("debug1", nScans2n / 2);
    scope_debug->AddChannel("debug2", nScans2n / 2);

    // QPictureBox.onClick
    auto clicked = [this](Qt::MouseButton button)
    {
        _showLargeImage = !_showLargeImage;

        if (_showLargeImage)
        {
            layout_previewLarge->addWidget(image_preview0, 1, 0, 1, 2);
            layout_previewLarge->addWidget(image_preview1, 0, 2, 2, 1);
            layout_content->setCurrentIndex(1);
        }
        else
        {
            layout_previewSmall->addWidget(image_preview0);
            layout_previewSmall->addWidget(image_preview1);
            layout_content->setCurrentIndex(0);
        }
    };

    image_preview0->onClicked = clicked;
    image_preview1->onClicked = clicked;
}

//////////

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);

    HWND console = ::GetConsoleWindow();
    ::SetWindowPos(console, NULL, 20, 20, 0, 0, SWP_NOSIZE);

    MainWindow mainWindow;
    mainWindow.setGeometry(200, 40, 1320, 720);
    mainWindow.show();

    return application.exec();
}
