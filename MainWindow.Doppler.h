#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_

#include <memory>
#include <QtWidgets>

namespace Ui 
{
    class MainWindow;
}

class SignatecDAQ;
class ImageProcess;
class Calibration;
class AnalogOutput;

class OFDIProcessViewer;
class DataRecording;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

	// callback from ImageProcess
	void didFinishImageProcessing(ImageProcess *controller);

protected:
	void closeEvent(QCloseEvent *event) override;

private slots:
    // MemoryManager signals
    void managerDidUpdateMemoryAllocationProgress(int currentFrameCount, int frameCountToAllocate);
    void managerDidFinishMemoryAllocation(int allocatedFrameCount);
    void managerDidFailMemoryAllocation();
    void managerDidGotMemoryFull();

private:
    std::unique_ptr<Ui::MainWindow> ui;

    std::unique_ptr<SignatecDAQ> _dataSource;
    std::unique_ptr<ImageProcess> _imageProcess;
    std::unique_ptr<Calibration> _calibration;
	std::unique_ptr<AnalogOutput> _analogOutput;

    std::unique_ptr<DataRecording> _dataRecording;
    OFDIProcessViewer *_ofdiViewer;
};

#endif // MAINWINDOW_H_