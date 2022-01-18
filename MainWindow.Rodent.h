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

class OFDIProcessViewerRodent;
class DataRecording;
class DataBufferModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

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

    std::unique_ptr<SignatecDAQ> _dataSource;
    std::unique_ptr<ImageProcess> _imageProcess0, _imageProcess1;
	std::unique_ptr<Calibration> _calibration;
	std::unique_ptr<AnalogOutput> _analogOutput;

	std::unique_ptr<DataRecording> _dataRecording;
    OFDIProcessViewerRodent *_ofdiViewer;

	std::unique_ptr<DataBufferModel> _recordedDataBuffer;
};

class QPictureBox;
class QScopeControl;

class OFDIProcessViewerRodent : public QWidget
{
public:
    OFDIProcessViewerRodent(int nScans, QWidget *parent = nullptr);
    void updateLayout();

    // widgets for showing processing result
    QPictureBox *image_preview0, *image_preview1;
    QScopeControl *scope_fringe; // Scope view of original fringe data
    QScopeControl *scope_debug;	// Scope view of diagnosis data for debugging

private:
    // layouts
    bool _showLargeImage;
    QStackedLayout *layout_content;
    QHBoxLayout *layout_previewSmall;
    QGridLayout *layout_previewLarge;
};

#endif // MAINWINDOW_H_