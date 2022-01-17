#include "stdafx.h"
#include "RecordResultView.h"
#include "moc_RecordResultView.cpp"

#include "DataBufferModel.h"
#include "ConfigurationFile.h"

#include "OFDIProcess3/ImageProcess.h"

#include "Erasmus/OFDIProcessViewer.h"
#include "Erasmus/QPictureBox.h"
#include "Erasmus/QScopeControl.h"

RecordResultView::RecordResultView(std::unique_ptr<DataBufferModel> &&dataBuffer, ImageProcess *imageProcess) :
	_isSaved(false), 

	_dataBuffer(std::move(dataBuffer)), 
	_imageProcess(nullptr)
{
	// Set tab page title
	if (_dataBuffer->getFrameCount() > 1)
		setWindowTitle("Recorded data *");
	else
		setWindowTitle("Captured Frame *");

    // Copy ImageProcess
	_imageProcess.reset(new ImageProcess);

	_imageProcess->nChannels = imageProcess->nChannels;
    _imageProcess->nScans = imageProcess->nScans;
	_imageProcess->nAlines = imageProcess->nAlines;

    _imageProcess->WindowLevel = imageProcess->WindowLevel;
    _imageProcess->WindowWidth = imageProcess->WindowWidth;
    _imageProcess->DC = imageProcess->DC;

	_imageProcess->initialize();

	// ImageProcessViewer
	_ofdiViewer.reset(new OFDIProcessViewer(_imageProcess->nScans)); 
	_ofdiViewer->image_preview->setInverted(true);
	_imageProcess->DidFinishImageProcessing += std::bind(&RecordResultView::didFinishImageProcessing, this, std::placeholders::_1);

	// Create user interface
	initUI();

	// Delete window when window closed
	setAttribute(Qt::WA_DeleteOnClose);
}

RecordResultView::~RecordResultView()
{
    // empty destructor for unique_ptr with incomplete types
}

void RecordResultView::initUI()
{
	// Initialize user interface
	QWidget *panel_main = new QWidget;
	QHBoxLayout *layout_main = new QHBoxLayout;
	{
		// Left panel
		QWidget *panel_left = new QWidget(panel_main);
		QVBoxLayout *layout_left = new QVBoxLayout;
		{
			button_saveToFile = new QPushButton("Save to File..."); 
			button_saveToFile->setMinimumHeight(30);
			layout_left->addWidget(button_saveToFile);

			layout_left->addStretch();
		}
		panel_left->setLayout(layout_left);
		panel_left->setFixedWidth(120);
		layout_main->addWidget(panel_left);

		// Picture panel
		QVBoxLayout *layout_picture = new QVBoxLayout;
		{
			QHBoxLayout *layout_currentFrame = new QHBoxLayout;
			{
				label_currentFrame = new QLabel("Frame: 0 / 0");
				label_currentFrame->setMinimumHeight(30);
				layout_currentFrame->addWidget(label_currentFrame);

				slider_currentFrame = new QSlider;
				slider_currentFrame->setOrientation(Qt::Horizontal);
				layout_currentFrame->addWidget(slider_currentFrame);
			}
			layout_picture->addLayout(layout_currentFrame);

            layout_picture->addWidget(_ofdiViewer.get());
		}
		layout_main->addLayout(layout_picture);
	}
	panel_main->setLayout(layout_main);

	setCentralWidget(panel_main);

	// connect signals
	connect(button_saveToFile, &QPushButton::clicked, this, &RecordResultView::action_saveToFile);
	connect(slider_currentFrame, &QSlider::valueChanged, this, &RecordResultView::update_currentFrame);

	// Set slider range
	slider_currentFrame->setRange(0, (int)_dataBuffer->getFrameCount() - 1);

	// Show first frame
	update_currentFrame(0);
}

