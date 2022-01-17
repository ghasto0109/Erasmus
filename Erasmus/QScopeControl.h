#ifndef QSCOPE_CONTROL_H_
#define QSCOPE_CONTROL_H_

#include <ActiveQt/QAxWidget>

__interface IScopePX14;
struct tagSAFEARRAY;

class QScopeControl : public QAxWidget
{
public:
	QScopeControl();
	~QScopeControl();

public:
	bool AddChannel(const std::string channelName, int channelSize);
	bool SetChannelBuffer(int channelIndex, uint16_t *buffer, int count);
	bool SetSampleValueRange(int minValue, int maxValue);

private:
	IScopePX14 *_comInterface;
	std::vector<struct tagSAFEARRAY *> _scopeBufferList;
};

#endif // QSCOPE_CONTROL_H_