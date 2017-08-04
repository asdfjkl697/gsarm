#include "IPCamConnection.h"
#include <stdio.h>
#include "gloox/util.h"
#include "gsiot/GSIOTDevice.h"
#include "CameraPTZ.h"
#include "common.h"

//IPCamConnection::IPCamConnection(IDeviceHandler *handler, ICameraAlarmRecv *hAlarmRecv)
IPCamConnection::IPCamConnection(IDeviceHandler *handler) //jyc20170511 modify
{
	isRunning = false;
	this->m_handler = handler;
	m_camManager = new IPCameraManager();
	//TI368Camera::InitSDK(this);
	//SSD1935Camera::InitSDK(this);
	//HikCamera::InitSDK(this, hAlarmRecv);  //jyc20170323 remove
}

IPCamConnection::~IPCamConnection(void)
{
	LOGMSG("~IPCamConnection\r\n");
	isRunning = false;
	//TI368Camera::Cleanup();
	//SSD1935Camera::Cleanup();
	//HikCamera::Cleanup();

	delete(m_camManager);
}

bool IPCamConnection::OnDealwithAlarm(long lLoginID, char *pBuf,unsigned long dwBufLen)
{
	return true;
}

void IPCamConnection::OnDisconnctCallback(long lLoginID)
{
	std::list<GSIOTDevice *> cameraList = m_camManager->GetCameraList();
	std::list<GSIOTDevice *>::const_iterator it = cameraList.begin();
	for(;it!=cameraList.end();it++){
		IPCameraBase *cam = (IPCameraBase *)(*it)->getControl();
		if(cam && cam->GetCameraHWND() == lLoginID){
			while(1){
			    cam->OnDisconnct();

				if( !cam->isAutoConnect() )
				{
					break;
				}

				const defGSReturn ConnectRet = cam->Connect(false);
				if( macGSSucceeded(ConnectRet) )
				{
				   break;
				}

				sleep(10); //Sleep(10000);  //sleep 10s
			}
			return;
		}
	}
}

void IPCamConnection::SearchInit(IPCameraType camType, ICameraHandler *handler)
{
	switch(camType){
	case SSD1935:
		//SSD1935Camera::SearchInit(handler);
		break;
	case TI368:
		//TI368Camera::SearchInit(handler);
		break;
	}
}
void IPCamConnection::SearchStart(IPCameraType camType)
{
	switch(camType){
	case SSD1935:
		//SSD1935Camera::SearchStart();
		break;
	case TI368:
		//TI368Camera::SearchStart();
		break;
	}
}
void IPCamConnection::SearchStop(IPCameraType camType)
{
	switch(camType){
	case SSD1935:
		//SSD1935Camera::SearchStop();
		break;
	case TI368:
		//TI368Camera::SearchStop();
		break;
	}
}

int IPCamConnection::AddIPCamera(IPCameraBase *ipcam, uint32_t enable, GSIOTDevice **outNewDev)
{
	int retadd = m_camManager->AddIPCamera(ipcam);
	if( retadd < 0 )
		return retadd;

	GSIOTDevice *dev = m_camManager->GetCameraList().back();

	if( outNewDev )
	{
		*outNewDev = dev;
	}

	if( dev->GetEnable() )
	{
		if( ipcam->isAutoConnect() )
		{
			ipcam->Connect(true);
		}
	}

	if(m_handler!=NULL){
		m_handler->OnDeviceConnect(dev);
	}

	return 1;
}

void IPCamConnection::Connect()
{
	isRunning = true;

	//��ȡ��ƵIP�б�XML��������������ɨ��
	std::list<GSIOTDevice *> cameraList = m_camManager->GetCameraList();
	std::list<GSIOTDevice *>::const_iterator it = cameraList.begin();
	for(;it!=cameraList.end();it++){
		IPCameraBase *cam = (IPCameraBase *)(*it)->getControl();
		if(cam){

			if( (*it)->GetEnable() )
			{
				//cam->PreinitBufCheck();
				if( cam->isAutoConnect() )
				{
					cam->Connect(true);
				}
			}
			else
			{
				//LOGMSGEX( defLOGNAME, defLOG_WORN, "cam(%s) is disable!\r\n", cam->GetName().c_str() );
				//continue;
			}
			
			if(m_handler!=NULL)
{
				m_handler->OnDeviceConnect(*it);
			}
		}
	}
}

