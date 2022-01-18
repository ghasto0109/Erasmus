#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_

#include <memory>
#include <QtWidgets>

namespace Ui 
{
    class MainWindow;
}

class AlazarDAQ;
class AlazarProcess;
class OFDIProcessViewer;
class AnalogOutput;

class DataRecording;
class DataBufferModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

	// callback from AlazarProcess
	void didFinishImageProcessing();

protected:
	void closeEvent(QCloseEvent *event) override;

signals:
    void signal_recordFinished();

private slots:
    void slot_recordFinished();

    // MemoryManager signals
    void managerDidUpdateMemoryAllocationProgress(int currentFrameCount, int frameCountToAllocate);
    void managerDidFinishMemoryAllocation(int allocatedFrameCount);
    void managerDidFailMemoryAllocation();
    void managerDidGotMemoryFull();

private:
    std::unique_ptr<Ui::MainWindow> ui;
    std::unique_ptr<AlazarDAQ> _dataSource;
    std::unique_ptr<AlazarProcess> _imageProcess;
    std::unique_ptr<AnalogOutput> _analogOutput;

    std::unique_ptr<OFDIProcessViewer> _ofdiViewer;
    std::unique_ptr<DataRecording> _dataRecording;
    std::unique_ptr<DataBufferModel> _recordedDataBuffer;
};

#endif // MAINWINDOW_H_