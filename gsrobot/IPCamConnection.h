#ifndef IPCAMCONNECTION_H
#define IPCAMCONNECTION_H

#include "IDeviceHandler.h"
#include "CameraControl.h"
#include "gloox/jid.h"
#include "IPCameraBase.h"
#include "ICameraHandler.h"
#include "IPCameraManager.h"

using namespace gloox;

class IPCamConnection:public ICameraHandler
{
private:
	bool isRunning;
	IDeviceHandler *m_handler;
	IPCameraManager *m_camManager;

public:
	IPCamConnection(IDeviceHandler *handler/*jyc20170511 remove, ICameraAlarmRecv *hAlarmRecv*/);
	~IPCamConnection(void);

	IPCameraManager* GetCameraManager()
	{
		return m_camManager;
	}

	bool OnDealwithAlarm(long lLoginID, char *pBuf,unsigned long dwBufLen);
	void OnDisconnctCallback(long lLoginID);
	
    void SearchInit(IPCameraType camType, ICameraHandler *handler);
    void SearchStart(IPCameraType camType);
    void SearchStop(IPCameraType camType);

	int AddIPCamera(IPCameraBase *ipcam, uint32_t enable=1, GSIOTDevice **outNewDev=NULL);
	bool RemoveIPCamera( GSIOTDevice *iotdevice );
	bool ModifyDevice( GSIOTDevice *iotdevice, uint32_t ModifySubFlag=0 );
	GSIOTDevice* GetIOTDevice( uint32_t deviceId ) const;

    void Connect();
	void Disconnect();
	defGSReturn ReConnect( GSIOTDevice *iotdevice, bool reconnectAudio, bool doforce );
	bool SendPTZ( int id, GSPTZ_CtrlCmd command, int param1=0, int param2=0, int speed=0, const char *callinfo=NULL );
	int GetPTZState(int id, GSPTZ_CtrlCmd command, bool refresh_device);
	defGSReturn PushToRTMPServer( const JID& jid,int id, const std::string& url, const std::vector<std::string> &url_backup );
	void StopRTMPSend(const JID& jid,int id);
	IPCameraBase *GetCamera(int id);
	void CheckRTMPSession( bool CheckNow );
	uint32_t UpdateRTMPSession(const JID& jid,int id);
	GSIOTDevice* GetLastPublishCamDev( const std::string& strjid );
};

#endif
