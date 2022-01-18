#include "stdafx.h"
#include "QScopeControl.h"

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include "IScopePX14.h"

#include <QtCore/QUuid>

// Define a module so we can use VC++ attributes.
[module(type=exe, name = "Erasmus2", version = "1.0")];

QScopeControl::QScopeControl() :
#ifdef _WIN64
	QAxWidget(QString::fromUtf8("PX14UI64.ScopeCtrl"), 0), 
#else
	QAxWidget(QString::fromUtf8("PX14UI.ScopeCtrl"), 0), 
#endif
	_comInterface(NULL)
{
#ifdef _WIN64
	int result = queryInterface(QUuid("59E8B91D-4D18-4AF8-B26E-9B96BAFCF709"), reinterpret_cast<void **>(&_comInterface));
#else
	int result = queryInterface(QUuid("80B6F618-9585-4653-A4ED-3C59D064D02F"), reinterpret_cast<void **>(&_comInterface));
#endif
	if (SUCCEEDED(result))
		SetSampleValueRange(0, 65535);
}

QScopeControl::~QScopeControl()
{
	if (_comInterface) { _comInterface->Release(); _comInterface = NULL; }
}

bool QScopeControl::AddChannel(const std::string channelName, int channelSize)
{
	if (NULL == _comInterface) return false;

	SAFEARRAY *scopeBuffer;
	UINT scopeSrcId;

	// Channel source
	scopeBuffer = SafeArrayCreateVector(VT_UI2, 0, channelSize);

	VARIANTARG varg;
	varg.vt = VT_UI2 | VT_ARRAY;
	varg.parray = scopeBuffer;

	CComBSTR bstrTitle(channelName.c_str());
	if (SUCCEEDED(_comInterface->AddBufferChannelSource(bstrTitle, varg, 1, channelSize, false, &scopeSrcId)))
	{
		_scopeBufferList.push_back(scopeBuffer);
		return true;
	}
	else
		return false;
}

bool QScopeControl::SetChannelBuffer(int channelIndex, uint16_t *buffer, int count)
{
	if (NULL == _comInterface) return false;

	SAFEARRAY *scopeBuffer = _scopeBufferList[channelIndex];
	unsigned short* rawp;

	SafeArrayAccessData(scopeBuffer, reinterpret_cast<void**>(&rawp));
	
	(rawp, buffer, sizeof(uint16_t) * count);
	SafeArrayUnaccessData(scopeBuffer);

	_comInterface->RefreshScope();
	return true;
}

bool QScopeControl::SetSampleValueRange(int minValue, int maxValue)
{
	if (NULL == _comInterface) return false;

	return SUCCEEDED(_comInterface->put_MinSampleValue(minValue)) && 
		SUCCEEDED(_comInterface->put_MaxSampleValue(maxValue));
}