void IPCamConnection::Disconnect()
{
	isRunning = false;
	std::list<GSIOTDevice *> cameraList = m_camManager->GetCameraList();
	std::list<GSIOTDevice *>::const_iterator it = cameraList.begin();
	for(;it!=cameraList.end();it++){
		IPCameraBase *cam = (IPCameraBase *)(*it)->getControl();
		if(cam){
			cam->StopRTMPSendAll();
			cam->OnDisconnct();
		}
	}
}

defGSReturn IPCamConnection::ReConnect( GSIOTDevice *iotdevice, bool reconnectAudio, bool doforce )
{
	if( !iotdevice->GetEnable() )
	{
		//LOGMSGEX( defLOGNAME, defLOG_WORN, "cam ReConnect(%s) is disable, no continue!\r\n", iotdevice->getName().c_str() );

		return defGSReturn_ObjDisable;
	}

	LOGMSG( "cam ReConnect(%s), reconnectAudio=%d\r\n", iotdevice->getName().c_str(), reconnectAudio );

	IPCameraBase *cam_ctl = (IPCameraBase*)iotdevice->getControl();
	if( !cam_ctl )
	{
		return defGSReturn_ErrConfig;
	}

	cam_ctl->StopRTMPSendAll();
	cam_ctl->OnDisconnct();
	sleep(1);//Sleep(1000);
	
	if( doforce || cam_ctl->isAutoConnect() )
	{
		const defGSReturn ConnectRet = cam_ctl->Connect(reconnectAudio);

		if( macGSFailed(ConnectRet) )
		{
			return ConnectRet;
		}
	}
	
	return defGSReturn_Success;
}
	
defGSReturn IPCamConnection::PushToRTMPServer( const JID& jid,int id, const std::string& url, const std::vector<std::string> &url_backup )
{
	std::list<GSIOTDevice *> &cameraList = m_camManager->GetCameraList();
	std::list<GSIOTDevice *>::const_iterator it = cameraList.begin();
	for(;it!=cameraList.end();it++){
		if((*it)->getId() == id){
			IPCameraBase *cam = (IPCameraBase *)(*it)->getControl();
			if(cam){
				return cam->SendToRTMPServer( jid, url, url_backup );
			}
			break;
		}
	}

	return defGSReturn_NoExist;
}
	
void IPCamConnection::StopRTMPSend(const JID& jid,int id)
{
	std::list<GSIOTDevice *> &cameraList = m_camManager->GetCameraList();
	std::list<GSIOTDevice *>::const_iterator it = cameraList.begin();
	for(;it!=cameraList.end();it++){
		if((*it)->getId() == id){
			IPCameraBase *cam = (IPCameraBase *)(*it)->getControl();
			if(cam){
				cam->StopRTMPSend(jid);
			}
			break;
		}
	}
}

bool IPCamConnection::SendPTZ( int id, GSPTZ_CtrlCmd command, int param1, int param2, int speed, const char *callinfo )
{
	std::list<GSIOTDevice *> &cameraList = m_camManager->GetCameraList();
	std::list<GSIOTDevice *>::const_iterator it = cameraList.begin();
	for(;it!=cameraList.end();it++){
		if((*it)->getId() == id){
			IPCameraBase *cam = (IPCameraBase *)(*it)->getControl();
			if(cam){
				return cam->SendPTZ( command, param1, param2, speed, callinfo );
			}
			break;
		}
	}

	return false;
}

int IPCamConnection::GetPTZState(int id, GSPTZ_CtrlCmd command, bool refresh_device)
{
	std::list<GSIOTDevice *> &cameraList = m_camManager->GetCameraList();
	std::list<GSIOTDevice *>::const_iterator it = cameraList.begin();
	for(;it!=cameraList.end();it++){
		if((*it)->getId() == id){
			IPCameraBase *cam = (IPCameraBase *)(*it)->getControl();
			if(cam){
				return cam->GetPTZState(command, refresh_device);
			}
			break;
		}
	}

	return 0;
}

IPCameraBase *IPCamConnection::GetCamera(int id)
{
    std::list<GSIOTDevice *> &cameraList = m_camManager->GetCameraList();
	std::list<GSIOTDevice *>::const_iterator it = cameraList.begin();
	for(;it!=cameraList.end();it++){
		if((*it)->getId() == id){
			return (IPCameraBase *)(*it)->getControl();
		}
	}
    return NULL;
}

