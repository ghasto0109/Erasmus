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
class DataRecording;
class Calibration;
class OFDIProcessViewer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

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
    std::unique_ptr<DataRecording> _dataRecording;
    std::unique_ptr<Calibration> _calibration;

    OFDIProcessViewer *_ofdiViewer;
};

#endif // MAINWINDOW_H_