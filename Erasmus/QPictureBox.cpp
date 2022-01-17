#include "stdafx.h"
#include "QPictureBox.h"

QPictureBox::QPictureBox(QWidget *parent) : QWidget(parent), 
	_inverted(false), 
	_standardColorTable(256), 
	_invertedColorTable(256)
{ 
	for (int i = 0; i < 256; i++)
		_standardColorTable[i] = 0xFF000000 + i * 0x010101; 

	for (int i = 0; i < 256; i++)
		_invertedColorTable[i] = 0xFF000000 + (255 - i) * 0x010101; // Inverted color

    _qimage = QImage(256, 256, QImage::Format_Indexed8);
	_qimage.setColorTable(_inverted ? _invertedColorTable : _standardColorTable);
}

void QPictureBox::setImage(const np::Array<uint8_t, 2> &image)
{
	// lazy initialize
	if (_qimage.width() != image.size(0) || _qimage.height() != image.size(1))
	{
		_qimage = QImage(image.size(0), image.size(1), QImage::Format_Indexed8);
		_qimage.setColorTable(_inverted ? _invertedColorTable : _standardColorTable);
	}

	uchar *targetBuffer = _qimage.bits();
	memcpy(targetBuffer, image, byteSize(image));

	update();
}

void QPictureBox::setInverted(bool inverted)
{
	_inverted = inverted;

	if (!_qimage.isNull())
	{
		_qimage.setColorTable(_inverted ? _invertedColorTable : _standardColorTable);
		update();
	}
}

void QPictureBox::paintEvent(QPaintEvent *event)
{
	// Flip y
	QTransform transform;
	transform.translate(0, 0);
	transform.scale(1, 1);

	QPainter painter(this);

	painter.setTransform(transform);
	painter.drawImage(QRect(0, 0, width(), height()), _qimage);
	painter.resetTransform();

	painter.end();
}

void QPictureBox::mousePressEvent(QMouseEvent *event)
{
	if (onClicked)
		onClicked(event->button());
}