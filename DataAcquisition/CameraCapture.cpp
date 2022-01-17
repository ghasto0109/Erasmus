#include "stdafx.h"
#include "CameraCapture.h"

#include "Erasmus/QPictureBox.h"

#include <opencv2/highgui/highgui.hpp>
#include <numcpp/opencv.h>

using namespace std;

CameraCapture::CameraCapture() : 
	widget(nullptr), 
	_running(false), 
	CameraID("CameraID", 0), 

	Start("Start", this, &CameraCapture::start), 
	Stop("Stop", this, &CameraCapture::stop)
{
	widget = new QPictureBox();
}

CameraCapture::~CameraCapture()
{
	if (_thread.joinable())
	{
		_running = false;
		_thread.join();
	}
}

void CameraCapture::accept(property_visitor &visit)
{
	visit(CameraID);

	visit(Start);
	visit(Stop);
}

void CameraCapture::start()
{
	cout << "[CameraCapture] start" << endl;

	DidCaptureFrame += [this](const np::Array<uint8_t, 2> &frame)
	{
		widget->setImage(frame);
	};

	_thread = std::thread(&CameraCapture::run, this);
}

void CameraCapture::stop()
{
	cout << "[CameraCapture] stop" << endl;

	if (_thread.joinable())
	{
		_running = false;
		_thread.join();
	}
}

void CameraCapture::run()
{
	cout << "[CameraCapture] run" << endl;

	cv::VideoCapture camera(CameraID);

	if (!camera.isOpened())
	{
		cout << "Error: cannot open camera" << endl;
		return;
	}

	cv::Mat cv_frame, cv_frame_grayscale;
	np::Array<uint8_t, 2> frame;

	_running = true;
	while (_running) 
	{
		if (!camera.read(cv_frame))
		{
			cout << "[CameraCapture] Error: cannot read frame from camera." << endl;
			return;
		}

		cv::cvtColor(cv_frame, cv_frame_grayscale, CV_BGR2GRAY);
		np::from_cv_mat(frame, cv_frame_grayscale);
		// cout << frame.size(0) << " " << frame.size(1) << endl;

		DidCaptureFrame(frame);
	}
}