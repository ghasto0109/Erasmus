#ifndef OFDI_PROCESS_VIEWER_H_
#define OFDI_PROCESS_VIEWER_H_

#include <QtWidgets>

class QPictureBox;
class QScopeControl;

class OFDIProcessViewer : public QWidget
{
public:
	OFDIProcessViewer(int nScans, QWidget *parent = nullptr);
	void updateLayout();

	// widgets for showing processing result
	QPictureBox *image_preview;
	QScopeControl *scope_fringe; // Scope view of original fringe data
	QScopeControl *scope_debug;	// Scope view of diagnosis data for debugging

private:
	// layouts
	bool _showLargeImage;
	QStackedLayout *layout_content;
	QHBoxLayout *layout_previewSmall, *layout_previewLarge;
};

#endif // OFDI_PROCESS_VIEWER_H_