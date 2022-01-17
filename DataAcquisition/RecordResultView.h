#ifndef RECORD_RESULT_VIEW_H_
#define RECORD_RESULT_VIEW_H_

#include <QtWidgets>

class DataBufferModel;
class ImageProcess;
class OFDIProcessViewer;

class RecordResultView : public QMainWindow
{
	Q_OBJECT

public:
	RecordResultView(std::unique_ptr<DataBufferModel> &&dataBuffer, ImageProcess *imageProcess);
	~RecordResultView();

    // callback from ImageProcess
    void didFinishImageProcessing(ImageProcess *controller);

protected:
	void closeEvent(QCloseEvent *event) override;

private slots:
	void action_saveToFile();
	void update_currentFrame(int value);

private:
	void initUI();
	bool saveToFile();
	bool _isSaved;

	std::unique_ptr<DataBufferModel> _dataBuffer;
	std::unique_ptr<ImageProcess> _imageProcess;
	std::unique_ptr<OFDIProcessViewer> _ofdiViewer;

	// Left panel
	QPushButton *button_saveToFile;

	// Center panel
	QSlider *slider_currentFrame;
	QLabel *label_currentFrame;
};

#endif // RECORD_RESULT_VIEW_H_