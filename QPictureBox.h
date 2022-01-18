#ifndef QPICTURE_BOX_H_
#define QPICTURE_BOX_H_

#include <QtWidgets>

#include <functional>
#include <numcpp/array.h>

class QPictureBox : public QWidget
{
public:
	QPictureBox(QWidget *parent = 0);

	void setImage(const np::Array<uint8_t, 2> &image);
	void setInverted(bool inverted);

	// signal
	std::function<void(Qt::MouseButton button)> onClicked;

protected:
	void paintEvent(QPaintEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;

private:
	QImage _qimage;
	bool _inverted;

	QVector<QRgb> _standardColorTable, _invertedColorTable;
};

#endif // QPICTURE_BOX_H_