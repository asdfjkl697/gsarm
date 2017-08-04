#ifndef ICAMERAHANDLER_H_
#define ICAMERAHANDLER_H_

#include "typedef.h"

class ICameraHandler
{
public:
	virtual bool OnDealwithAlarm(long lLoginID, char *pBuf,unsigned long dwBufLen)=0;
	virtual void OnDisconnctCallback(long lLoginID){};
	virtual void OnSearchCallback(IPCAMERABROADCAST *ipcamera){};
};

class ICameraAlarmRecv
{
public:
	virtual void OnCameraAlarmRecv( const bool isAlarm, const IPCameraType CameraType, const char *sDeviceIP, const int nPort, const int channel, const char *alarmstr )=0;
};

#endif
