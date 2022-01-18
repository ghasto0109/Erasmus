#include "stdafx.h"
#include "OFDIProcessViewer.h"

#include "QPictureBox.h"
#include "QScopeControl.h"

#include <math.h>

using namespace std;

OFDIProcessViewer::OFDIProcessViewer(int nScans, QWidget *parent) : QWidget(parent), 
	_showLargeImage(false)
{
    // nScans2n = 2 ^ ceil(log2(nScans));
    const int nScans2n = 1 << ((int)(log2((double)nScans) + 1.));

	image_preview = new QPictureBox(this);
	image_preview->setMinimumSize(200, 200);

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
					layout_previewSmall->addWidget(image_preview);
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
		layout_previewLarge = new QHBoxLayout;
		{
			// image_preview will be added here when _showLargeImage == true
			// layout_previewLarge->addWidget(image_preview);
		}
		panel_previewLarge->setLayout(layout_previewLarge);
		layout_content->addWidget(panel_previewLarge);
	}
	this->setLayout(layout_content);

	// Add scope channels
	scope_fringe->AddChannel("fringe1", nScans);
	scope_fringe->AddChannel("fringe2", nScans);
	scope_debug->AddChannel("debug1", nScans2n/2);
	scope_debug->AddChannel("debug2", nScans2n/2);

	// QPictureBox.onClick
	image_preview->onClicked = [this](Qt::MouseButton button)
	{
		_showLargeImage = !_showLargeImage;

		if (_showLargeImage)
		{
			layout_previewLarge->addWidget(image_preview);
			layout_content->setCurrentIndex(1); 
		}
		else
		{
			layout_previewSmall->addWidget(image_preview);
			layout_content->setCurrentIndex(0); 
		}
	};
}