void RecordResultView::closeEvent(QCloseEvent *event)
{
	// do not close by default
	event->ignore();

	// close if already saved
	if (_isSaved)
	{
		event->accept();
		return;
	}

	QMessageBox::StandardButton selectedButton = QMessageBox::warning(this, "Warning", 
		"Do you want to save recorded data to hard disk?\nIf not, recorded data will be lost.", 
		QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Cancel);

	// close if save succeeded
	if (selectedButton == QMessageBox::Save)
	{
		if (saveToFile())
			event->accept();
	}
	// close if discard
	else if (selectedButton == QMessageBox::Discard)
	{
		event->accept();
	}
}

// User actions
void RecordResultView::action_saveToFile()
{
	saveToFile();
}

void RecordResultView::update_currentFrame(int value)
{
	// Update current frame label
	label_currentFrame->setText(QString().sprintf("Frame: %d / %d", value + 1, _dataBuffer->getFrameCount()));

	// Load current frame image
	_imageProcess->startImageProcessing(_dataBuffer->getFrame(value));
}

// Callback from ImageProcess
void RecordResultView::didFinishImageProcessing(ImageProcess *process)
{
    // Update image
    //_ofdiViewer->image_preview->setImage(process->resultImage);

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
}

// Save recorded data to file (return false : canceled)
bool RecordResultView::saveToFile()
{
	QString fileName = QFileDialog::getSaveFileName(this, "Save As", "", "OCT Configuration File (*.ini)");
	if (fileName == "") return false;

	QProgressDialog progressDialog("Saving data file...", "Cancel", 0, (int)_dataBuffer->getFrameCount(), this);
	progressDialog.setWindowModality(Qt::WindowModal);

	// Save ini file
	QSettings settings(fileName, QSettings::IniFormat);

	settings.setValue("configuration/nAlines", _imageProcess->nAlines.get());
	settings.setValue("configuration/nScans", _imageProcess->nScans.get());
	settings.setValue("configuration/processType", "intensity");

	settings.setValue("file/dataFile", QFileInfo(fileName).baseName() + ".data");
	settings.setValue("file/calibrationFile", QFileInfo(fileName).baseName() + ".calibration");
	settings.setValue("file/backgroundFile", QFileInfo(fileName).baseName() + ".background");

	settings.sync();
	
	ConfigurationFile iniFile;
	if (false == iniFile.loadFromIniFile(fileName))
	{
		QMessageBox::critical(this, "Error", "Failed to save ini file.");
		return false;
	}

	// Copy calibration file
    if (false == QFile::copy(_imageProcess->CalibrationFile.get().c_str(), iniFile.calibrationFilePath()))
	{
        std::cout << "Failed to copy calibration file." << std::endl;
	}

	// Copy background file
    if (false == QFile::copy("bg.bin", iniFile.backgroundFilePath()))
	{
        std::cout << "Failed to copy background file." << std::endl;
	}

	// Save data file
	QFile output(iniFile.dataFilePath());
	if (false == output.open(QIODevice::WriteOnly))
	{
		QMessageBox::critical(this, "Error", "Failed to save data file.");
		return false;
	}

	for (int frameIndex = 0; frameIndex < _dataBuffer->getFrameCount(); frameIndex++)
	{
		if (frameIndex % 100 == 0)
			std::cout << "Writing #" << frameIndex << " frames..." << std::endl;

		qint64 written = output.write(
			(char *)_dataBuffer->getFrame(frameIndex), 
			// FIXME: _dataBuffer.size()
			//_imageProcess->nChannels * _imageProcess->nScans * _imageProcess->nAlines * sizeof(uint16_t));
			_imageProcess->nChannels * _imageProcess->nScans * _imageProcess->nAlines * sizeof(uint8_t) * 3 / 2 + 0.5);

		progressDialog.setValue(frameIndex);

		if (progressDialog.wasCanceled())
		{
			output.close();
			return false;
		}
	}

	output.close();

	// Successfully saved. 
	_isSaved = true;

	if (_dataBuffer->getFrameCount() > 1)
		setWindowTitle("Recorded data");
	else
		setWindowTitle("Captured Frame");

	return true;
}