void IPCamConnection::CheckRTMPSession( const bool CheckNow )
{
	if( CheckNow )
	{
		LOGMSG( "IPCamConnection::CheckRTMPSession CheckNow\r\n" );
	}
	/* jyc20170405 remove jyc20170512 resume */
	std::list<GSIOTDevice *> &cameraList = m_camManager->GetCameraList();
	std::list<GSIOTDevice *>::const_iterator it = cameraList.begin();
	for(;isRunning && it!=cameraList.end();it++){		
		IPCameraBase *cam = (IPCameraBase *)(*it)->getControl();
		if(cam)
		{
			if( cam->CheckSession( (*it)->GetEnable(), false, CheckNow ) )
			{
				if( isRunning && m_handler )
				{
					m_handler->OnDeviceNotify( defDeviceNotify_StateChanged, (*it), NULL );
				}
			}
		}
	}
}

uint32_t IPCamConnection::UpdateRTMPSession(const JID& jid,int id)
{
	std::list<GSIOTDevice *> &cameraList = m_camManager->GetCameraList();
	std::list<GSIOTDevice *>::const_iterator it = cameraList.begin();
	for(;it!=cameraList.end();it++){	
		if((*it)->getId() == id){
			IPCameraBase *cam = (IPCameraBase *)(*it)->getControl();
			if(cam && cam->IsPublish()){
				return cam->UpdateSession(jid);
			}
			break;
		}
	}

	return 0;
}

bool IPCamConnection::RemoveIPCamera( GSIOTDevice *iotdevice )
{
	iotdevice->SetEnable(0);
	if(m_camManager)
	{
		if( IOT_DEVICE_Camera == iotdevice->getType() )
		{
			IPCameraBase *cam = (IPCameraBase*)iotdevice->getControl();

			cam->StopRTMPSendAll();
			cam->OnDisconnct();
		}

		if( m_camManager->RemoveIPCamera( iotdevice ) )
		{
			m_handler->OnDeviceDisconnect(iotdevice);
			delete iotdevice;
			return true;
		}
	}

	return false;
}

bool IPCamConnection::ModifyDevice(GSIOTDevice *iotdevice, uint32_t ModifySubFlag)
{
	if(m_camManager)
	{
		if( m_camManager->ModifyDevice( iotdevice, ModifySubFlag ) )
		{
			IPCameraBase *cam = (IPCameraBase*)iotdevice->getControl();
			if( iotdevice->GetEnable() )
			{
				if( !cam->IsConnect() )
				{
					if( cam->isAutoConnect() )
					{
						cam->Connect(true);
					}
					else
					{
						cam->check_NetUseable();
					}
				}
			}
			else
			{
				cam->OnDisconnct();
				cam->StopRTMPSendAll();
			}

			if( m_handler )
			{
				m_handler->OnDeviceNotify( defDeviceNotify_Modify, iotdevice, NULL );
			}

			return true;
		}
	}

	return false;
}

GSIOTDevice* IPCamConnection::GetIOTDevice( uint32_t deviceId ) const
{
	return m_camManager->GetIOTDevice(deviceId);
}

GSIOTDevice* IPCamConnection::GetLastPublishCamDev( const std::string& strjid )
{
	std::list<GSIOTDevice*> &deviceList = m_camManager->GetCameraList();
	if( deviceList.empty() )
		return NULL;

	GSIOTDevice *pDevLast = NULL;
	uint32_t last_publish_tick = 0;
	for( std::list<GSIOTDevice*>::const_iterator it = deviceList.begin(); it!=deviceList.end(); ++it )
	{
		GSIOTDevice *pTempDev = (*it);

		if( !pTempDev->GetEnable() )
			continue;

		if( IOT_DEVICE_Camera != pTempDev->getType() )
			continue;

		if( !pTempDev->getControl() )
			continue;

		IPCameraBase *pTempCam = (IPCameraBase*)pTempDev->getControl();
		if( !pTempCam->IsConnect() )
			continue;

		if( !pTempCam->GetStreamObj() )
			continue;

		if( !pTempCam->GetStreamObj()->IsPublish() )
			continue;

		if( !pTempCam->GetStreamObj()->GetRTMPSendObj() )
			continue;

		uint32_t cur_tick = pTempCam->GetStreamObj()->GetRTMPSendObj()->get_startTime_tick();

		if( !strjid.empty() )
		{
			const uint32_t SessionStartTime = pTempCam->GetSessionStartTime( strjid );
			if( 0==SessionStartTime )
				continue;

			cur_tick = SessionStartTime;
		}

		if( pDevLast )
		{
			if( cur_tick > last_publish_tick )
			{
				pDevLast = pTempDev;
				last_publish_tick = cur_tick;
			}
		}
		else
		{
			pDevLast = pTempDev;
			last_publish_tick = cur_tick;
		}
	}

	return pDevLast;
}
