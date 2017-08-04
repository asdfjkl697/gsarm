#include "gloox/message.h" //jyc20170222 add
#include "gloox/rostermanager.h" //jyc20170224 add

#include "GSIOTClient.h"
#include "GSIOTInfo.h"
#include "GSIOTDevice.h"
#include "GSIOTControl.h"
#include "GSIOTDeviceInfo.h"
#include "GSIOTHeartbeat.h"
#include "../RS485DevControl.h"
#include "../RFRemoteControl.h"
#include "../xmpp/XmppGSResult.h"
#include "../AutoEventthing.h" //jyc20170222 resume
#include "../xmpp/XmppGSMessage.h"
#include "../xmpp/XmppRegister.h"
#include "../xmpp/XmppGSChange.h"

//#include "../APlayer.h" //jyc20170330 resume jyc20170606 remove

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>   

#include <pthread.h> //jyc20160922

#define defPlayback_SessionName "Playback@GSIOT.gsss.cn"  //jyc20170405 resume
#define defcheckNetUseable_timeMax (5*60*1000)

//#define defForceDataSave  //jyc20170228 debug

static int sg_blUpdatedProc = 0;  //jyc20170318 resume

namespace httpreq  //jyc20170318 resume
{
	#include "../HttpRequest.cpp"
}

std::string g_IOTGetVersion()
{
	return std::string(GSIOT_VERSION);
}

std::string g_IOTGetBuildInfo()
{
	std::string str;
	str += __DATE__;
	str += " ";
	str += __TIME__;
	return str;
}

#ifdef _DEBUG
#define macDebugLog_AlarmGuardState
//#define macDebugLog_AlarmGuardState LOGMSG
#else
#define macDebugLog_AlarmGuardState
#endif

// is valid guard
bool g_IsValidCurTimeInAlarmGuardState()
{
	// always guard
	if( IsRUNCODEEnable(defCodeIndex_TEST_DisableAlarmGuard) )
	{
		macDebugLog_AlarmGuardState( "AlarmGuardTime Disabled!" );
		return true;
	}

	// now time
	SYSTEMTIME st;
	memset( &st, 0, sizeof(st) );
	//::GetLocalTime(&st);
	GetLocalTime(&st);

	const int agCurTime = st.wHour*100 + st.wMinute;
	const int w = (0==st.wDayOfWeek) ? 7:st.wDayOfWeek;

	// 
	const defCodeIndex_ wIndex = g_AlarmGuardTimeWNum2Index(w);
	if( defCodeIndex_Unknown == wIndex )
	{
		macDebugLog_AlarmGuardState( "AlarmGuard Time w%d: w err invalid", w );
		return false;
	}

	const int ad = RUNCODE_Get(wIndex);
	if( defAlarmGuardTime_AllDay==ad )
	{
		macDebugLog_AlarmGuardState( "AlarmGuard Time w%d: flag fullday valid", w );
		return true;
	}
	else if( defAlarmGuardTime_UnAllDay==ad )
	{
		macDebugLog_AlarmGuardState( "AlarmGuard Time w%d: flag fullday invalid", w );
		return false;
	}
	
	std::vector<uint32_t> vecFlag;
	std::vector<uint32_t> vecBegin;
	std::vector<uint32_t> vecEnd;
	g_GetAlarmGuardTime( wIndex, vecFlag, vecBegin, vecEnd );

	macDebugLog_AlarmGuardState( "AlarmGuard Time w%d: inte: ag1(%d,%d-%d), ag2(%d,%d-%d), ag3(%d,%d-%d)",
		w,
		vecFlag[0], vecBegin[0], vecEnd[0],
		vecFlag[1], vecBegin[1], vecEnd[1],
		vecFlag[2], vecBegin[2], vecEnd[2] );

	// alarm guard time
	for( int i=0; i<defAlarmGuard_AGTimeCount; ++i )
	{
		if( !vecFlag[i] )
			continue;

		const int agTime_Begin = vecBegin[i];
		const int agTime_End = vecEnd[i];

		if( agTime_Begin == agTime_End )
		{
		}
		else if( agTime_Begin < agTime_End )
		{
			if( agCurTime >= agTime_Begin && agCurTime <= agTime_End )
			{
				macDebugLog_AlarmGuardState( "AlarmGuard Time w%d:close inte valid: cur=%d, ag(%d-%d)", w, agCurTime, agTime_Begin, agTime_End );
				return true;
			}
		}
		else
		{
			if( agCurTime >= agTime_Begin || agCurTime <= agTime_End )
			{
				macDebugLog_AlarmGuardState( "AlarmGuard Time w%d:open inte valid: cur=%d, ag(%d-%d)", w, agCurTime, agTime_Begin, agTime_End );
				return true;
			}
		}
	}

	macDebugLog_AlarmGuardState( "AlarmGuard Time w%d: invalid inte: cur=%d", w, agCurTime );
	return false;
}

//unsigned __stdcall PlaybackProcThread(LPVOID lpPara)
void *PlaybackProc_Thread(LPVOID lpPara)
{
	GSIOTClient *client = (GSIOTClient*)lpPara;

	uint32_t tick = 0;

	CHeartbeatGuard hbGuard( "Playback" );

	while( client->is_running() )
	{
		hbGuard.alive();

		if( !client->PlaybackCmd_OnProc() )
		{
			if( timeGetTime()-tick > 180000 )
			{
				client->Playback_ThreadPrinthb();
				tick = timeGetTime();
			}

			usleep(10000); //ok
		}
	}

	//LOGMSGEX( defLOGNAME, defLOG_SYS, "PlaybackProcThread exit." );
	client->OnPlayBackThreadExit();
	pthread_detach(pthread_self()); //jyc20170722 add
	//return 0;
}

//unsigned __stdcall PlayMgrProcThread(LPVOID lpPara)
void *PlayMgrProc_Thread(LPVOID lpPara)
{
	//CoInitialize(NULL);

	GSIOTClient *client = (GSIOTClient*)lpPara;
	uint32_t tick = 0;
	CHeartbeatGuard hbGuard( "PlayMgr" );
	while( client->is_running() )
	{
		hbGuard.alive();
		if( !client->PlayMgrCmd_OnProc() )
		{
			const bool CheckNow = client->PlayMgrCmd_IsCheckNow();
			if( CheckNow || timeGetTime()-tick > 15000 )
			{
				client->PlayMgrCmd_SetCheckNow( false );
				client->GetIPCameraConnection()->CheckRTMPSession( CheckNow );
				sleep(1);
				tick = timeGetTime();
			}
			client->check_all_NetUseable( CheckNow );
			client->check_all_devtime();
			client->check_all_devtime_proc();

			usleep(10000);//ok
		}
	}

	//LOGMSGEX( defLOGNAME, defLOG_SYS, "PlayMgrProcThread exit." );
	client->OnPlayMgrThreadExit();
	pthread_detach(pthread_self()); //jyc20170722 add
	//return 0;
}

//jyc20160922 add three thread 
//unsigned __stdcall DataProcThread(LPVOID lpPara)
void *DataProc_Thread(LPVOID lpPara)
{
	GSIOTClient *client = (GSIOTClient*)lpPara;
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "DataProcThread Running..." );
	client->GetDataStoreMgr( true );
	CHeartbeatGuard hbGuard( "DataProc" );
	DWORD dwCheckStat = ::timeGetTime();
	while( client->is_running() )
	{
		hbGuard.alive();
		client->DataSave();
		client->DataProc();
		DWORD dwStart = ::timeGetTime();
		while( client->is_running() && ::timeGetTime()-dwStart < 5*1000 )
		{
			usleep(1000);
			client->DataSave();
		}
		if( ::timeGetTime()-dwCheckStat > 60*1000 )
		{
			client->DataStatCheck();
			dwCheckStat = ::timeGetTime();
		}
	}
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "DataProcThread exit." );
	client->OnDataProcThreadExit();
	pthread_detach(pthread_self()); //jyc20170722 add
	//return 0;
}

//unsigned __stdcall AlarmProcThread(LPVOID lpPara)
void *AlarmProc_Thread(LPVOID lpPara)
{
	GSIOTClient *client = (GSIOTClient*)lpPara;
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "AlarmProcThread Running..." );
	CHeartbeatGuard hbGuard( "AlarmProc" );
	DWORD dwStart = ::timeGetTime();
	while( client->is_running() )
	{
		if( ::timeGetTime()-dwStart > 10000 )
		{
			client->AlarmCheck();
			hbGuard.alive();
			dwStart = ::timeGetTime();
		}
		const bool isdo = client->AlarmProc();
		if( !isdo && client->is_running() )
		{
			usleep(50000);
		}
	}
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "AlarmProcThread exit." );
	client->OnAlarmProcThreadExit();
	pthread_detach(pthread_self()); //jyc20170722 add
	//return 0;
}

//jyc20170405 resume 
struPlaybackSession::struPlaybackSession()
	: dev( NULL )
{
	ts = timeGetTime();
	lastUpdateTS = ts;
}

struPlaybackSession::struPlaybackSession( const std::string &in_key, const std::string &in_from_jid, const std::string &in_url, const std::string &in_peerid, const std::string &in_streamid, const std::string &in_devname, GSIOTDevice *in_dev )
	: key( in_key ), from_jid( in_from_jid ), url( in_url ), peerid( in_peerid ), streamid( in_streamid ), devname( in_devname ), dev( in_dev )
{
	ts = timeGetTime();
	lastUpdateTS = ts;
}

bool struPlaybackSession::check() const
{
	const uint32_t overtime = RUNCODE_Get(defCodeIndex_SYS_PlaybackSessionOvertime);

	if( timeGetTime()-ts > (1000*overtime) )
	{
		LOGMSG( "Playback Session overtime=%ds, devid=%d, devname=%s, url=%s, from=%s",
				overtime, dev->getId(), dev->getName().c_str(), url.c_str(), from_jid.c_str() );
		return false;
	}

	return true;
} //jyc20170405 resume until here

GSIOTClient::GSIOTClient(IDeviceHandler *handler, const std::string &RunParam) :
		m_parser(this), m_PreInitState(false), m_cfg(NULL), m_event(NULL), timer(NULL),
		deviceClient(NULL), ipcamClient(NULL), xmppClient(NULL), timeCount(0),
		serverPingCount(0), m_handler(handler), m_running(false), m_IGSMessageHandler(NULL),
		m_ITriggerDebugHandler(NULL), m_EnableTriggerDebug(false), m_last_checkNetUseable_camid(0)
{
	g_SYS_SetGSIOTClient( this ); 
	this->PreInit( RunParam );
	
	m_DataStoreMgr = NULL;
}

void GSIOTClient::PreInit( const std::string &RunParam )
{
    //CoInitialize(NULL);
	if( !m_RunCodeMgr.get_db() )
	{
		m_PreInitState = false;
		return ;
	}

	m_IOT_starttime = g_GetUTCTime();
	m_str_IOT_starttime = g_TimeToStr( m_IOT_starttime ); //jyc20170224 modify
	//m_str_IOT_starttime = g_TimeToStr( m_IOT_starttime , defTimeToStrFmt_UTC );

	printf("Start Time(%u): %s\r\n", (uint32_t)m_IOT_starttime, m_str_IOT_starttime.c_str());

	m_cfg = new GSIOTConfig(); 
	if( !m_cfg->PreInit( RunParam ) )
	{
		m_PreInitState = false;
		return ;
	}

	this->RunCodeInit(); //have a trouble in runcode.cpp jyc20160823
	m_event = new GSIOTEvent();
	ResetNoticeJid();

//	APlayer::Init(); //jyc20170606 remove
	this->m_TalkMgr.set_ITalkNotify(this);
	//this->m_TalkMgr.set_ITalkNotify(nullptr); //jyc2017527 modify this->nullptr

	m_PreInitState = true;
	m_isThreadExit = true;
	m_isPlayBackThreadExit = true; //jyc20170405 resume
	m_isPlayMgrThreadExit = true;
	m_PlaybackThreadTick = timeGetTime();
	m_PlaybackThreadCreateCount = 0;

	m_PlayMgr_CheckNowFlag = defPlayMgrCmd_Unknown;  //jyc20170405 resume until here

	m_last_checkNetUseable_time = timeGetTime();

	m_isDataProcThreadExit = true;
	m_isAlarmProcThreadExit = true;

	m_lastShowLedTick = timeGetTime();
	m_lastcheck_AC = timeGetTime();
	m_isACProcThreadExit = true;
}

void GSIOTClient::ResetNoticeJid()
{

}

void GSIOTClient::RunCodeInit()
{
	m_RunCodeMgr.Init();
}

void GSIOTClient::Stop(void)
{
	DWORD dwStart = ::timeGetTime();
	m_running = false;
	
	dwStart = ::timeGetTime();
	while( ::timeGetTime()-dwStart < 10*1000 )
	{
		if( m_isThreadExit 
			&& m_isPlayBackThreadExit
			&& m_isPlayMgrThreadExit
			&& m_isDataProcThreadExit
			&& m_isAlarmProcThreadExit
			&& m_isACProcThreadExit
			&& (!timer || timer->IsThreadExit() )
			&& (!deviceClient || deviceClient->IsThreadExit() )  //jyc20170405 resume
			)
		{
			break;
		}
		usleep(1000);
	}
	//printf( "~GSIOTClient: thread exit wait usetime=%dms\r\n", ::timeGetTime()-dwStart );
}

GSIOTClient::~GSIOTClient(void)  //jyc20170302 modify 
{
	if(xmppClient){
		xmppClient->disconnect();

	   xmppClient->removeStanzaExtension(ExtIot);
	   xmppClient->removeIqHandler(this, ExtIot);
	   xmppClient->removeStanzaExtension(ExtIotResult);
	   xmppClient->removeIqHandler(this, ExtIotResult);
	   xmppClient->removeStanzaExtension(ExtIotControl);
       xmppClient->removeIqHandler(this, ExtIotControl);
	   xmppClient->removeStanzaExtension(ExtIotHeartbeat);
	   xmppClient->removeIqHandler(this, ExtIotHeartbeat);
	   xmppClient->removeStanzaExtension(ExtIotDeviceInfo);
       xmppClient->removeIqHandler(this, ExtIotDeviceInfo);
	   xmppClient->removeStanzaExtension(ExtIotAuthority);
	   xmppClient->removeIqHandler(this, ExtIotAuthority);
	   xmppClient->removeStanzaExtension(ExtIotAuthority_User);
	   xmppClient->removeIqHandler(this, ExtIotAuthority_User);
	   xmppClient->removeStanzaExtension(ExtIotManager);
	   xmppClient->removeIqHandler(this, ExtIotManager);
	   xmppClient->removeStanzaExtension(ExtIotEvent);
	   xmppClient->removeIqHandler(this, ExtIotEvent);
	   xmppClient->removeStanzaExtension(ExtIotState);
	   xmppClient->removeIqHandler(this, ExtIotState);
	   xmppClient->removeStanzaExtension(ExtIotChange);
	   xmppClient->removeIqHandler(this, ExtIotChange);
	   xmppClient->removeStanzaExtension(ExtIotTalk);
	   xmppClient->removeIqHandler(this, ExtIotTalk);
	   xmppClient->removeStanzaExtension(ExtIotPlayback);
	   xmppClient->removeIqHandler(this, ExtIotPlayback);
	   xmppClient->removeStanzaExtension(ExtIotRelation);
	   xmppClient->removeIqHandler(this, ExtIotRelation);
	   xmppClient->removeStanzaExtension(ExtIotPreset);
	   xmppClient->removeIqHandler(this, ExtIotPreset);
	   xmppClient->removeStanzaExtension(ExtIotVObj);
	   xmppClient->removeIqHandler(this, ExtIotVObj);
	   //xmppClient->removeStanzaExtension(ExtIotTrans);
	   //xmppClient->removeIqHandler(this, ExtIotTrans);
	   xmppClient->removeStanzaExtension(ExtIotReport);
	   xmppClient->removeIqHandler(this, ExtIotReport);
	   xmppClient->removeStanzaExtension(ExtIotMessage);
	   xmppClient->removeIqHandler(this, ExtIotMessage);
	   xmppClient->removeStanzaExtension(ExtIotUpdate);
	   xmppClient->removeIqHandler(this, ExtIotUpdate);
	   xmppClient->removeSubscriptionHandler(this);
	   xmppClient->removeMessageHandler(this);
	   xmppClient->removeIqHandler(this,ExtPing);
	   delete(xmppClient);
	}
	
	PlaybackCmd_clean(); //jyc20170405 resume
	Playback_DeleteAll(); //jyc20170405 resume 

	if(ipcamClient){ //jyc20170330 resume
		delete(ipcamClient);
	}

	if(deviceClient){
		delete(deviceClient);
	}

	if( m_cfg ) delete(m_cfg);
    if( m_event ) delete(m_event);
	if( timer ) delete(timer);

	if( m_DataStoreMgr )
	{
		delete m_DataStoreMgr;
		m_DataStoreMgr = NULL;
	}
	//CoUninitialize();  //jyc20170302 comport init for windows
}

//* jyc20170223 notice
void GSIOTClient::OnTimeOverForCmdRecv(const defLinkID LinkID, 
                                       const IOTDeviceType DevType, 
                                       const uint32_t DevID, 
                                       const uint32_t addr )
{
	if( IOT_DEVICE_Unknown==DevType || 0==DevID )
	{
		return;
	}

	switch( DevType )
	{
	case IOT_DEVICE_RS485:
		{
			std::list<GSIOTDevice*>::const_iterator it = IotDeviceList.begin();
			std::list<GSIOTDevice*>::const_iterator itEnd = IotDeviceList.end();
			for( ; it!=itEnd && addr; ++it )
			{
				GSIOTDevice *pCurDev = (*it);
				if( pCurDev->getType() != DevType )
					continue;

				if( pCurDev->getId() != DevID )
					continue;

				if( pCurDev->GetLinkID() != LinkID )
					return;

				RS485DevControl *pCurCtl = (RS485DevControl*)pCurDev->getControl();
				if( !pCurCtl )
					return;

				DeviceAddress *pCurAddr = pCurCtl->GetAddress( addr );
				if( pCurAddr )  //jyc20170303 notice 
				{
					bool isChanged = false;
					pCurCtl->check_NetUseable_RecvFailed( &isChanged );

					if( isChanged && m_handler )
					{
						
					}
				}

				return;
			}
		}
		break;
	}
}

defUseable GSIOTClient::get_all_useable_state_ForLinkID( defLinkID LinkID )
{
	if( defLinkID_Local == LinkID )
	{
		return defUseable_OK;
	}

	CCommLinkAuto_Run_Info_Get AutoCommLink( deviceClient->m_CommLinkMgr, LinkID );
	CCommLinkRun *pCommLink = AutoCommLink.p();
	if( pCommLink )
	{
		return pCommLink->get_all_useable_state_ForDevice();
	}

	return defUseable_Err;
}

void GSIOTClient::OnDeviceDisconnect(GSIOTDevice *iotdevice)
{
	//std::list<GSIOTDevice *>::const_iterator it = IotDeviceList.begin(); //jyc20160922 modify
	std::list<GSIOTDevice *>::iterator it = IotDeviceList.begin();
	for(;it!=IotDeviceList.end();it++){
		if((*it)->getId() == iotdevice->getId() && (*it)->getType() == iotdevice->getType()){
			IotDeviceList.erase(it); 
			break;
		}
	}
	//OnDeviceStatusChange(); //jyc20160919 no really code
	if(m_handler){
		m_handler->OnDeviceDisconnect(iotdevice);
	}
}

void GSIOTClient::OnDeviceConnect(GSIOTDevice *iotdevice)
{
	std::list<GSIOTDevice *>::const_iterator it = IotDeviceList.begin();
	for(;it!=IotDeviceList.end();it++){
		if((*it)->getId() == iotdevice->getId() && (*it)->getType() == iotdevice->getType()){
		    return;
		}
	}
	IotDeviceList.push_back(iotdevice);
	//OnDeviceStatusChange(); //jyc20160919 no code
	if(m_handler){
		m_handler->OnDeviceConnect(iotdevice);
	}
}

void GSIOTClient::OnDeviceNotify( defDeviceNotify_ notify, GSIOTDevice *iotdevice, DeviceAddress *addr )
{
	if( defDeviceNotify_Modify == notify )
	{
		std::list<GSIOTDevice*>::const_iterator it = IotDeviceList.begin();
		for(;it!=IotDeviceList.end();it++)
		{
			GSIOTDevice *pDev = (*it);
			if( pDev->getId() == iotdevice->getId()
				&& pDev->getType() == iotdevice->getType() )
			{
				if( addr )
				{
					// only refresh address
					ControlBase *pCCtl = pDev->getControl();
					switch(pCCtl->GetType())
					{
					case IOT_DEVICE_RS485:
						{
							RS485DevControl *ctl = (RS485DevControl*)pCCtl;
							ctl->UpdateAddress( addr );
						}
						break;
					}
				}
				else
				{
					// only refresh device
					pDev->setName( iotdevice->getName() );
				}

				break;
			}
		}
	}

	if( m_handler )
	{
		m_handler->OnDeviceNotify( notify, iotdevice, addr );
	}
}

void GSIOTClient::OnDeviceData( defLinkID LinkID, GSIOTDevice *iotdevice, ControlBase *ctl, GSIOTObjBase *addr )
{
	//const int thisThreadId = ::GetCurrentThreadId();
	const int thisThreadId = ::pthread_self();
	LOGMSG( "OnDeviceData Link%d, ctl(%d,%d)-ThId%d\n", LinkID, ctl->GetType(), iotdevice?iotdevice->getId():0, thisThreadId );	
	switch(ctl->GetType())
	{
	case IOT_DEVICE_RS485:
		{
			const bool hasaddr = ( addr && ((DeviceAddress*)addr)->GetAddress()>0 );

			if( !hasaddr )
			{
				RS485DevControl *rsctl = (RS485DevControl*)ctl;

				const defAddressQueue &AddrQue = rsctl->GetAddressList();
				if( !AddrQue.empty() )
				{
					defAddressQueue::const_iterator itAddrQue = AddrQue.begin();
					for( ; itAddrQue!=AddrQue.end(); ++itAddrQue )
					{
						DeviceAddress *pOneAddr = *itAddrQue;

						OnDeviceData_ProcOne( LinkID, iotdevice, ctl, pOneAddr );
					}

					return;
				}
			}
		}
	}

	OnDeviceData_ProcOne( LinkID, iotdevice, ctl, (DeviceAddress*)addr );
}

//jyc20170330 notice recive msg here
void GSIOTClient::OnDeviceData_ProcOne( defLinkID LinkID, GSIOTDevice *iotdevice, ControlBase *ctl, DeviceAddress *addr )
{
	//const int thisThreadId = ::GetCurrentThreadId(); //jyc20160919
	const int thisThreadId = ::pthread_self();
	const time_t curUTCTime = g_GetUTCTime();
	

	switch(ctl->GetType())
	{
		case IOT_DEVICE_Trigger:
		{
			TriggerControl *tctl = (TriggerControl *)ctl;

			struGSTime curdt;
			g_struGSTime_GetCurTime( curdt );
			if( m_EnableTriggerDebug && m_ITriggerDebugHandler )
			{
				m_ITriggerDebugHandler->OnTriggerDebug( LinkID, iotdevice?iotdevice->getType():IOT_DEVICE_Unknown, iotdevice?iotdevice->getName():"", tctl->GetAGRunState(), this->GetAlarmGuardGlobalFlag(), g_IsValidCurTimeInAlarmGuardState(), curdt, iotdevice->GetStrAlmBody( true, curdt ), iotdevice->GetStrAlmSubject( true ) );
			}

			tctl->CompareTick();		
			if(tctl->IsTrigger(true)){
				LOGMSG( "TriggerControl(id=%d,name=%s) isTrigger true, CurTriggerCount=%d -ThId%d\r\n",
					iotdevice?iotdevice->getId():0, iotdevice?iotdevice->getName().c_str():"", tctl->GetCurTriggerCount(), thisThreadId );

				const int AlarmGuardGlobalFlag = this->GetAlarmGuardGlobalFlag();
				const bool IsValidCurTime = g_IsValidCurTimeInAlarmGuardState();

				this->DoAlarmDevice( iotdevice, tctl->GetAGRunState(), AlarmGuardGlobalFlag, IsValidCurTime, iotdevice->GetStrAlmBody( true, curdt ), iotdevice->GetStrAlmSubject( true ) );

				tctl->SetTriggerDo();
			}
		    break;
		}
	case IOT_DEVICE_Remote:
		{
			break;
		}
	case IOT_DEVICE_RFDevice:
		{
			break;
		}
	case IOT_DEVICE_CANDevice:
		{
			break;
		}
	case IOT_DEVICE_RS485:
		{
#if 1
			// update device cur value
			std::list<GSIOTDevice*>::const_iterator it = IotDeviceList.begin();
			std::list<GSIOTDevice*>::const_iterator itEnd = IotDeviceList.end();
			for( ; it!=itEnd && addr; ++it )
			{
				GSIOTDevice *pCurDev = NULL;
				RS485DevControl *pCurCtl = NULL;
				DeviceAddress *pCurAddr = NULL;

				pCurDev = (*it);

				if( !pCurDev->GetEnable() )
					continue;

				pCurCtl = (RS485DevControl*)pCurDev->getControl();

				if( !pCurCtl )
					continue;

				if( pCurDev->GetLinkID() != LinkID )
					continue;
		
				if( iotdevice )
				{
					if( !GSIOTClient::Compare_Device( iotdevice, pCurDev ) )
						continue;
				}

				if( !GSIOTClient::Compare_Control( pCurCtl, ctl ) )
					continue;

				pCurAddr = pCurCtl->GetAddress( addr->GetAddress() );

				if( !pCurAddr )
					continue;

				if( !pCurAddr->GetEnable() )
					continue;

				if( pCurAddr )
				{
					std::string strlog;
					if( !pCurAddr->SetCurValue( addr->GetCurValue(), curUTCTime, true, &strlog ) )
					{
						if( !strlog.empty() )
						{
							LOGMSG( "%s dev(%d,%d)-ThId%d", strlog.c_str(), pCurDev->getType(), pCurDev->getId(), thisThreadId );
						}

						continue;
					}

					bool isChanged = false;
					pCurCtl->set_NetUseable( defUseable_OK, &isChanged );

					if( m_DataStoreMgr && g_isNeedSaveType(pCurAddr->GetType()) )
					{
						bool doSave = false;
						time_t SaveTime = 0;
						std::string SaveValue = "0";
						defDataFlag_ dataflag = defDataFlag_Norm;
						strlog = "";

						pCurAddr->DataAnalyse( addr->GetCurValue(), curUTCTime, &doSave, &SaveTime, &SaveValue, &dataflag, &strlog );

#if defined(defForceDataSave)
						if( IsRUNCODEEnable(defCodeIndex_TEST_ForceDataSave) )
						{
							if( !doSave )
							{
								doSave = true;
								SaveTime = curUTCTime;
								SaveValue = addr->GetCurValue();
								strlog = "force save";
							}
						}
#endif

						if( !strlog.empty() )
						{
							LOGMSG( "%s dev(%d,%d)-ThId%d", strlog.c_str(), pCurDev->getType(), pCurDev->getId(), thisThreadId );
						}

						if( doSave )
						{
							gloox::util::MutexGuard mutexguard( m_mutex_DataStore );

							const size_t DataSaveBufSize = m_lstDataSaveBuf.size();
							if( DataSaveBufSize<10000 )
							{
								m_lstDataSaveBuf.push_back( new struDataSave( SaveTime, pCurDev->getType(),
								          pCurDev->getId(), pCurAddr->GetType(), pCurAddr->GetAddress(),
								          dataflag, SaveValue, pCurDev->getName()+"-"+pCurAddr->GetName()));
							}
							else if( DataSaveBufSize > 100 )
							{
								LOGMSG( "lstDataSaveBuf max, size=%d -ThId%d", m_lstDataSaveBuf.size(), thisThreadId );
							}
						}
					}

					if( isChanged && m_handler )
					{
						
					}
				}
			}
#endif
		}
		break;
	}
	//send to UI     
	if(m_handler){
	    m_handler->OnDeviceData(LinkID, iotdevice, ctl, addr);
	}

	while(1)   //send to network UI  JYC20170220 TRANS
	{
		ControlMessage *pCtlMsg = PopControlMesssageQueue( iotdevice, ctl, addr );  //send is here
		if( pCtlMsg && addr )
		{
			DeviceAddress *reAddr = (DeviceAddress*)pCtlMsg->GetObj();
			reAddr->SetCurValue(addr->GetCurValue());

			if( pCtlMsg->GetDevice() )
			{
				pCtlMsg->GetDevice()->SetCurValue( addr );
			}

			if( pCtlMsg->GetJid() )
			{
				IQ re( IQ::Result, pCtlMsg->GetJid(), pCtlMsg->GetId());
				re.addExtension(new GSIOTControl(pCtlMsg->GetDevice()->clone(), defUserAuth_RW, false));
				XmppClientSend(re,"OnDeviceData Send");
			}

			delete pCtlMsg;
		}
		else
		{
			break;
		}
	}
}

void GSIOTClient::DoAlarmDevice( const GSIOTDevice *iotdevice, const bool AGRunState, const int AlarmGuardGlobalFlag, const bool IsValidCurTime, const std::string &strAlmBody, const std::string &strAlmSubject )
{
	if( !iotdevice )
		return ;

	printf( "DoAlarmDevice Begin(id=%d,name=%s) AGRunState=%d", 
	       iotdevice?iotdevice->getId():0, iotdevice?iotdevice->getName().c_str():"", (int)AGRunState );

	if( !AlarmGuardGlobalFlag )
	{
		LOGMSG( "DoAlarmDevice(id=%d,name=%s) AlarmGuardGlobalFlag=%d is stop\r\n",
			iotdevice?iotdevice->getId():0, iotdevice?iotdevice->getName().c_str():"", AlarmGuardGlobalFlag );
		//break;
	}
	else if( !AGRunState )
	{
		LOGMSG( "DoAlarmDevice(id=%d,name=%s) AGRunState=%d is stop run\r\n",
			iotdevice?iotdevice->getId():0, iotdevice?iotdevice->getName().c_str():"", (int)AGRunState );
		//break;
	}
	else if( !IsValidCurTime )
	{
		LOGMSG( "DoAlarmDevice(id=%d,name=%s) AGRunState=%d is running, but AlarmGuard invalid time\r\n",
			iotdevice?iotdevice->getId():0, iotdevice?iotdevice->getName().c_str():"", (int)AGRunState );
		//break;
	}

	std::list<ControlEvent *> evtList = m_event->GetEvents();
	std::list<ControlEvent *>::const_iterator it = evtList.begin();
	for(;it!=evtList.end();it++)
	{
		if((*it)->GetEnable() 
			&& (*it)->GetDeviceType() == iotdevice->getType()
			&& (*it)->GetDeviceID() == iotdevice->getId()
			)
		{
			if( (*it)->isForce()
				|| ( AlarmGuardGlobalFlag && AGRunState && IsValidCurTime ) // 
				)	
			{
				// interval action
				bool needDoInterval = DoControlEvent( iotdevice->getType(), iotdevice->getId(), (*it), true, strAlmBody, strAlmSubject, (*it)->isForce()?"Force Trigger doevent":"Trigger doevent" );

				if( needDoInterval )
				{
					uint32_t DoInterval = (*it)->GetDoInterval();
					DoInterval = DoInterval>3000 ? 3000:DoInterval;

					if( DoInterval > 0 )
					{
						//Sleep( DoInterval );
						usleep( DoInterval*1000 ); //jyc20170302 notice 1000 ->100 ??
					}
				}
			}
		}
	}

	LOGMSG( "DoAlarmDevice End(id=%d,name=%s)", iotdevice?iotdevice->getId():0, iotdevice?iotdevice->getName().c_str():"" );
}

bool GSIOTClient::DoControlEvent( const IOTDeviceType DevType, const uint32_t DevID, ControlEvent *ctlevt, const bool isAlarm, const std::string &strAlmBody, const std::string &strAlmSubject, const char *callinfo, const bool isTest, const char *teststr )
{
	bool needDoInterval = false;

	if( !isTest )
	{
		uint32_t outDoInterval = 0;
		if( !ctlevt->IsCanDo(m_cfg, outDoInterval) )
		{
			LOGMSG( "DoControlEvent IsCanDo=false, evttype=%d, evtid=%d, DoInterval=%d", ctlevt->GetType(), ctlevt->GetID(), outDoInterval );

			return needDoInterval;
		}

		ctlevt->SetDo();
	}

	switch(ctlevt->GetType()){
	case SMS_Event:
		{
			AutoSendSMSEvent *pnewEvt = (AutoSendSMSEvent*)((AutoSendSMSEvent*)ctlevt)->clone();
			pnewEvt->SetTest( isTest );

			if( pnewEvt->GetSMS().empty() )
			{
				pnewEvt->SetSMS( strAlmBody );
			}

			//deviceClient->GetGSM().AddSMS( pnewEvt ); //jyc20160919 remove
		}
		break;

	case EMAIL_Event:
		//un realize
		break;

	case NOTICE_Event:
		{
			AutoNoticeEvent *aEvt = (AutoNoticeEvent *)ctlevt;

			std::set<std::string> jidlist;
			if( aEvt->GetToJid().empty() )
			{
				const defmapGSIOTUser &mapUser = m_cfg->m_UserMgr.GetList_User();
				for( defmapGSIOTUser::const_iterator it=mapUser.begin(); it!=mapUser.end(); ++it )
				{
					const GSIOTUser *pUser = it->second;

					if( !pUser->GetEnable() )
						continue;

					if( !pUser->get_UserFlag( defUserFlag_NoticeGroup ) )
						continue;

					const defUserAuth curAuth = m_cfg->m_UserMgr.check_Auth( pUser, ctlevt->GetDeviceType(), ctlevt->GetDeviceID() );
					if( !GSIOTUser::JudgeAuth( curAuth, defUserAuth_RO ) )
						continue;

					jidlist.insert( pUser->GetJid() );
				}
			}
			else
			{
				jidlist.insert( aEvt->GetToJid() );
			}

			//if( m_NoticeJid || to_jid )
			if( !jidlist.empty() )
			{
				std::string strBody = aEvt->GetBody();
				if( strBody.empty() )
				{
					strBody = strAlmBody;
				}

				std::string strSubject = aEvt->GetSubject();
				if( strSubject.empty() )
				{
					strSubject = strAlmSubject;
				}

				if( isTest && teststr )
				{
					strBody = std::string(teststr) + strBody;
					//strBody += " (system test)";
				}

				XmppClientSend_jidlist( jidlist, strBody, strSubject, callinfo );
			}
			else
			{
				printf( "NoticeJid is invalid! no noticejid\r\n" ); //jyc20160919 notice
			}
		}
		break;

	case CONTROL_Event:
		{
			AutoControlEvent *aEvt = (AutoControlEvent *)ctlevt;
			GSIOTDevice* ctldev = this->GetIOTDevice( aEvt->GetControlDeviceType(), aEvt->GetControlDeviceId() );
			ControlBase *cctl = ctldev ? ctldev->getControl() : NULL;
			if(cctl){
				switch(cctl->GetType()){
				case IOT_DEVICE_Remote:
					{
						GSIOTDevice *sendDev = ctldev->clone( false );
						RFRemoteControl *ctl = (RFRemoteControl*)sendDev->getControl();
						RemoteButton *pButton = ctl->GetButton(aEvt->GetAddress());
						if(pButton)
						{
							ctl->ButtonQueueChangeToOne( pButton->GetId() );
							ctl->Print( callinfo, true, pButton );
							this->SendControl(DevType, sendDev, NULL, defNormSendCtlOvertime, defNormMsgOvertime, aEvt->GetDoInterval()>0 ? aEvt->GetDoInterval():1 );
						}
						macCheckAndDel_Obj(ctl);
						break;
					}
					break;
				case IOT_DEVICE_RFDevice: //jyc 20160919 for cc1101
					{
						break;
					}
				case IOT_DEVICE_CANDevice:
					{
						break;
					}
				case IOT_DEVICE_RS485:
					{
						GSIOTDevice *sendDev = ctldev->clone( false );
						RS485DevControl *rsCtl = (RS485DevControl*)sendDev->getControl();
						DeviceAddress *addr = rsCtl->GetAddress(aEvt->GetAddress());
						if(addr)
						{
							rsCtl->SetCommand( defModbusCmd_Write );
							addr->SetCurValue( aEvt->GetValue() );

							if(addr)
							{
								rsCtl->Print( callinfo, true, addr );
								this->SendControl(DevType, sendDev, addr, defNormSendCtlOvertime, defNormMsgOvertime, aEvt->GetDoInterval()>0 ? aEvt->GetDoInterval():1 );
							}
						}
						macCheckAndDel_Obj(rsCtl);
						break;
					}

				case IOT_DEVICE_Camera: //jyc20170330 resume
					IPCameraBase *ctl = (IPCameraBase*)cctl;
					if( ctl->GetAdvAttr().get_AdvAttr( defCamAdvAttr_PTZ_Preset ) )
					{
						const CPresetObj *pPresetLocal = ctl->GetPreset(aEvt->GetAddress());
						if( pPresetLocal )
						{
							ctl->SendPTZ( GSPTZ_Goto_Preset, pPresetLocal->GetIndex(), 0, 0, callinfo );
							needDoInterval = true;
						}
					}
					break;
				}
			}
			break;
		}

	case Eventthing_Event:
		{
			AutoEventthing *aEvt = (AutoEventthing *)ctlevt;
			DoControlEvent_Eventthing( aEvt, ctlevt, callinfo, isTest );
		}
		break;
	}

	return needDoInterval;
}

void GSIOTClient::DoControlEvent_Eventthing( const AutoEventthing *aEvt, const ControlEvent *ctlevt, const char *callinfo, const bool isTest )
{
	if( aEvt->IsAllDevice() )
	{
		LOGMSG( "Do Eventthing_Event all dev set runstate=%d", aEvt->GetRunState() );
		SetAlarmGuardGlobalFlag( aEvt->GetRunState() ); // this->SetAllEventsState( aEvt->GetRunState(), callinfo, false );

	}
	else
	{
		LOGMSG( "Do Eventthing_Event devtype=%d, devid=%d, set runstate=%d", aEvt->GetControlDeviceType(), aEvt->GetControlDeviceId(), aEvt->GetRunState() );

		GSIOTDevice *dev = deviceClient->GetIOTDevice( aEvt->GetControlDeviceType(), aEvt->GetControlDeviceId() );
		DoControlEvent_Eventthing_Event( dev, aEvt, ctlevt, callinfo, isTest );
	}
}

void GSIOTClient::DoControlEvent_Eventthing_Event( GSIOTDevice *dev, const AutoEventthing *aEvt, const ControlEvent *ctlevt, const char *callinfo, const bool isTest )
{
	if( dev )
	{
		if( dev->getControl() )
		{
			switch( dev->getType() )
			{
			case IOT_DEVICE_Trigger:
				{
					TriggerControl *ctl = (TriggerControl *)dev->getControl();
					ctl->SetAGRunState( aEvt->GetRunState() );
					deviceClient->ModifyDevice( dev, 0 );
				}
				break;

			case IOT_DEVICE_Camera: //jyc20170330 resume
				{
					IPCameraBase *ctl = (IPCameraBase*)dev->getControl();
					ctl->SetAGRunState( aEvt->GetRunState() );
					deviceClient->ModifyDevice( dev, 0 );
				}
				break;

			default:
				LOGMSG( "Do Eventthing_Event devtype=%d, devid=%d, not support", aEvt->GetControlDeviceType(), aEvt->GetControlDeviceId() );
				break;
			}
		}
		else
		{
			LOGMSG( "Do Eventthing_Event found dev and ctl err, devtype=%d, devid=%d", aEvt->GetControlDeviceType(), aEvt->GetControlDeviceId() );
		}
	}
	else
	{
		LOGMSG( "Do Eventthing_Event not found dev, devtype=%d, devid=%d", aEvt->GetControlDeviceType(), aEvt->GetControlDeviceId() );
	}
}

GSAGCurState_ GSIOTClient::GetAllEventsState() const
{
	bool isAllTrue = true;
	bool isAllFalse = true;
	
	std::list<GSIOTDevice*> devices = deviceClient->GetDeviceManager()->GetDeviceList();
	for( std::list<GSIOTDevice*>::const_iterator it=devices.begin(); it!=devices.end(); ++it )
	{
		const GSIOTDevice* dev = (*it);

		if( !dev->getControl() )
			continue;

		if( !dev->GetEnable()
			|| !GSIOTDevice::IsSupportAlarm(dev)
			)
		{
			continue;
		}

		bool AGRunState = false;
		switch( dev->getType() )
		{
		case IOT_DEVICE_Trigger:
			{
				const TriggerControl *ctl = (TriggerControl*)dev->getControl();
				AGRunState = ctl->GetAGRunState();
			}
			break;

		case IOT_DEVICE_Camera: //jyc20170330 resume
			{
				const IPCameraBase *ctl = (IPCameraBase*)dev->getControl();
				AGRunState = ctl->GetAGRunState();
			}
			break;

		default:
			continue;
		}

		if( AGRunState )
		{
			isAllFalse = false;
		}
		else
		{
			isAllTrue = false;
		}
	}
	//jyc20170330 resume but have trouble in run 20170602 resume
//	devices = ipcamClient->GetCameraManager()->GetCameraList();
//	for( std::list<GSIOTDevice*>::const_iterator it=devices.begin(); it!=devices.end(); ++it )
//	{
//		const GSIOTDevice* dev = (*it);
//
//		if( !dev->getControl() )
//			continue;
//
//		if( !dev->GetEnable()
//			|| !GSIOTDevice::IsSupportAlarm(dev)
//			)
//		{
//			continue;
//		}
//
//		bool AGRunState = false;
//		switch( dev->getType() )
//		{
//		case IOT_DEVICE_Trigger:
//			{
//				const TriggerControl *ctl = (TriggerControl*)dev->getControl();
//				AGRunState = ctl->GetAGRunState();
//			}
//			break;
//
//		case IOT_DEVICE_Camera:
//			{
//				const IPCameraBase *ctl = (IPCameraBase*)dev->getControl();
//				AGRunState = ctl->GetAGRunState();
//			}
//			break;
//
//		default:
//			continue;
//		}
//
//		if( AGRunState )
//		{
//			isAllFalse = false;
//		}
//		else
//		{
//			isAllTrue = false;
//		}
//	}

	if( isAllTrue )
		return GSAGCurState_AllArmed;	// all work

	if( isAllFalse )
		return GSAGCurState_UnArmed;	// no work

	return GSAGCurState_PartOfArmed;	// part work
}

void GSIOTClient::SetAllEventsState( const bool AGRunState, const char *callinfo, const bool forcesave )
{
	LOGMSG( "SetAllEventsState=%d, forcesave=%d, info=%s\r\n", (int)AGRunState, (int)forcesave, callinfo?callinfo:"" );

	SetAllEventsState_do( AGRunState, forcesave, true );
	SetAllEventsState_do( AGRunState, forcesave, false );
}

void GSIOTClient::SetAllEventsState_do( const bool AGRunState, const bool forcesave, bool isEditCam )
{
	//jyc20170330 resume but have trouble
	SQLite::Database *db = isEditCam ? ipcamClient->GetCameraManager()->get_db() : deviceClient->GetDeviceManager()->get_db();
	//SQLite::Database *db = deviceClient->GetDeviceManager()->get_db();
	UseDbTransAction dbta(db);
	//jyc20170330 resume but have trouble
	const std::list<GSIOTDevice*> devices = isEditCam ? ipcamClient->GetCameraManager()->GetCameraList() : deviceClient->GetDeviceManager()->GetDeviceList();
	//const std::list<GSIOTDevice*> devices = deviceClient->GetDeviceManager()->GetDeviceList();

	
	for( std::list<GSIOTDevice*>::const_iterator it=devices.begin(); it!=devices.end(); ++it )
	{
		GSIOTDevice* dev = (*it);

		if( isEditCam )
		{
			if( IOT_DEVICE_Camera != dev->getType() )
				continue;
		}
		else
		{
			if( IOT_DEVICE_Camera == dev->getType() )
				continue;
		}

		if( !dev->getControl() )
			continue;

		if( !dev->GetEnable()
			|| !GSIOTDevice::IsSupportAlarm(dev)
			)
		{
			continue;
		}

		switch( dev->getType() )
		{
		case IOT_DEVICE_Trigger:
			{
				TriggerControl *ctl = (TriggerControl*)dev->getControl();
				if( forcesave || ctl->GetAGRunState() != AGRunState )
				{
					ctl->SetAGRunState( AGRunState );
				}
			}
			break;

		case IOT_DEVICE_Camera: //jyc20170330 resume
			{
				IPCameraBase *ctl = (IPCameraBase*)dev->getControl();
				if( forcesave || ctl->GetAGRunState() != AGRunState )
				{
					ctl->SetAGRunState( AGRunState );
				}
			}
			break;

		default:
			continue;
		}

		if( isEditCam )
		{
			ipcamClient->ModifyDevice( dev ); //jyc20170330 resume
		}
		else
		{
			deviceClient->ModifyDevice( dev );
		}
	}
}

void GSIOTClient::SetITriggerDebugHandler( ITriggerDebugHandler *handler )
{
	this->m_ITriggerDebugHandler = handler; 
}

void GSIOTClient::AddGSMessage( GSMessage *pMsg )
{
	if( !pMsg )
		return;

	defGSMsgType_ MsgType = pMsg->getMsgType();

	m_mutex_lstGSMessage.lock();

	// 
	if( m_lstGSMessage.size()>10000 )
	{
		printf( "AddGSMessage full, do release , beforeCount=%u\r\n", m_lstGSMessage.size() );

		while( m_lstGSMessage.size()>500 )
		{
			GSMessage *p =  m_lstGSMessage.front();
			m_lstGSMessage.pop_front();
			delete p;
		}

		printf( "AddGSMessage full, do release , AfterCount=%u\r\n", m_lstGSMessage.size() );
	}

	m_lstGSMessage.push_back( pMsg );

	m_mutex_lstGSMessage.unlock();

	if( m_IGSMessageHandler ) //qjyc20160923 trouble no set 
		m_IGSMessageHandler->OnGSMessage( MsgType, 0 ); //jyc20160923 no ongsmessage
}

GSMessage* GSIOTClient::PopGSMessage()
{
	gloox::util::MutexGuard mutexguard( m_mutex_lstGSMessage );

	if( m_lstGSMessage.size()>0 )
	{
		GSMessage *p = m_lstGSMessage.front();
		m_lstGSMessage.pop_front();
		return p;
	}

	return NULL;
}

void GSIOTClient::OnGSMessageProcess()  //jyc20170227 notice local ui program ,sometime must use;
{
	DWORD dwStart = ::timeGetTime();
	while( ::timeGetTime()-dwStart < 700 )
	{
		GSMessage *pMsg = PopGSMessage();
		if( !pMsg )
			return ;

		if( pMsg->isOverTime() )
		{
			LOGMSG( "OnGSMessageProcess overtime!!! MsgType=%d,", pMsg->getMsgType() );
			delete pMsg;
			return;
		}

		LOGMSG( "OnGSMessageProcess MsgType=%d,", pMsg->getMsgType() );

		if( pMsg->getpEx() )
		{
			switch(pMsg->getpEx()->extensionType())
			{
			case ExtIotAuthority:
				{
					handleIq_Set_XmppGSAuth( pMsg );
				}
				break;

			case ExtIotManager:
				{
					handleIq_Set_XmppGSManager( pMsg );
				}
				break;

			case ExtIotEvent:
				{
					handleIq_Set_XmppGSEvent( pMsg );
				}
				break;

			case ExtIotRelation:
				{
					handleIq_Set_XmppGSRelation( pMsg );
				}
				break;

			case ExtIotPreset:
				{
					handleIq_Set_XmppGSPreset( pMsg ); //jyc20170405 resume
				}
				break;

			case ExtIotVObj:
				{
					handleIq_Set_XmppGSVObj( pMsg );
				}
				break;

			default:
				{
					printf( "OnGSMessageProcess MsgType=%d, exType=%d err", pMsg->getMsgType(), pMsg->getpEx()->extensionType() );
				}
				break;
			}
		}

		delete pMsg;
	}
}

void GSIOTClient::PushControlMesssageQueue( ControlMessage *pCtlMsg )
{
	
	gloox::util::MutexGuard mutexguard( m_mutex_ctlMessageList );

	ctlMessageList.push_back( pCtlMsg );

	if( ctlMessageList.size()>10000  )
	{
		printf( "PushControlMesssageQueue full, do release , beforeCount=%u\r\n", ctlMessageList.size() );

		while( ctlMessageList.size()>500 )
		{
			ControlMessage *p =  ctlMessageList.front();
			ctlMessageList.pop_front();
			delete p;
		}

		printf( "PushControlMesssageQueue full, do release , AfterCount=%u\r\n", ctlMessageList.size() );
	}
}

bool GSIOTClient::CheckControlMesssageQueue( GSIOTDevice *device, DeviceAddress *addr, JID jid, std::string id )
{
	bool blCheck = false;
	
	gloox::util::MutexGuard mutexguard( m_mutex_ctlMessageList );

	if( ctlMessageList.empty() )
	{
		return false;
	}

	std::list<ControlMessage *>::iterator it = ctlMessageList.begin();
	for(;it!=ctlMessageList.end();it++)
	{
		GSIOTDevice *dev = (*it)->GetDevice();
		if( Compare_Device( dev, device ) 
			&& Compare_GSIOTObjBase( (*it)->GetObj(), addr )
			&& jid == (*it)->GetJid()
			&& id == (*it)->GetId() )
		{
			(*it)->SetNowTime();
			blCheck = true;
			break;
		}
	}

	return blCheck;
}

ControlMessage* GSIOTClient::PopControlMesssageQueue( GSIOTDevice *device, ControlBase *ctl, DeviceAddress *addr, IOTDeviceType specType, IOTDeviceType specExType )
{
	if( !addr && IOT_DEVICE_Unknown==specType && IOT_DEVICE_Unknown==specExType )
	{
		return NULL;
	}

	ControlMessage *pCtlMsg = NULL;

	gloox::util::MutexGuard mutexguard( m_mutex_ctlMessageList );

	if( ctlMessageList.empty() )
	{
		return NULL;
	}

	std::list<ControlMessage *>::iterator it = ctlMessageList.begin();
	for(;it!=ctlMessageList.end();it++)
	{
		GSIOTDevice *dev = (*it)->GetDevice();
		
		// get extype
		if( !device && !ctl && !addr )
		{
			if( dev->getType()==specType && dev->getExType()==specExType )
			{
				pCtlMsg = (*it);
				ctlMessageList.erase( it );
				break;
			}

			continue;
		}

		if( device )
		{
			if( !Compare_Device( dev, device ) )
			{
				continue;
			}
		}
		else if( ctl )
		{
			if( !Compare_Control( dev->getControl(), ctl ) )
			{
				continue;
			}
		}
		else
		{
			continue;
		}

		if( !Compare_GSIOTObjBase( (*it)->GetObj(), addr ) )
		{
			continue;
		}

		pCtlMsg = (*it);
		ctlMessageList.erase(it);
		break;
	}

	return pCtlMsg;
}

void GSIOTClient::CheckOverTimeControlMesssageQueue()
{
	gloox::util::MutexGuard mutexguard( m_mutex_ctlMessageList );

	CheckOverTimeControlMesssageQueue_nolock();
}

void GSIOTClient::CheckOverTimeControlMesssageQueue_nolock()
{
	ControlMessage *pCtlMsg = NULL;

	if( ctlMessageList.empty() )
	{
		return ;
	}

	std::list<ControlMessage *>::iterator it = ctlMessageList.begin();
	while( it!=ctlMessageList.end() )
	{
		pCtlMsg = (*it);

		if( pCtlMsg->IsOverTime() )
		{
			pCtlMsg->Print( "ctlMessage:: overtime" );

			delete pCtlMsg;
			ctlMessageList.erase(it);
			it = ctlMessageList.begin();
			continue;
		}

		++it;
	}
}

void GSIOTClient::onConnect()
{
	printf( "GSIOTClient::onConnect\r\n" );
}

void GSIOTClient::onDisconnect( ConnectionError e )
{
	printf( "GSIOTClient::onDisconnect(err=%d)\r\n", e );

}

bool GSIOTClient::onTLSConnect( const CertInfo& info )
{
	printf( "GSIOTClient::onTLSConnect\r\n" );

	return true;
}

void GSIOTClient::handleMessage( const Message& msg, MessageSession* session)
{
	std::string subject = msg.subject();
	std::string body = msg.body();
	
	if(body == "help"){
	    
	}
}

void GSIOTClient::handleIqID( const IQ& iq, int context )
{
}

bool GSIOTClient::handleIq( const IQ& iq )
{
	if( iq.from() == this->xmppClient->jid() )
	{
#ifdef _DEBUG
		printf( "handleIq iq.from() == this->jid()!!!" );
#endif
		return true;
	}

//	XmppPrint( iq, "test recv.......\n" );  //jyc20170227 debug recv message

	switch( iq.subtype() ){
        	case IQ::Get:
			{
				// heartbeat always passed with server --jyc note
				const StanzaExtension *Ping= iq.findExtension(ExtPing);
				if(Ping){
					XmppPrint( iq, "handleIq recv" );

					if(iq.from().full() == XMPP_SERVER_DOMAIN){
						serverPingCount++;
					}
				    return true;
				}

				/*author control*/
				GSIOTUser *pUser = m_cfg->m_UserMgr.check_GetUser( iq.from().bare() );
				this->m_cfg->FixOwnerAuth(pUser);

				XmppGSAuth_User *pExXmppGSAuth_User = (XmppGSAuth_User*)iq.findExtension(ExtIotAuthority_User);
				if( pExXmppGSAuth_User )
				{
					handleIq_Get_XmppGSAuth_User( pExXmppGSAuth_User, iq, pUser );
					return true;
				}
#if defined(defTest_defCfgOprt_GetSelf)
				XmppGSAuth *pExXmppGSAuth_Test = (XmppGSAuth*)iq.findExtension(ExtIotAuthority);
				if( pExXmppGSAuth_Test )
				{
					handleIq_Get_XmppGSAuth( pExXmppGSAuth_Test, iq, pUser );
					return true;
				}
#endif

				defGSReturn ret = m_cfg->m_UserMgr.check_User(pUser);
				if( macGSFailed(ret) )
				{
					printf( "(%s)IQ::Get: Not found userinfo. no auth.", iq.from().bare().c_str() );
					IQ re( IQ::Result, iq.from(), iq.id());
					re.addExtension( new XmppGSResult( "all", defGSReturn_NoAuth ) );
					XmppClientSend(re,"handleIq Send(all Get ACK)");
					return true;
				}
					
				

				XmppGSAuth *pExXmppGSAuth = (XmppGSAuth*)iq.findExtension(ExtIotAuthority);
				if( pExXmppGSAuth )
				{
					handleIq_Get_XmppGSAuth( pExXmppGSAuth, iq, pUser );
					return true;
				}

				
				XmppGSState *pExXmppGSState = (XmppGSState*)iq.findExtension(ExtIotState);
				if( pExXmppGSState )
				{
					defUserAuth curAuth = m_cfg->m_UserMgr.check_Auth( pUser, IOT_Module_system, defAuth_ModuleDefaultID );
					if( !GSIOTUser::JudgeAuth( curAuth, defUserAuth_RO ) )
					{
						IQ re( IQ::Result, iq.from(), iq.id());
						re.addExtension( new XmppGSResult( XMLNS_GSIOT_STATE, defGSReturn_NoAuth ) );
						XmppClientSend(re,"handleIq Send(Get ExtIotState ACK)");
						return true;
					}

					std::list<GSIOTDevice *> tempDevGetList;
					std::list<GSIOTDevice *>::const_iterator it = IotDeviceList.begin();
					for(;it!=IotDeviceList.end();it++)
					{
						GSIOTDevice *pTempDev = (*it);

						if( !pTempDev->GetEnable() )
						{
							continue;
						}

						defUserAuth curAuth = m_cfg->m_UserMgr.check_Auth( pUser, pTempDev->getType(), pTempDev->getId() );

						if( GSIOTUser::JudgeAuth( curAuth, defUserAuth_RO ) )
						{
							tempDevGetList.push_back(pTempDev);
						}
					}
			
					IQ re( IQ::Result, iq.from(), iq.id());					
					re.addExtension( new XmppGSState( struTagParam(), 
						deviceClient->GetAllCommunicationState(true),//deviceClient->GetPortState(), 
						//IsRUNCODEEnable(defCodeIndex_SYS_GSM) ? ( deviceClient->GetGSM().GetGSMState()==GSMProcess::defGSMState_OK ? 1:0 ) : -1,
						IsRUNCODEEnable(defCodeIndex_SYS_GSM) ?  0 : -1,  //jyc20170223 modify //-1, //jyc20160923 modify
					    GetAlarmGuardGlobalFlag(),//global GetAllEventsState()
						this->GetAlarmGuardCurState(),
						this->m_IOT_starttime,
						tempDevGetList
						) );
					XmppClientSend(re,"handleIq Send(Get ExtIotState ACK)");
					return true;
				}
				
				XmppGSChange *pExXmppGSChange = (XmppGSChange*)iq.findExtension(ExtIotChange);
				if( pExXmppGSChange )
				{
					IQ re( IQ::Result, iq.from(), iq.id());
					re.addExtension( new XmppGSChange( struTagParam(), RUNCODE_Get(defCodeIndex_SYS_Change_Global), RUNCODE_Get(defCodeIndex_SYS_Change_Global,defRunCodeValIndex_2) ) );
					XmppClientSend(re,"handleIq Send(Get ExtIotChange ACK)");
					return true;
				}

				XmppGSEvent *pExXmppGSEvent = (XmppGSEvent*)iq.findExtension(ExtIotEvent);
				if( pExXmppGSEvent )
				{
					defUserAuth curAuth = m_cfg->m_UserMgr.check_Auth( pUser, IOT_Module_event, defAuth_ModuleDefaultID );
					if( !GSIOTUser::JudgeAuth( curAuth, defUserAuth_RO ) )
					{
						IQ re( IQ::Result, iq.from(), iq.id());
						re.addExtension( new XmppGSResult( XMLNS_GSIOT_EVENT, defGSReturn_NoAuth ) );
						XmppClientSend(re,"handleIq Send(Get ExtIotEvent ACK)");
						return true;
					}

					if( !pExXmppGSEvent->GetDevice() )
					{
						IQ re( IQ::Result, iq.from(), iq.id());
						re.addExtension( new XmppGSResult( XMLNS_GSIOT_EVENT, defGSReturn_Err ) );
						XmppClientSend(re,"handleIq Send(Get ExtIotEvent ACK)");
						return true;
					}

					const std::list<ControlEvent*> &EventsSrc = pExXmppGSEvent->GetEventList();
					std::list<ControlEvent*> EventsDest;

					std::list<ControlEvent *> evtList = m_event->GetEvents();
					std::list<ControlEvent *>::const_iterator it = evtList.begin();
					for(;it!=evtList.end();it++)
					{
						if( (*it)->GetDeviceType() == pExXmppGSEvent->GetDevice()->getType()
							&& (*it)->GetDeviceID() == pExXmppGSEvent->GetDevice()->getId()
							)
						{
							ControlEvent *pClone = (*it)->clone();
							EventsDest.push_back( pClone );

							switch( pClone->GetType() )
							{
							case CONTROL_Event:
								{
									AutoControlEvent *aevt = (AutoControlEvent*)pClone;

									const GSIOTDevice *pCtlDev = this->GetIOTDevice( aevt->GetControlDeviceType(), aevt->GetControlDeviceId() );
									if( pCtlDev )
									{
										aevt->AddEditAttr( "ctrl_devtype_name", pCtlDev->getName() );
										aevt->AddEditAttr( "address_name", GetDeviceAddressName( pCtlDev, aevt->GetAddress() ) );
									}
								}
								break;

							case Eventthing_Event:
								{
									AutoEventthing *aevt = (AutoEventthing*)pClone;

									const GSIOTDevice *pCtlDev = this->GetIOTDevice( aevt->GetControlDeviceType(), aevt->GetControlDeviceId() );
									if( pCtlDev )
									{
										aevt->AddEditAttr( "ctrl_devtype_name", pCtlDev->getName() );
									}
								}
								break;
							}
						}
					}
					
					GSIOTDevice *pDevice = this->GetIOTDevice( pExXmppGSEvent->GetDevice()->getType(), pExXmppGSEvent->GetDevice()->getId() );
					bool AGRunState = false;
					if( pDevice && pDevice->getControl() )
					{
						switch( pDevice->getType() )
						{
							case IOT_DEVICE_Trigger:
							{
								TriggerControl *ctl = (TriggerControl*)pDevice->getControl();
								AGRunState = ctl->GetAGRunState();
							}
							break;

						case IOT_DEVICE_Camera:
							{
								IPCameraBase *ctl = (IPCameraBase*)pDevice->getControl();
								AGRunState = ctl->GetAGRunState();
							}
							break;

						default:
							break;
						}
					}

					IQ re( IQ::Result, iq.from(), iq.id());
					re.addExtension( new XmppGSEvent(pExXmppGSEvent->GetSrcMethod(), pExXmppGSEvent->GetDevice(), EventsDest, AGRunState, struTagParam(), true ) );
					XmppClientSend(re,"handleIq Send(Get ExtIotEvent ACK)");
					return true;
				}
				//jyc20170330 resume
				XmppGSTalk *pExXmppGSTalk = (XmppGSTalk*)iq.findExtension(ExtIotTalk);
				if( pExXmppGSTalk )
				{
					handleIq_Set_XmppGSTalk( pExXmppGSTalk, iq, pUser );
					return true;
				}
				//jyc20170405 resume
				XmppGSPlayback *pExXmppGSPlayback = (XmppGSPlayback*)iq.findExtension(ExtIotPlayback);
				if( pExXmppGSPlayback )
				{
					handleIq_Set_XmppGSPlayback( pExXmppGSPlayback, iq, pUser );
					return true;
				}

				XmppGSRelation *pExXmppGSRelation = (XmppGSRelation*)iq.findExtension(ExtIotRelation);
				if( pExXmppGSRelation )
				{
					handleIq_Get_XmppGSRelation( pExXmppGSRelation, iq, pUser );
					return true;
				}
				//jyc20170405 resume
				XmppGSPreset *pExXmppGSPreset = (XmppGSPreset*)iq.findExtension(ExtIotPreset);
				if( pExXmppGSPreset )
				{
					handleIq_Get_XmppGSPreset( pExXmppGSPreset, iq, pUser );
					return true;
				}				

				XmppGSReport *pExXmppGSReport = (XmppGSReport*)iq.findExtension(ExtIotReport);
				if( pExXmppGSReport )
				{
					handleIq_Get_XmppGSReport( pExXmppGSReport, iq, pUser );
					return true;
				}

				XmppGSVObj *pExXmppGSVObj = (XmppGSVObj*)iq.findExtension(ExtIotVObj);
				if( pExXmppGSVObj )
				{
					handleIq_Get_XmppGSVObj( pExXmppGSVObj, iq, pUser );
					return true;
				}
				//jyc20170330 resume but no XmppGSTrans
				/*XmppGSTrans *pExXmppGSTrans = (XmppGSTrans*)iq.findExtension( ExtIotTrans );
				if( pExXmppGSTrans )
				{
					handleIq_Get_XmppGSTrans( pExXmppGSTrans, iq, pUser );
					return true;
				}*/
				
				XmppGSUpdate *pExXmppGSUpdate = (XmppGSUpdate*)iq.findExtension(ExtIotUpdate);
				if( pExXmppGSUpdate )
				{
					handleIq_Set_XmppGSUpdate( pExXmppGSUpdate, iq, pUser );
					return true;
				}
				
				GSIOTInfo *iotInfo = (GSIOTInfo *)iq.findExtension(ExtIot);
				if(iotInfo){
					std::list<GSIOTDevice *> tempDevGetList;
					std::list<GSIOTDevice *>::const_iterator	it = IotDeviceList.begin();
							
					for(;it!=IotDeviceList.end();it++)
					{
						GSIOTDevice *pTempDev = (*it);
						if( !iotInfo->isAllType() )
						{
							if( !iotInfo->isInGetType( pTempDev->getType() ) )
							{
								continue;
							}
						}

						if( !pTempDev->GetEnable() )
						{
							continue;
						}
 
						defUserAuth curAuth = m_cfg->m_UserMgr.check_Auth( pUser, pTempDev->getType(), pTempDev->getId() );

						if( GSIOTUser::JudgeAuth( curAuth, defUserAuth_RO ) )
						{
							tempDevGetList.push_back(pTempDev);
						}
					}
					
					
					IQ re( IQ::Result, iq.from(), iq.id());
					re.addExtension(new GSIOTInfo(tempDevGetList));
						
					XmppClientSend(re,"handleIq Send(Get ExtIot ACK)");
					
					tempDevGetList.clear();
					return true;
				}
				
				
				GSIOTDeviceInfo *deviceInfo = (GSIOTDeviceInfo *)iq.findExtension(ExtIotDeviceInfo);
				if(deviceInfo){
					GSIOTDevice *device = deviceInfo->GetDevice();

					if(device){
						std::list<GSIOTDevice *>::const_iterator it = IotDeviceList.begin();
						for(;it!=IotDeviceList.end();it++){
							if((*it)->getId() == device->getId() && (*it)->getType() == device->getType()){

								if( !(*it)->GetEnable() )
								{
									IQ re( IQ::Result, iq.from(), iq.id() );
									re.addExtension( new XmppGSResult( XMLNS_GSIOT_DEVICE, defGSReturn_NoExist ) );
									XmppClientSend( re, "handleIq Send(Get ExtIotDeviceInfo ACK)" );
									return true;
								}
								
								defUserAuth curAuth = m_cfg->m_UserMgr.check_Auth( pUser, device->getType(), device->getId() );
								if( !GSIOTUser::JudgeAuth( curAuth, defUserAuth_RO ) )
								{
									printf( "(%s)IQ::Get ExtIotDeviceInfo: no auth., curAuth=%d, devType=%d, devID=%d", iq.from().bare().c_str(), curAuth, device->getType(), device->getId() );

									IQ re( IQ::Result, iq.from(), iq.id());
									re.addExtension( new XmppGSResult( XMLNS_GSIOT_DEVICE, defGSReturn_NoAuth ) );
									XmppClientSend(re,"handleIq Send(Get ExtIotDeviceInfo ACK)");
									return true;
								}
								

								if( deviceInfo->isShare() )
								{
									const defUserAuth guestAuth = m_cfg->m_UserMgr.check_Auth( m_cfg->m_UserMgr.GetUser(XMPP_GSIOTUser_Guest), device->getType(), device->getId() );
									curAuth = ( defUserAuth_RW==guestAuth ) ? defUserAuth_RW : defUserAuth_RO;
								}
								
								IQ re( IQ::Result, iq.from(), iq.id());
								re.addExtension(new GSIOTDeviceInfo(*it, curAuth, deviceInfo->isShare()?defRunCodeVal_Spec_Enable:0) );
								XmppClientSend(re,"handleIq Send(Get ExtIotDeviceInfo ACK)");
								return true;
							}
						}
					}
				    return true;
				}
				break;
			}
		case IQ::Set:
			{
				GSIOTUser *pUser = m_cfg->m_UserMgr.check_GetUser( iq.from().bare() );

				this->m_cfg->FixOwnerAuth(pUser); //jyc20170301 note if have authority

				defGSReturn ret = m_cfg->m_UserMgr.check_User(pUser);
				 if( macGSFailed(ret) )  
				{
					printf( "(%s)IQ::Set: Not found userinfo. no auth.", iq.from().bare().c_str() );

					IQ re( IQ::Result, iq.from(), iq.id());
					re.addExtension( new XmppGSResult( "all", defGSReturn_NoAuth ) );
					XmppClientSend(re,"handleIq Send(all Set ACK)");
					return true;
				}
				
				GSIOTHeartbeat *heartbeat = (GSIOTHeartbeat *)iq.findExtension(ExtIotHeartbeat);
				if(heartbeat){ //jyc20170330 resume
					ipcamClient->UpdateRTMPSession(iq.from(),heartbeat->GetDeviceID());
				    return true;
				}

				XmppGSState *pExXmppGSState = (XmppGSState*)iq.findExtension(ExtIotState);
				if( pExXmppGSState )
				{
					switch( pExXmppGSState->get_cmd() )
					{
					case XmppGSState::defStateCmd_events:
						{
							defUserAuth curAuth = m_cfg->m_UserMgr.check_Auth( pUser, IOT_Module_system, defAuth_ModuleDefaultID );
							if( !GSIOTUser::JudgeAuth( curAuth, defUserAuth_WO ) )
							{
								IQ re( IQ::Result, iq.from(), iq.id());
								re.addExtension( new XmppGSResult( XMLNS_GSIOT_STATE, defGSReturn_NoAuth ) );
								XmppClientSend(re,"handleIq Send(Get ExtIotState ACK)");
								return true;
							}

#if 1
							SetAlarmGuardGlobalFlag( pExXmppGSState->get_state_events() ); // this->SetAllEventsState( pExXmppGSState->get_state_events(), "from xmpp", false );
#else
							AutoEventthing aEvt;
							aEvt.SetAllDevice();
							aEvt.SetRunState( pExXmppGSState->get_state_events() );

							DoControlEvent_Eventthing( &aEvt, &aEvt, "Set ExtIotState", false );
#endif

							// ack
							IQ re( IQ::Result, iq.from(), iq.id());
							re.addExtension( new XmppGSResult( XMLNS_GSIOT_STATE, defGSReturn_Success ) );
							XmppClientSend(re,"handleIq Send(Get ExtIotState ACK)");
						}
						break;
						
					case XmppGSState::defStateCmd_alarmguard:
						{
							defUserAuth curAuth = m_cfg->m_UserMgr.check_Auth( pUser, IOT_Module_system, defAuth_ModuleDefaultID );
							if( !GSIOTUser::JudgeAuth( curAuth, defUserAuth_WO ) )
							{
								IQ re( IQ::Result, iq.from(), iq.id());
								re.addExtension( new XmppGSResult( XMLNS_GSIOT_STATE, defGSReturn_NoAuth ) );
								XmppClientSend(re,"handleIq Send(Get ExtIotState ACK)");
								return true;
							}

							const std::map<int,XmppGSState::struAGTime> &mapagTimeRef = pExXmppGSState->get_mapagTime();
							for( std::map<int,XmppGSState::struAGTime>::const_iterator it=mapagTimeRef.begin(); it!=mapagTimeRef.end(); ++it )
							{
								const int vecagt_size = it->second.vecagTime.size();
								const int agtime1 = vecagt_size>0 ? it->second.vecagTime[0]:0;
								const int agtime2 = vecagt_size>1 ? it->second.vecagTime[1]:0;
								const int agtime3 = vecagt_size>2 ? it->second.vecagTime[2]:0;

								int allday = it->second.allday;
								
								this->m_RunCodeMgr.SetCodeAndSaveDb( g_AlarmGuardTimeWNum2Index(it->first), allday, agtime1, agtime2, agtime3, true, true, true, true );
							}

							// ack
							IQ re( IQ::Result, iq.from(), iq.id());
							re.addExtension( new XmppGSResult( XMLNS_GSIOT_STATE, defGSReturn_Success ) );
							XmppClientSend(re,"handleIq Send(Set ExtIotState ACK)");
						}
						break;
						
					case XmppGSState::defStateCmd_exitlearnmod:
						{
							defUserAuth curAuth = m_cfg->m_UserMgr.check_Auth( pUser, IOT_Module_system, defAuth_ModuleDefaultID );
							if( !GSIOTUser::JudgeAuth( curAuth, defUserAuth_WO ) )
							{
								IQ re( IQ::Result, iq.from(), iq.id());
								re.addExtension( new XmppGSResult( XMLNS_GSIOT_STATE, defGSReturn_NoAuth ) );
								XmppClientSend(re,"handleIq Send(Set ExtIotState ACK)");
								return true;
							}

							this->deviceClient->SendMOD_set( defMODSysSet_IR_TXCtl_TX, defLinkID_All );
						}
						break;

					case XmppGSState::defStateCmd_reboot:
						{
							defUserAuth curAuth = m_cfg->m_UserMgr.check_Auth( pUser, IOT_Module_reboot, defAuth_ModuleDefaultID );
							if( !GSIOTUser::JudgeAuth( curAuth, defUserAuth_WO ) )
							{
								IQ re( IQ::Result, iq.from(), iq.id());
								re.addExtension( new XmppGSResult( XMLNS_GSIOT_STATE, defGSReturn_NoAuth ) );
								XmppClientSend(re,"handleIq Send(Set ExtIotState ACK)");
								return true;
							}
							//jyc20160923 notice
							//sys_reset( iq.from().full().c_str(), 1 );
						}
						break;
					}

					return true;
				}

				XmppGSAuth *pExXmppGSAuth = (XmppGSAuth*)iq.findExtension(ExtIotAuthority);
				if( pExXmppGSAuth )
				{
					defUserAuth curAuth = m_cfg->m_UserMgr.check_Auth( pUser, IOT_Module_authority, defAuth_ModuleDefaultID );
					if( !GSIOTUser::JudgeAuth( curAuth, defUserAuth_WO ) )
					{
						IQ re( IQ::Result, iq.from(), iq.id());
						re.addExtension( new XmppGSResult( XMLNS_GSIOT_AUTHORITY, defGSReturn_NoAuth ) );
						XmppClientSend(re,"handleIq Send(Get ExtIotAuthority ACK)");
						return true;
					}

					//jyc20170301 modify
					//this->AddGSMessage( new GSMessage(defGSMsgType_Notify, iq.from(), iq.id(), pExXmppGSAuth->clone() ) );
					GSMessage *pMsg = new GSMessage(defGSMsgType_Notify, iq.from(), iq.id(), pExXmppGSAuth->clone() );			
					handleIq_Set_XmppGSAuth( pMsg );
					delete pMsg;
					return true;
				}

				XmppGSManager *pExXmppGSManager = (XmppGSManager*)iq.findExtension(ExtIotManager);
				if( pExXmppGSManager )
				{
					defUserAuth curAuth = m_cfg->m_UserMgr.check_Auth( pUser, IOT_Module_manager, defAuth_ModuleDefaultID );
					if( !GSIOTUser::JudgeAuth( curAuth, defUserAuth_WO ) )
					{
						IQ re( IQ::Result, iq.from(), iq.id());
						re.addExtension( new XmppGSResult( XMLNS_GSIOT_MANAGER, defGSReturn_NoAuth ) );
						XmppClientSend(re,"handleIq Send(Get ExtIotManager ACK)");
						return true;
					}
					//jyc20170301 modify
					//this->AddGSMessage( new GSMessage(defGSMsgType_Notify, iq.from(), iq.id(), pExXmppGSManager->clone() ) );
					GSMessage *pMsg = new GSMessage(defGSMsgType_Notify, iq.from(), iq.id(), pExXmppGSManager->clone() );			
					handleIq_Set_XmppGSManager( pMsg );
					delete pMsg;
					return true;
				}
				
				XmppGSEvent *pExXmppGSEvent = (XmppGSEvent*)iq.findExtension(ExtIotEvent);
				if( pExXmppGSEvent )
				{
					defUserAuth curAuth = m_cfg->m_UserMgr.check_Auth( pUser, IOT_Module_event, defAuth_ModuleDefaultID );
					if( !GSIOTUser::JudgeAuth( curAuth, defUserAuth_WO ) )
					{
						IQ re( IQ::Result, iq.from(), iq.id());
						re.addExtension( new XmppGSResult( XMLNS_GSIOT_EVENT, defGSReturn_NoAuth ) );
						XmppClientSend(re,"handleIq Send(Get ExtIotEvent ACK)");
						return true;
					}
					//jyc20170227 modify
					//this->AddGSMessage( new GSMessage(defGSMsgType_Notify, iq.from(), iq.id(), pExXmppGSEvent->clone() ) );
					GSMessage *pMsg = new GSMessage(defGSMsgType_Notify, iq.from(), iq.id(), pExXmppGSEvent->clone() );
					handleIq_Set_XmppGSEvent( pMsg );
					delete pMsg;
					
					return true;
				}
				//jyc20170330 resume
				XmppGSTalk *pExXmppGSTalk = (XmppGSTalk*)iq.findExtension(ExtIotTalk);
				if( pExXmppGSTalk )
				{
					handleIq_Set_XmppGSTalk( pExXmppGSTalk, iq, pUser );
					return true;
				}

				XmppGSRelation *pExXmppGSRelation = (XmppGSRelation*)iq.findExtension(ExtIotRelation);
				if( pExXmppGSRelation )
				{
					defUserAuth curAuth = m_cfg->m_UserMgr.check_Auth( pUser, IOT_Module_manager, defAuth_ModuleDefaultID );
					if( !GSIOTUser::JudgeAuth( curAuth, defUserAuth_WO ) )
					{
						IQ re( IQ::Result, iq.from(), iq.id());
						re.addExtension( new XmppGSResult( XMLNS_GSIOT_RELATION, defGSReturn_NoAuth ) );
						XmppClientSend(re,"handleIq Send(Set ExtIotRelation ACK)");
						return true;
					}

					curAuth = m_cfg->m_UserMgr.check_Auth( pUser, pExXmppGSRelation->get_device_type(), pExXmppGSRelation->get_device_id() );
					if( !GSIOTUser::JudgeAuth( curAuth, defUserAuth_WO ) )
					{
						IQ re( IQ::Result, iq.from(), iq.id());
						re.addExtension( new XmppGSResult( XMLNS_GSIOT_RELATION, defGSReturn_NoAuth ) );
						XmppClientSend(re,"handleIq Send(Set ExtIotRelation ACK)");
						return true;
					}
					//jyc20170301 modify
					//this->AddGSMessage( new GSMessage(defGSMsgType_Notify, iq.from(), iq.id(), pExXmppGSRelation->clone() ) );
					GSMessage *pMsg = new GSMessage(defGSMsgType_Notify, iq.from(), iq.id(), pExXmppGSRelation->clone() );
					handleIq_Set_XmppGSRelation( pMsg );
					delete pMsg;
					return true;
				}
				//jyc20170405 resume
				XmppGSPreset *pExXmppGSPreset = (XmppGSPreset*)iq.findExtension(ExtIotPreset);
				if( pExXmppGSPreset )
				{
					if( XmppGSPreset::defPSMethod_goto == pExXmppGSPreset->GetMethod() )
					{
						defGSReturn result = defGSReturn_Success;

						if( IOT_DEVICE_Camera == pExXmppGSPreset->get_device_type() )
						{
							const CPresetObj *pPresetIn = pExXmppGSPreset->GetFristPreset();
							if( pPresetIn )
							{
								const defUserAuth curAuth = m_cfg->m_UserMgr.check_Auth( pUser, pExXmppGSPreset->get_device_type(), pExXmppGSPreset->get_device_id() );

								if( GSIOTUser::JudgeAuth( curAuth, defUserAuth_RW ) )
								{
									const GSIOTDevice *device = this->GetIOTDevice( pExXmppGSPreset->get_device_type(), pExXmppGSPreset->get_device_id() );
									if( device )
									{
										IPCameraBase *ctl = (IPCameraBase*)device->getControl();

										if( ctl->GetAdvAttr().get_AdvAttr( defCamAdvAttr_PTZ_Preset ) )
										{
											const CPresetObj *pPresetLocal = ctl->GetPreset(pPresetIn->GetId());
											if( pPresetLocal )
											{
												if( !ctl->SendPTZ( GSPTZ_Goto_Preset, pPresetLocal->GetIndex() ) )
												{
													result = defGSReturn_Err;
												}
											}
											else
											{
												result = defGSReturn_NoExist;
											}
										}
										else
										{
											result = defGSReturn_FunDisable;
										}
									}
									else
									{
										result = defGSReturn_NoExist;
									}
								}
								else
								{
									result = defGSReturn_NoAuth;
								}
							}
							else
							{
								result = defGSReturn_ErrParam;
							}
						}
						else
						{
							result = defGSReturn_UnSupport;
						}

						IQ re( IQ::Result, iq.from(), iq.id() );
						re.addExtension( new XmppGSPreset(struTagParam(true,true), pExXmppGSPreset->GetSrcMethod(), pExXmppGSPreset->get_device_type(), pExXmppGSPreset->get_device_id(), defPresetQueue(), result ) );
						XmppClientSend(re,"handleIq Send(Set ExtIotPreset ACK)");
						return true;
					}

					defUserAuth curAuth = m_cfg->m_UserMgr.check_Auth( pUser, IOT_Module_manager, defAuth_ModuleDefaultID );
					if( !GSIOTUser::JudgeAuth( curAuth, defUserAuth_WO ) )
					{
						// ack
						IQ re( IQ::Result, iq.from(), iq.id());
						re.addExtension( new XmppGSPreset(struTagParam(true,true), pExXmppGSPreset->GetSrcMethod(), pExXmppGSPreset->get_device_type(), pExXmppGSPreset->get_device_id(), defPresetQueue(), defGSReturn_NoAuth ) );
						XmppClientSend(re,"handleIq Send(Set ExtIotPreset ACK)");
						return true;
					}

					//this->AddGSMessage( new GSMessage(defGSMsgType_Notify, iq.from(), iq.id(), pExXmppGSPreset->clone() ) );
					GSMessage *pMsg = new GSMessage(defGSMsgType_Notify, iq.from(), iq.id(), pExXmppGSPreset->clone() );
					handleIq_Set_XmppGSPreset( pMsg );
					delete pMsg;
					return true;
				}

				XmppGSVObj *pExXmppGSVObj = (XmppGSVObj*)iq.findExtension(ExtIotVObj);
				if( pExXmppGSVObj )
				{
					defUserAuth curAuth = m_cfg->m_UserMgr.check_Auth( pUser, IOT_Module_manager, defAuth_ModuleDefaultID );
					if( !GSIOTUser::JudgeAuth( curAuth, defUserAuth_WO ) )
					{
						// ack
						IQ re( IQ::Result, iq.from(), iq.id());
						re.addExtension( new XmppGSVObj(struTagParam(true,true), 
						                                pExXmppGSVObj->GetSrcMethod(), 
						                                defmapVObjConfig(), 
						                                defGSReturn_NoAuth ) );
						XmppClientSend(re,"handleIq Send(Set ExtIotVObj ACK)");
						return true;
					}
					//jyc20170318 modify
					//this->AddGSMessage( new GSMessage(defGSMsgType_Notify, iq.from(), iq.id(), pExXmppGSVObj->clone() ) );
					GSMessage *pMsg = new GSMessage(defGSMsgType_Notify, iq.from(), iq.id(), pExXmppGSVObj->clone() );
					handleIq_Set_XmppGSVObj( pMsg );
					delete pMsg;
					return true;
				}

				XmppGSUpdate *pExXmppGSUpdate = (XmppGSUpdate*)iq.findExtension(ExtIotUpdate);
				if( pExXmppGSUpdate )
				{
					handleIq_Set_XmppGSUpdate( pExXmppGSUpdate, iq, pUser );
					return true;
				}

				GSIOTControl *iotControl = (GSIOTControl *)iq.findExtension(ExtIotControl);
				if(iotControl){
					GSIOTDevice *device = iotControl->getDevice();
					if(device){
						GSIOTDevice *pLocalDevice = NULL;
												
						if( 0 != device->getId() )
						{  
							pLocalDevice = this->GetIOTDevice( device->getType(), device->getId() );
							if( !pLocalDevice )
							{
								printf( "(%s)IQ::Set: dev not found, devType=%d, devID=%d", iq.from().bare().c_str(), device->getType(), device->getId() );
								return true;
							}

							if( !pLocalDevice->getControl() )
							{
								printf( "(%s)IQ::Set: dev ctl err, devType=%d, devID=%d", iq.from().bare().c_str(), device->getType(), device->getId() );
								return true;
							}

							if( !pLocalDevice->GetEnable() )
							{
								printf( "(%s)IQ::Set: dev disabled, devType=%d, devID=%d", iq.from().bare().c_str(), device->getType(), device->getId() );
								return true;
							}

							if( pLocalDevice )
							{
								if( device->GetLinkID() != pLocalDevice->GetLinkID() )
								{
									device->SetLinkID( pLocalDevice->GetLinkID() );
								}
							}
						}
						defUserAuth curAuth = m_cfg->m_UserMgr.check_Auth( pUser, device->getType(), device->getId() );
						
						if( defUserAuth_Null == curAuth )
						{
							if( iotControl->getNeedRet() )
							{
								IQ re( IQ::Result, iq.from(), iq.id());
								re.addExtension( new XmppGSResult( XMLNS_GSIOT_CONTROL, defGSReturn_NoAuth ) );
								XmppClientSend(re,"handleIq Send(Set iotControl NoAuth ACK)");
							}

							printf( "(%s)IQ::Set: no auth., curAuth=%d, devType=%d, devID=%d", iq.from().bare().c_str(), curAuth, device->getType(), device->getId() );
							return true;
						}

						if(device->getControl()){
							switch(device->getType())
							{
							case IOT_DEVICE_Camera:
								{
									/*jyc20170331 resume cam auth control*/
									CameraControl *cam_ctl = (CameraControl *)device->getControl();
									const CameraPTZ *ptz = cam_ctl->getPtz();
									const CameraFocal *focal = cam_ctl->getFocal();
									if( cam_ctl->getPTZFlag() && ptz )
									{
										if( !GSIOTUser::JudgeAuth( curAuth, defUserAuth_RW ) )
										{
											//LOGMSGEX( defLOGNAME, defLOG_INFO, "(%s)IQ::Set ptz: no auth., curAuth=%d, devType=%d, devID=%d", iq.from().bare().c_str(), curAuth, device->getType(), device->getId() );
											return true;
										}

										LOGMSG( "cam_ctl ptz devid=%d, cmd=%d, auto=%d, speed=%d\r\n", device->getId(), ptz->getCommand(), ptz->getAutoflag(), ptz->getSpeed() );
										ipcamClient->SendPTZ( device->getId(), (GSPTZ_CtrlCmd)ptz->getCommand(), ptz->getAutoflag(), 0, ptz->getSpeed() );
									}
									else if(  cam_ctl->getFocalFlag() && focal )
									{
										if( !GSIOTUser::JudgeAuth( curAuth, defUserAuth_RW ) )
										{
											//LOGMSGEX( defLOGNAME, defLOG_INFO, "(%s)IQ::Set focal: no auth., curAuth=%d, devType=%d, devID=%d", iq.from().bare().c_str(), curAuth, device->getType(), device->getId() );
											return true;
										}

										LOGMSG( "cam_ctl focal devid=%d, cmd=%d, auto=%d\r\n", device->getId(), focal->GetZoom(), focal->getAutoflag() );
										ipcamClient->SendPTZ( device->getId(), (GSPTZ_CtrlCmd)focal->GetZoom(), focal->getAutoflag(), 0 );
									}
									else if( cam_ctl->HasCmdCtl() )
									{
										if( !GSIOTUser::JudgeAuth( curAuth, defUserAuth_RW ) )
										{
											//LOGMSGEX( defLOGNAME, defLOG_INFO, "(%s)IQ::Set track: no auth., curAuth=%d, devType=%d, devID=%d", iq.from().bare().c_str(), curAuth, device->getType(), device->getId() );
											return true;
										}

										LOGMSG( "cam_ctl track devid=%d, cmd=%d, x=%d, y=%d\r\n", device->getId(), cam_ctl->get_trackCtl(), cam_ctl->get_trackX(), cam_ctl->get_trackY() );

										switch( cam_ctl->get_trackCtl() )
										{
										case GSPTZ_MOTION_TRACK_Enable:
											{
												if( !ipcamClient->SendPTZ( device->getId(), GSPTZ_MOTION_TRACK_Enable, 0 ) )
												{
													IQ re( IQ::Result, iq.from(), iq.id());
													re.addExtension( new XmppGSResult( XMLNS_GSIOT_CONTROL, defGSReturn_Err, defNormResultMod_control_MotionTrack ) );
													XmppClientSend(re,"handleIq Send(MOTION_TRACK_Enable failed ACK)");
												}
											}
											break;

										case GSPTZ_MOTION_TRACK_Disable:
											{
												if( !ipcamClient->SendPTZ( device->getId(), GSPTZ_MOTION_TRACK_Disable, 0 ) )
												{
													IQ re( IQ::Result, iq.from(), iq.id());
													re.addExtension( new XmppGSResult( XMLNS_GSIOT_CONTROL, defGSReturn_Err, defNormResultMod_control_MotionTrack ) );
													XmppClientSend(re,"handleIq Send(MOTION_TRACK_Disable failed ACK)");
												}
											}
											break;

										case GSPTZ_MANUALTRACE:
										case GSPTZ_MANUALPTZSel:
											{
												int xpos = cam_ctl->get_trackX();
												int ypos = cam_ctl->get_trackY();
												GSPTZ_CtrlCmd ctrlcmd = cam_ctl->get_trackCtl();

												if( GSPTZ_MANUALTRACE == ctrlcmd )
												{
													if( IsRUNCODEEnable(defCodeIndex_SYS_PTZ_TRACE2PTZSel) )
													{
														IPCameraBase *pcam = ipcamClient->GetCamera(device->getId());
														if( pcam && !pcam->isRight_manual_trace() )
														{
															LOGMSG( "cam_ctl track devid=%d, SYS_PTZ_TRACE2PTZSel", device->getId() );

															ctrlcmd = GSPTZ_MANUALPTZSel;
														}
													}
												}

												ipcamClient->SendPTZ( device->getId(), ctrlcmd, xpos*10000 + ypos );
											}
											break;

										case GSPTZ_MANUALZoomRng:
											{
												const int xpos = cam_ctl->get_trackX();
												const int ypos = cam_ctl->get_trackY();
												const int EndXpos = cam_ctl->get_trackEndX();
												const int EndYpos = cam_ctl->get_trackEndY();
												const GSPTZ_CtrlCmd ctrlcmd = cam_ctl->get_trackCtl();

												ipcamClient->SendPTZ( device->getId(), ctrlcmd, xpos*10000 + ypos, EndXpos*10000 + EndYpos );
											}
											break;

										case GSPTZ_PTZ_ParkAction_Enable:
											{
												if( !ipcamClient->SendPTZ( device->getId(), GSPTZ_PTZ_ParkAction_Enable, 0 ) )
												{
													IQ re( IQ::Result, iq.from(), iq.id());
													re.addExtension( new XmppGSResult( XMLNS_GSIOT_CONTROL, defGSReturn_Err, defNormResultMod_control_PTZ_ParkAction ) );
													XmppClientSend(re,"handleIq Send(PTZ_ParkAction_Enable failed ACK)");
												}
											}
											break;

										case GSPTZ_PTZ_ParkAction_Disable:
											{
												if( !ipcamClient->SendPTZ( device->getId(), GSPTZ_PTZ_ParkAction_Disable, 0 ) )
												{
													IQ re( IQ::Result, iq.from(), iq.id());
													re.addExtension( new XmppGSResult( XMLNS_GSIOT_CONTROL, defGSReturn_Err, defNormResultMod_control_PTZ_ParkAction ) );
													XmppClientSend(re,"handleIq Send(PTZ_ParkAction_Disable failed ACK)");
												}
											}
											break;

										case GSPTZ_DoPrePic:
											{
												defGSReturn ret = defGSReturn_FunDisable;
												if( m_IGSMessageHandler )
												{
													ret = m_IGSMessageHandler->OnControlOperate( defCtrlOprt_DoPrePic, device->getExType(), device, NULL );
												}

												if( macGSFailed( ret ) )
												{
													IQ re( IQ::Result, iq.from(), iq.id() );
													re.addExtension( new XmppGSResult( XMLNS_GSIOT_CONTROL, ret ) );
													XmppClientSend( re, "handleIq Send(DoPrePic failed ACK)" );
												}
											}
											break;
										}
									}
									else if( cam_ctl->isReboot() )
									{
										if( !GSIOTUser::JudgeAuth( m_cfg->m_UserMgr.check_Auth( pUser, IOT_Module_reboot, defAuth_ModuleDefaultID ), defUserAuth_WO ) )
										{
											IQ re( IQ::Result, iq.from(), iq.id());
											re.addExtension( new XmppGSResult( XMLNS_GSIOT_CONTROL, defGSReturn_NoAuth ) );
											XmppClientSend(re,"handleIq Send(Set CamReboot ACK)");
											return true;
										}

										if( !GSIOTUser::JudgeAuth( m_cfg->m_UserMgr.check_Auth( pUser, IOT_DEVICE_Camera, device->getId() ), defUserAuth_WO ) )
										{
											IQ re( IQ::Result, iq.from(), iq.id());
											re.addExtension( new XmppGSResult( XMLNS_GSIOT_CONTROL, defGSReturn_NoAuth ) );
											XmppClientSend(re,"handleIq Send(Set CamReboot ACK)");
											return true;
										}

										if( ipcamClient->SendPTZ( device->getId(), GSPTZ_CameraReboot, 0 ) )
										{
											LOGMSG( "cam_ctl reboot devid=%d success.\r\n", device->getId() );
										}
										else{
											LOGMSG( "cam_ctl reboot devid=%d failed!\r\n", device->getId() );
										}
									}
									else{
										if( !GSIOTUser::JudgeAuth( curAuth, defUserAuth_RO ) ){
											//LOGMSGEX( defLOGNAME, defLOG_INFO, "(%s)IQ::Set rtmp: no auth., curAuth=%d, devType=%d, devID=%d", iq.from().bare().c_str(), curAuth, device->getType(), device->getId() );
											return true;
										}

										if(cam_ctl->getProtocol() == "rtmp" || cam_ctl->getProtocol() == "rtmfp"){
											if(cam_ctl->getStatus()=="play"){

												std::string url_use = cam_ctl->getFromUrl();
												std::vector<std::string> url_backup_use = cam_ctl->get_url_backup();

												if( IsRUNCODEEnable(defCodeIndex_TEST_StreamServer) )
												{
													const int val2 = RUNCODE_Get(defCodeIndex_TEST_StreamServer,defRunCodeValIndex_2);
													const int val3 = RUNCODE_Get(defCodeIndex_TEST_StreamServer,defRunCodeValIndex_3);

													char ssvr[32] = {0};
													snprintf( ssvr, sizeof(ssvr), "%d.%d.%d.%d", val2/1000, val2%1000, val3/1000, val3%1000 );

													g_replace_all_distinct( url_use, "www.gsss.cn", ssvr );

													for( uint32_t iurlbak=0; iurlbak<url_backup_use.size(); ++iurlbak )
													{
														g_replace_all_distinct( url_backup_use[iurlbak], "www.gsss.cn", ssvr );

													#ifdef _DEBUG
														LOGMSG( "urlbak=%s", url_backup_use[iurlbak].c_str() );
													#endif
													}
												}

												if( device->GetEnable() ){
													//jyc20170511 resume
													this->PlayMgrCmd_push( defPlayMgrCmd_Start, curAuth, iq, device->getId(), url_use, url_backup_use );
												}
											}else{
												if( device->GetEnable() )
												{
													//jyc20170511 resume
													this->PlayMgrCmd_push( defPlayMgrCmd_Stop, curAuth, iq, device->getId(), "", cam_ctl->get_url_backup()  );
												}
											}
										}
										else{
											LOGMSG( "IQ::Set cam play: unsupport %s, from=%s", cam_ctl->getProtocol().c_str(), iq.from().bare().c_str() );
										}
									}
									return true;
								}
							case IOT_DEVICE_RFDevice:
								{
									return true;
								}
							case IOT_DEVICE_CANDevice:
								{
									return true;
								}

							case IOT_DEVICE_RS485:
								{
									/*rs485 author*/
									RS485DevControl *ctl = (RS485DevControl *)device->getControl();
									ctl->AddressQueueChangeToOneAddr();
									
									if( !GSIOTUser::JudgeAuth( curAuth, RS485DevControl::IsReadCmd( ctl->GetCommand() )?defUserAuth_RO:defUserAuth_WO ) )
									{
										if( iotControl->getNeedRet() )
										{
											IQ re( IQ::Result, iq.from(), iq.id());
											re.addExtension( new XmppGSResult( XMLNS_GSIOT_CONTROL, defGSReturn_NoAuth ) );
											XmppClientSend(re,"handleIq Send(Set RS485DevControl NoAuth ACK)");
										}

										printf( "(%s)IQ::Set RS485: no auth., curAuth=%d, devType=%d, devID=%d, cmd=%d", iq.from().bare().c_str(), curAuth, device->getType(), device->getId(), ctl->GetCommand() );

										return true;
									}

									RS485DevControl *pLocalCtl = (RS485DevControl*)pLocalDevice->getControl();

									const defAddressQueue &AddrQue = ctl->GetAddressList();
									defAddressQueue::const_iterator itAddrQue = AddrQue.begin();
									for( ; itAddrQue!=AddrQue.end(); ++itAddrQue )
									{
										DeviceAddress *pCurOneAddr = *itAddrQue; //

										if( !pCurOneAddr )
											continue;

										DeviceAddress *pLocalAddr = pLocalCtl->GetAddress( pCurOneAddr->GetAddress() );
										if( !pLocalAddr )
										{
											printf( "(%s)IQ::Set RS485: notfound addr=%d, devType=%d, devID=%d \n", iq.from().bare().c_str(), pCurOneAddr->GetAddress(), device->getType(), device->getId() );
											continue;
										}
#if 1
									if( RS485DevControl::IsReadCmd( ctl->GetCommand() ) )
									{
										if( pLocalAddr )
										{
											bool isOld = false;
											uint32_t noUpdateTime = 0;

											std::string strCurValue = pLocalAddr->GetCurValue( &isOld, &noUpdateTime );

											if( !isOld )
											{
												device->SetCurValue( pLocalAddr );

												IQ re( IQ::Result, iq.from(), iq.id() );
												re.addExtension( new GSIOTControl( pLocalDevice ) );
												XmppClientSend( re,"handleIq Send(RS485 Read fasttime<<<<<)" );
												return true;
											}
										}
									}
#endif
									GSIOTDevice *sendDev = pLocalDevice->clone(false);
									if( !sendDev )
										continue;

									RS485DevControl *sendCtl = (RS485DevControl*)sendDev->getControl();
									if( sendCtl )
									{				
										DeviceAddress *sendAddr = sendCtl->GetAddress(pLocalAddr->GetAddress());
										if( sendAddr )
										{
											uint32_t nextInterval = 1;
									

											sendCtl->SetCommand( ctl->GetCommand() );

											const bool IsWriteCmd = RS485DevControl::IsWriteCmd( sendCtl->GetCommand() );
											if( IsWriteCmd )
											{
												pLocalDevice->ResetUpdateState( pCurOneAddr->GetAddress() );
												sendAddr->SetCurValue( pCurOneAddr->GetCurValue() );

												// 
												if( pLocalAddr->GetAttrObj().get_AdvAttr(DeviceAddressAttr::defAttr_IsReSwitch)
													|| pLocalAddr->GetAttrObj().get_AdvAttr(DeviceAddressAttr::defAttr_IsAutoBackSwitch)
													)
												{
													const std::string ReSwitchValue = pLocalAddr->GetCurValue()=="1"?"0":"1";
													pLocalAddr->SetCurValue( ReSwitchValue );
													sendAddr->SetCurValue( ReSwitchValue );
												}
											}

											// is really cmd
											if( RS485DevControl::IsReadCmd( sendCtl->GetCommand() )
												&& !this->CheckControlMesssageQueue(sendDev,sendAddr,iq.from(),iq.id()) )
											{
												GSIOTDevice *dev = sendDev->clone(false);
												RS485DevControl *msgctl = (RS485DevControl *)dev->getControl();
												msgctl->AddressQueueChangeToOneAddr( sendAddr->GetAddress() );
												PushControlMesssageQueue( new ControlMessage( iq.from(), iq.id(), dev, msgctl->GetAddress(sendAddr->GetAddress()) ) );
											}

											// auto reset switch
											if( pLocalAddr->GetAttrObj().get_AdvAttr(DeviceAddressAttr::defAttr_IsAutoBackSwitch) )
											{
												nextInterval = 300;
											}

											this->SendControl( device->getType(), sendDev, sendAddr,
											                  defNormSendCtlOvertime, defNormMsgOvertime, nextInterval);
											if( IsWriteCmd )
											{
												if( iotControl->getNeedRet() )
												{
													IQ re( IQ::Result, iq.from(), iq.id());
													re.addExtension( new XmppGSResult( XMLNS_GSIOT_CONTROL, defGSReturn_SuccExecuted ) );
													XmppClientSend(re,"handleIq Send(Set RS485DevControl executed ACK)");
												}
											}

											// auto reset switch
											if( pLocalAddr->GetAttrObj().get_AdvAttr(DeviceAddressAttr::defAttr_IsAutoBackSwitch) )
											{
												const bool IsWriteCmd = RS485DevControl::IsWriteCmd( sendCtl->GetCommand() );
												if( IsWriteCmd )
												{
													const std::string AutoBackSwitchValue = sendAddr->GetCurValue()=="1"?"0":"1";
													pLocalAddr->SetCurValue( AutoBackSwitchValue );
													sendAddr->SetCurValue( AutoBackSwitchValue );
													this->SendControl( device->getType(), sendDev, sendAddr );
												}
											}

										}
									}
									macCheckAndDel_Obj(sendDev);
									}
									return true;
								}

							case IOT_DEVICE_Remote:
								{
									const RFRemoteControl *ctl = (RFRemoteControl *)device->getControl();
									const RFRemoteControl *localctl = (RFRemoteControl *)pLocalDevice->getControl();

									if( !GSIOTUser::JudgeAuth( curAuth, defUserAuth_WO ) )
									{
										if( iotControl->getNeedRet() )
										{
											IQ re( IQ::Result, iq.from(), iq.id() );
											re.addExtension( new XmppGSResult( XMLNS_GSIOT_CONTROL, defGSReturn_NoAuth ) );
											XmppClientSend( re, "handleIq Send(Set RFRemoteControl NoAuth ACK)" );
										}

										printf( "(%s)IQ::Set remote: no auth., curAuth=%d, devType=%d, devID=%d", iq.from().bare().c_str(), curAuth, device->getType(), device->getId() );
										return true;
									}

									switch( localctl->GetExType() )
									{
									case IOTDevice_AC_Ctl:
										{
											const defUserAuth curAuth_acctl = m_cfg->m_UserMgr.check_Auth( pUser, IOT_Module_acctl, defAuth_ModuleDefaultID );
											if( !GSIOTUser::JudgeAuth( curAuth_acctl, g_IsReadOnlyCmd( ctl->GetCmd() )?defUserAuth_RO:defUserAuth_WO ) )
											{
												IQ re( IQ::Result, iq.from(), iq.id() );
												re.addExtension( new XmppGSResult( XMLNS_GSIOT_CONTROL, defGSReturn_NoAuth ) );
												XmppClientSend( re, "handleIq Send(Set AC_Ctl ACK)" );
												return true;
											}
										}
										break;
									default:
										break;
									}

									// get button list
									const defButtonQueue &que = ctl->GetButtonList();
									defButtonQueue::const_iterator it = que.begin();
									defButtonQueue::const_iterator itEnd = que.end();
									for( ; it!=itEnd; ++it )
									{
										RemoteButton *pCurButton = *it;
										const RemoteButton *pLocalButton = localctl->GetButton( pCurButton->GetId() );

										if( pLocalButton )
										{
											if( IsRUNCODEEnable(defCodeIndex_TEST_Develop_NewFunc) )
											{
												const int testid = atoi(pLocalDevice->getVer().c_str());
												if( pLocalDevice->getVer().find("presetDEBUGDEVELOP")!=std::string::npos )
												{
													//ipcamClient->SendPTZ( testid, GSPTZ_Goto_Preset, pLocalButton->GetSignalSafe().original[0] );
													continue;
												}
												else
												{
													const bool is_ptzctlDEBUGDEVELOP = ( pLocalDevice->getVer().find("ptzctlDEBUGDEVELOP")!=std::string::npos );
													if( is_ptzctlDEBUGDEVELOP )
													{
														const GSPTZ_CtrlCmd ctlcmd = (GSPTZ_CtrlCmd)pLocalButton->GetSignalSafe().original[0];
														int sleeptime = pLocalButton->GetSignalSafe().original[1];
														int speedlevel = pLocalButton->GetSignalSafe().original[2];

														if( sleeptime < 50 || sleeptime > 5000 )
														{
															sleeptime = 255;
														}

														if( speedlevel < 1 || speedlevel > 100 ) // 7?
														{
															speedlevel = 7;
														}

														if( ctlcmd<120 )
														{
															//ipcamClient->SendPTZ( testid, ctlcmd, 0, 0, speedlevel );
															usleep( sleeptime * 1000 );
															//ipcamClient->SendPTZ( testid, GSPTZ_STOPAll, 0 );
														}

														continue;
													}
												}
											}

											switch( localctl->GetExType() )
											{
											case IOTDevice_AC_Ctl:
											{
												GSIOTDevice *sendDev = pLocalDevice->clone( false );
												RFRemoteControl *sendctl = (RFRemoteControl*)sendDev->getControl();
												sendctl->ButtonQueueChangeToOne( pCurButton->GetId() );
												
												if( defCmd_Null == ctl->GetCmd() )
												{
													sendctl->SetCmd( defCmd_Default );
												}
												PushControlMesssageQueue( new ControlMessage( iq.from(), iq.id(), sendDev, sendctl->GetButton( pCurButton->GetId() ) ) );
												return true;
											}
											break;

											default:
												break;
											}

											GSIOTDevice *sendDev = pLocalDevice->clone( false );
											RFRemoteControl *sendctl = (RFRemoteControl*)sendDev->getControl();
											sendctl->ButtonQueueChangeToOne( pCurButton->GetId() );

											this->SendControl( device->getType(), sendDev, NULL );

											macCheckAndDel_Obj(sendctl);

											if( iotControl->getNeedRet() )
											{
												IQ re( IQ::Result, iq.from(), iq.id() );
												re.addExtension( new XmppGSResult( XMLNS_GSIOT_CONTROL, defGSReturn_SuccExecuted ) );
												XmppClientSend( re, "handleIq Send(Set RFRemoteControl executed ACK)" );
											}
										}
										else
										{
											if( iotControl->getNeedRet() )
											{
												IQ re( IQ::Result, iq.from(), iq.id() );
												re.addExtension( new XmppGSResult( XMLNS_GSIOT_CONTROL, defGSReturn_NoExist ) );
												XmppClientSend( re, "handleIq Send(Set RFRemoteControl NoExist ACK)" );
											}

											printf( "(%s)IQ::Set remote: button not found, devType=%d, devID=%d, btnid=%d", iq.from().bare().c_str(), device->getType(), device->getId(), pCurButton->GetId() );
										}
									}

									return true;
																	}
								break;
							}
						}

					}
					return true;
				}
			}
			break;

		case IQ::Result:
			{
				XmppGSMessage *pExXmppGSMessage = (XmppGSMessage*)iq.findExtension(ExtIotMessage);
				if( pExXmppGSMessage )
				{
					if( defGSReturn_Success == pExXmppGSMessage->get_state() )
					{
						EventNoticeMsg_Remove( pExXmppGSMessage->get_id() );
					}

					return true;
				}
			}
			break;
	}
	return true;
}

void GSIOTClient::handleIq_Get_XmppGSAuth_User( const XmppGSAuth_User *pExXmppGSAuth_User, const IQ& iq, const GSIOTUser *pUser )
{
	defmapGSIOTUser mapUserDest;
	const defmapGSIOTUser &mapUser = m_cfg->m_UserMgr.GetList_User();

	if( defCfgOprt_GetSelf == pExXmppGSAuth_User->GetMethod()
#if defined(defTest_defCfgOprt_GetSelf)
		|| true //--temptest
#endif
		)
	{

		const std::string selfJid = iq.from().bare();

		GSIOTUser *pUserRet = NULL;
		const GSIOTUser *pUserSelf = m_cfg->m_UserMgr.GetUser( selfJid );
		if( pUserSelf )
		{
			pUserRet = pUserSelf->clone();
		}
		else
		{
			const GSIOTUser *pUserGuest = m_cfg->m_UserMgr.GetUser( XMPP_GSIOTUser_Guest );
			if( pUserGuest )
			{

				pUserRet = pUserGuest->clone();
				pUserRet->SetName( "guest" );
			}
			else
			{
				pUserRet = new GSIOTUser();
				pUserRet->SetJid(selfJid);
				pUserRet->SetID(0);
				pUserRet->SetName( "(un add user)" );
				pUserRet->SetEnable(defDeviceDisable);
			}
		}

		if( pUserRet )
		{
			this->m_cfg->FixOwnerAuth(pUserRet);

			pUserRet->RemoveUnused();
			GSIOTUserMgr::usermapInsert( mapUserDest, pUserRet );
		}
		else
		{
			return;
		}
	}
	else
	{
		const defUserAuth curAuth = m_cfg->m_UserMgr.check_Auth( pUser, IOT_Module_authority, defAuth_ModuleDefaultID );
		if( !GSIOTUser::JudgeAuth( curAuth, defUserAuth_RO ) )
		{
			IQ re( IQ::Result, iq.from(), iq.id());
			re.addExtension( new XmppGSResult( XMLNS_GSIOT_AUTHORITY_USER, defGSReturn_NoAuth ) );
			XmppClientSend(re,"handleIq Send(Get ExtIotAuthority_User ACK)");
			return ;
		}

		const std::string keyjid_owner = this->m_cfg->GetOwnerKeyJid();

		const defmapGSIOTUser& needGetUser = pExXmppGSAuth_User->GetList_User();

		for( defmapGSIOTUser::const_iterator it=mapUser.begin(); it!=mapUser.end(); ++it )
		{
			const GSIOTUser *pUser = it->second;
			const std::string keyjid_user = pUser->GetKeyJid();

			if( needGetUser.find( keyjid_user ) != needGetUser.end() )
			{
				GSIOTUser *pUserRet = pUser->clone();

				this->m_cfg->FixOwnerAuth(pUserRet);

				if( defCfgOprt_GetSimple == pExXmppGSAuth_User->GetMethod()
					|| this->m_cfg->isOwnerForKeyJid(keyjid_owner,keyjid_user) )
				{
					pUserRet->RemoveUnused( true );
				}

				GSIOTUserMgr::usermapInsert( mapUserDest, pUserRet );
			}
		}
	}

	IQ re( IQ::Result, iq.from(), iq.id());
	re.addExtension( new XmppGSAuth_User(pExXmppGSAuth_User->GetSrcMethod(), mapUserDest, struTagParam(), true) );
	XmppClientSend(re,"handleIq Send(Get ExtIotAuthority_User ACK)");
}


void GSIOTClient::handleIq_Get_XmppGSAuth( const XmppGSAuth *pExXmppGSAuth, const IQ& iq, const GSIOTUser *pUser )
{
#if !defined(defTest_defCfgOprt_GetSelf)
	defUserAuth curAuth = m_cfg->m_UserMgr.check_Auth( pUser, IOT_Module_authority, defAuth_ModuleDefaultID );
	if( !GSIOTUser::JudgeAuth( curAuth, defUserAuth_RO ) )
	{
		IQ re( IQ::Result, iq.from(), iq.id());
		re.addExtension( new XmppGSResult( XMLNS_GSIOT_AUTHORITY, defGSReturn_NoAuth ) );
		XmppClientSend(re,"handleIq Send(Get ExtIotAuthority ACK)");
		return ;
	}
#endif

	const defmapGSIOTUser& needGetUser = pExXmppGSAuth->GetList_User();

	defmapGSIOTUser mapUserDest;
	const defmapGSIOTUser &mapUser = m_cfg->m_UserMgr.GetList_User();
	for( defmapGSIOTUser::const_iterator it=mapUser.begin(); it!=mapUser.end(); ++it )
	{
		const GSIOTUser *pUser = it->second;

		if( defUserAuth_RO == curAuth && !pUser->GetEnable() )
		{
			continue;
		}

		GSIOTUser *pUserRet = pUser->clone();
		pUserRet->ResetOnlyAuth(true,false);
		GSIOTUserMgr::usermapInsert( mapUserDest, pUserRet );
	}

	IQ re( IQ::Result, iq.from(), iq.id());
	re.addExtension( new XmppGSAuth(false, pExXmppGSAuth->GetSrcMethod(), mapUserDest, struTagParam(), true ) );
	XmppClientSend(re,"handleIq Send(Get ExtIotAuthority ACK)");
}


void GSIOTClient::handleIq_Set_XmppGSAuth( const GSMessage *pMsg )
{
	if( !pMsg )
		return;

	if( !pMsg->getpEx() )
		return;

	if( ExtIotAuthority != pMsg->getpEx()->extensionType() )
		return;

	const XmppGSAuth *pExXmppGSAuth = (const XmppGSAuth*)pMsg->getpEx();

	defmapGSIOTUser mapUserDest;
	GSIOTUserMgr::usermapCopy( mapUserDest, pExXmppGSAuth->GetList_User() );

	for( defmapGSIOTUser::const_iterator it=mapUserDest.begin(); it!=mapUserDest.end(); ++it )
	{
		GSIOTUser *pUser = it->second;
		if( pUser->isMe( pMsg->getFrom().bare() ) )
		{
			pUser->SetResult(defGSReturn_IsSelf);
		}
		else if( this->m_cfg->isOwner(pUser->GetJid()) )
		{
			pUser->SetResult(defGSReturn_ObjEditDisable);
		}
		else
		{
			defGSReturn ret = m_cfg->m_UserMgr.CfgChange_User( pUser, pExXmppGSAuth->GetMethod() );
			//pUser->SetValidAttribute( GSIOTUser::defAttr_all, false );
			pUser->SetResult(ret);
		}
	}

	// ack
	struTagParam TagParam;
	TagParam.isValid = true;
	TagParam.isResult = true;
	IQ re( IQ::Result, pMsg->getFrom(), pMsg->getId());
	re.addExtension( new XmppGSAuth(true, pExXmppGSAuth->GetSrcMethod(), mapUserDest, TagParam, true) );
	XmppClientSend(re,"handleIq Send(Set ExtIotAuthority ACK)");
}

void GSIOTClient::handleIq_Set_XmppGSManager( const GSMessage *pMsg )
{
	if( !pMsg )
		return;

	if( !pMsg->getpEx() )
		return;

	if( ExtIotManager != pMsg->getpEx()->extensionType() )
		return;

	const XmppGSManager *pExXmppGSManager = (const XmppGSManager*)pMsg->getpEx();

	GSIOTUser *pUser = m_cfg->m_UserMgr.check_GetUser( pMsg->getFrom().bare() );

	this->m_cfg->FixOwnerAuth(pUser);

	defGSReturn ret = m_cfg->m_UserMgr.check_User(pUser);
	if( macGSFailed(ret) )
	{
		printf( "(%s)IQ::Set: Not found userinfo. no auth.", pMsg->getFrom().bare().c_str() );

		IQ re( IQ::Result, pMsg->getFrom(), pMsg->getId());
		re.addExtension( new XmppGSResult( "all", defGSReturn_NoAuth ) );
		XmppClientSend(re,"handleIq Send(all Set ACK)");
		return ;
	}

	defCfgOprt_ method = pExXmppGSManager->GetMethod();

	const std::list<GSIOTDevice*> devices = pExXmppGSManager->GetDeviceList();
	for( std::list<GSIOTDevice*>::const_iterator it=devices.begin(); it!=devices.end(); ++it )
	{
		GSIOTDevice *pDeviceSrc = *it;

		if( defCfgOprt_Add != method )
		{
			defUserAuth curAuth = m_cfg->m_UserMgr.check_Auth( pUser, pDeviceSrc->getType(), pDeviceSrc->getId() );
			if( !GSIOTUser::JudgeAuth( curAuth, defUserAuth_WO ) )
			{
				IQ re( IQ::Result, pMsg->getFrom(), pMsg->getId());
				re.addExtension( new XmppGSResult( XMLNS_GSIOT_MANAGER, defGSReturn_NoAuth ) );
				XmppClientSend(re,"handleIq Send(Get ExtIotManager ACK)");
				return ;
			}
		}

		switch(method)
		{
		case defCfgOprt_Add:
			{
				add_GSIOTDevice( pDeviceSrc );
			}
			break;

		case defCfgOprt_AddModify:
			break;

		case defCfgOprt_Modify:
			{
				edit_GSIOTDevice( pDeviceSrc );
			}
			break;

		case defCfgOprt_Delete:
			{
				delete_GSIOTDevice( pDeviceSrc );
			}
			break;
		}
	}

	// ack
	struTagParam TagParam;
	TagParam.isValid = true;
	TagParam.isResult = true;
	IQ re( IQ::Result, pMsg->getFrom(), pMsg->getId());
	re.addExtension( new XmppGSManager(pExXmppGSManager->GetSrcMethod(), pExXmppGSManager->GetDeviceList(), TagParam) );
	XmppClientSend(re,"handleIq Send(Set ExtIotManager ACK)");
}

bool GSIOTClient::add_GSIOTDevice( GSIOTDevice *pDeviceSrc )
{
	if( !pDeviceSrc->getControl() )
	{
		pDeviceSrc->SetResult( defGSReturn_Err );
		printf( "add_GSIOTDevice: failed, no ctl, devType=%d, devID=%d", pDeviceSrc->getType(), pDeviceSrc->getId() );
		return false;
	}

	GSIOTDevice *pLocalDevice = this->GetIOTDevice( pDeviceSrc->getType(), pDeviceSrc->getId() );

	if( !pDeviceSrc->hasChild() )
	{
		if( pLocalDevice )
		{
			pDeviceSrc->SetResult( defGSReturn_IsExist );
			printf( "add_GSIOTDevice: failed, dev IsExist, devType=%d, devID=%d", pDeviceSrc->getType(), pDeviceSrc->getId() );
			return false;
		}
	}

	// edit attr all
	pDeviceSrc->doEditAttrFromAttrMgr_All();

	if( pDeviceSrc->getName().empty() )
	{
		pDeviceSrc->setName( "dev" );
	}

	if( pLocalDevice )
	{
		bool isSuccess = true;

		switch(pLocalDevice->getControl()->GetType())
		{
		case IOT_DEVICE_RS485:
			{
				RS485DevControl *ctl = (RS485DevControl*)pDeviceSrc->getControl();
				RS485DevControl *localctl = (RS485DevControl*)pLocalDevice->getControl();

				const defAddressQueue &AddrQue = ctl->GetAddressList();
				defAddressQueue::const_iterator itAddrQue = AddrQue.begin();
				for( ; itAddrQue!=AddrQue.end(); ++itAddrQue )
				{
					DeviceAddress *pSrcAddr = *itAddrQue;
					pSrcAddr->SetResult( defGSReturn_Null );

					DeviceAddress *pLocalAddr = localctl->GetAddress( pSrcAddr->GetAddress() );
					if( pLocalAddr )
					{
						pSrcAddr->SetResult( defGSReturn_IsExist );
						continue;
					}

					deviceClient->GetDeviceManager()->Add_DeviceAddress( pLocalDevice, pSrcAddr->clone() );
					pSrcAddr->SetResult( defGSReturn_Success );
				}
			}
			break;

		case IOT_DEVICE_Remote:
			{
				RFRemoteControl *ctl = (RFRemoteControl*)pDeviceSrc->getControl();
				RFRemoteControl *localctl = (RFRemoteControl*)pLocalDevice->getControl();

				const defButtonQueue &que = ctl->GetButtonList();
				defButtonQueue::const_iterator it = que.begin();
				defButtonQueue::const_iterator itEnd = que.end();
				for( ; it!=itEnd; ++it )
				{
					RemoteButton *pSrcButton = *it;
					pSrcButton->SetResult( defGSReturn_Null );

					RemoteButton *pLocalButton = localctl->GetButton( pSrcButton->GetId() );
					if( pLocalButton )
					{
						pSrcButton->SetResult( defGSReturn_IsExist );
						isSuccess = false;
						continue;
					}

					deviceClient->GetDeviceManager()->Add_remote_button( pLocalDevice, pSrcButton->clone() );
					pSrcButton->SetResult( defGSReturn_Success );
				}
			}
			break;
		}

		return isSuccess;
	}
	else
	{   //jyc20170331 resume
		if( IOT_DEVICE_Camera == pDeviceSrc->getType() ){
			CameraControl *ctl = (CameraControl*)pDeviceSrc->getControl();

			std::string outAttrValue;
			int module_type = -1;
			if( ctl->FindEditAttr( "module_type", outAttrValue ) )
			{
				module_type = atoi(outAttrValue.c_str());
			}

			IPCameraBase *cam = NULL;
			switch( module_type )
			{
				case CameraType_hik:
				//jyc20170331 trouble no HikCamera
				//cam = new HikCamera( "", "", "", 0, "", "", "1.0", GSPtzFlag_Null, GSFocalFlag_Null, 0, 0 );
				break;
			}

			cam->SetName( pDeviceSrc->getName() );
			cam->doEditAttrFromAttrMgr( *ctl );

			this->ipcamClient->AddIPCamera( cam, pDeviceSrc->GetEnable(), &pLocalDevice );
			pDeviceSrc->SetResult( defGSReturn_Success );	
		}
		else
		{
			this->deviceClient->AddController( pDeviceSrc->getControl()->clone(), c_DefaultVer, pDeviceSrc->GetEnable(), &pLocalDevice );
			pDeviceSrc->SetResult( defGSReturn_Success );
		}

		if( !pLocalDevice )
		{
			pDeviceSrc->SetResult( defGSReturn_Err );
			printf( "add_GSIOTDevice: add new failed, devType=%d, devID=%d", pDeviceSrc->getType(), pDeviceSrc->getId() );
			return false;
		}

		return true;
	}

	return false;
}

bool GSIOTClient::edit_GSIOTDevice( GSIOTDevice *pDeviceSrc )
{
	GSIOTDevice *pLocalDevice = this->GetIOTDevice( pDeviceSrc->getType(), pDeviceSrc->getId() );
	if( !pLocalDevice )
	{
		pDeviceSrc->SetResult( defGSReturn_NoExist );
		printf( "edit_GSIOTDevice: failed, dev not found, devType=%d, devID=%d", pDeviceSrc->getType(), pDeviceSrc->getId() );
		return false;
	}

	if( pDeviceSrc->GetEditAttrMap().empty() )
	{
		pDeviceSrc->SetResult( defGSReturn_Null );
	}
	else
	{
		if( pLocalDevice->doEditAttrFromAttrMgr( *pDeviceSrc ) )
		{
			if( IOT_DEVICE_Camera == pLocalDevice->getType() )
			{
				ipcamClient->ModifyDevice( pLocalDevice );  //jyc20170331 resume
			}
			else
			{
				deviceClient->ModifyDevice( pLocalDevice );
			}
			pDeviceSrc->SetResult( defGSReturn_Success );
		}
	}

	if( !pLocalDevice->getControl() )
	{
		printf( "edit_GSIOTDevice: dev ctl err, devType=%d, devID=%d", pDeviceSrc->getType(), pDeviceSrc->getId() );
		return false;
	}

	if( pDeviceSrc->getControl() )
	{
		switch( pDeviceSrc->getType() )
		{
		case IOT_DEVICE_Camera:
			{
				//return edit_CamDevControl( pLocalDevice, pDeviceSrc ); //jyc20170331 resume but have trouble
			}

		case IOT_DEVICE_RS485:
			{
				return edit_RS485DevControl( pLocalDevice, pDeviceSrc );
			}

		case IOT_DEVICE_Remote:
			{
				return edit_RFRemoteControl( pLocalDevice, pDeviceSrc );
			}

		default:
			{
				printf( "edit_GSIOTDevice unsupport type=%d", pDeviceSrc->getType() );
				return false;
			}
		}
	}

	return true;
}


bool GSIOTClient::edit_RS485DevControl( GSIOTDevice *pLocalDevice, GSIOTDevice *pDeviceSrc )
{
	RS485DevControl *ctl = (RS485DevControl*)pDeviceSrc->getControl();
	RS485DevControl *localctl = (RS485DevControl*)pLocalDevice->getControl();

	if( localctl->doEditAttrFromAttrMgr( *ctl ) )
	{
		deviceClient->ModifyDevice( pLocalDevice );
		ctl->SetResult( defGSReturn_Success );
	}

	//ergodic the list   jyc20170222 trans 
	const defAddressQueue &AddrQue = ctl->GetAddressList();
	defAddressQueue::const_iterator itAddrQue = AddrQue.begin();
	for( ; itAddrQue!=AddrQue.end(); ++itAddrQue )
	{
		DeviceAddress *pSrcAddr = *itAddrQue;
		pSrcAddr->SetResult( defGSReturn_Null );

		DeviceAddress *pLocalAddr = localctl->GetAddress( pSrcAddr->GetAddress() );
		if( !pLocalAddr )
		{
			pSrcAddr->SetResult( defGSReturn_NoExist );
			continue;
		}

		if( pLocalAddr->doEditAttrFromAttrMgr( *pSrcAddr ) )
		{
			deviceClient->ModifyAddress( pLocalDevice, pLocalAddr );
			pSrcAddr->SetResult( defGSReturn_Success );
		}
	}

	return true;
}

bool GSIOTClient::edit_RFRemoteControl( GSIOTDevice *pLocalDevice, GSIOTDevice *pDeviceSrc )
{
	RFRemoteControl *ctl = (RFRemoteControl*)pDeviceSrc->getControl();
	RFRemoteControl *localctl = (RFRemoteControl*)pLocalDevice->getControl();

	//ergodic the list   jyc20170222 trans
	const defButtonQueue &que = ctl->GetButtonList();
	defButtonQueue::const_iterator it = que.begin();
	defButtonQueue::const_iterator itEnd = que.end();
	for( ; it!=itEnd; ++it )
	{
		RemoteButton *pSrcButton = *it;
		pSrcButton->SetResult( defGSReturn_Null );

		RemoteButton *pLocalButton = localctl->GetButton( pSrcButton->GetId() );
		if( !pLocalButton )
		{
			pSrcButton->SetResult( defGSReturn_NoExist );
			continue;
		}

		if( pLocalButton->doEditAttrFromAttrMgr( *pSrcButton ) )
		{
			deviceClient->GetDeviceManager()->DB_Modify_remote_button( pLocalDevice->getType(), pLocalDevice->getId(), pLocalButton );
			pSrcButton->SetResult( defGSReturn_Success );
		}
	}

	return true;
}

bool GSIOTClient::delete_GSIOTDevice( GSIOTDevice *pDeviceSrc )
{
	GSIOTDevice *pLocalDevice = this->GetIOTDevice( pDeviceSrc->getType(), pDeviceSrc->getId() );

	if( !pLocalDevice )
	{
		pDeviceSrc->SetResult( defGSReturn_NoExist );
		printf( "delete_GSIOTDevice: failed, dev NoExist, devType=%d, devID=%d", pDeviceSrc->getType(), pDeviceSrc->getId() );
		return false;
	}

	if( pLocalDevice->m_ObjLocker.islock() )
	{
		pDeviceSrc->SetResult( defGSReturn_IsLock );
		printf( "delete_GSIOTDevice: failed, dev IsLock, devType=%d, devID=%d", pDeviceSrc->getType(), pDeviceSrc->getId() );
		return false;
	}

	//judge has child to deal
	if( !pDeviceSrc->hasChild() )
	{
		if( this->DeleteDevice( pLocalDevice ) )
		{
			pDeviceSrc->SetResult( defGSReturn_Success );
			return true;
		}

		pDeviceSrc->SetResult( defGSReturn_Err );
		return false;
	}

	bool isSuccess = true;

	switch(pLocalDevice->getControl()->GetType())
	{
	case IOT_DEVICE_RS485:
		{
			RS485DevControl *ctl = (RS485DevControl*)pDeviceSrc->getControl();
			RS485DevControl *localctl = (RS485DevControl*)pLocalDevice->getControl();

			const defAddressQueue &AddrQue = ctl->GetAddressList();
			defAddressQueue::const_iterator itAddrQue = AddrQue.begin();
			for( ; itAddrQue!=AddrQue.end(); ++itAddrQue )
			{
				DeviceAddress *pSrcAddr = *itAddrQue;
				pSrcAddr->SetResult( defGSReturn_Null );

				DeviceAddress *pLocalAddr = localctl->GetAddress( pSrcAddr->GetAddress() );
				if( !pLocalAddr )
				{
					pSrcAddr->SetResult( defGSReturn_NoExist );
					continue;
				}

				if( deviceClient->GetDeviceManager()->Delete_DeviceAddress( pLocalDevice, pLocalAddr->GetAddress() ) )
					pSrcAddr->SetResult( defGSReturn_Success );
				else
					pSrcAddr->SetResult( defGSReturn_Err );
			}
		}
		break;

	case IOT_DEVICE_Remote:
		{
			RFRemoteControl *ctl = (RFRemoteControl*)pDeviceSrc->getControl();
			RFRemoteControl *localctl = (RFRemoteControl*)pLocalDevice->getControl();

			const defButtonQueue &que = ctl->GetButtonList();
			defButtonQueue::const_iterator it = que.begin();
			defButtonQueue::const_iterator itEnd = que.end();
			for( ; it!=itEnd; ++it )
			{
				RemoteButton *pSrcButton = *it;
				pSrcButton->SetResult( defGSReturn_Null );

				RemoteButton *pLocalButton = localctl->GetButton( pSrcButton->GetId() );
				if( !pLocalButton )
				{
					pSrcButton->SetResult( defGSReturn_NoExist );
					isSuccess = false;
					continue;
				}

				if( deviceClient->GetDeviceManager()->Delete_remote_button( pLocalDevice, pLocalButton ) )
					pSrcButton->SetResult( defGSReturn_Success );
				else
					pSrcButton->SetResult( defGSReturn_Err );
			}
		}
		break;
	}

	return isSuccess;
}

void GSIOTClient::handleIq_Set_XmppGSEvent( const GSMessage *pMsg )
{
	if( !pMsg )
		return;

	if( !pMsg->getpEx() )
		return;

	if( ExtIotEvent != pMsg->getpEx()->extensionType() )
		return;

	const XmppGSEvent *pExXmppGSEvent = (const XmppGSEvent*)pMsg->getpEx();

	GSIOTUser *pUser = m_cfg->m_UserMgr.check_GetUser( pMsg->getFrom().bare() );

	this->m_cfg->FixOwnerAuth(pUser);

	defGSReturn ret = m_cfg->m_UserMgr.check_User(pUser);
	if( macGSFailed(ret) )
	{
		printf( "(%s)IQ::Set: Not found userinfo. no auth.", pMsg->getFrom().bare().c_str() );

		IQ re( IQ::Result, pMsg->getFrom(), pMsg->getId());
		re.addExtension( new XmppGSResult( "all", defGSReturn_NoAuth ) );
		XmppClientSend(re,"handleIq Send(all Set ACK)");
		return ;
	}

	defCfgOprt_ method = pExXmppGSEvent->GetMethod();
	
	bool needsort = false;

	const std::list<ControlEvent*> &Events = pExXmppGSEvent->GetEventList();
	for( std::list<ControlEvent*>::const_iterator it=Events.begin(); it!=Events.end(); ++it )
	{
		ControlEvent *pSrc = *it;

		defUserAuth curAuth = m_cfg->m_UserMgr.check_Auth( pUser, pSrc->GetDeviceType(), pSrc->GetDeviceID() );
		if( !GSIOTUser::JudgeAuth( curAuth, defUserAuth_WO ) )
		{
			IQ re( IQ::Result, pMsg->getFrom(), pMsg->getId());
			re.addExtension( new XmppGSResult( XMLNS_GSIOT_EVENT, defGSReturn_NoAuth ) );
			XmppClientSend(re,"handleIq Send(Get ExtIotManager ACK)");
			return ;
		}

		switch(method)
		{
		case defCfgOprt_Add:
			{
				add_ControlEvent( pSrc );
				needsort = true;
			}
			break;

		case defCfgOprt_AddModify:
			break;

		case defCfgOprt_Modify:
			{
				edit_ControlEvent( pSrc );
				needsort = true;
			}
			break;

		case defCfgOprt_Delete:
			{
				delete_ControlEvent( pSrc );
			}
			break;
		}
	}

	if( defCfgOprt_Modify == method )
	{
		std::string outAttrValue;
		if( pExXmppGSEvent->FindEditAttr( "state", outAttrValue ) )
		{
			if( pExXmppGSEvent->GetDevice() )
			{
				GSIOTDevice *pDevice = this->GetIOTDevice( pExXmppGSEvent->GetDevice()->getType(), pExXmppGSEvent->GetDevice()->getId() );
				const bool AGRunState = (bool)atoi(outAttrValue.c_str());
				if( pDevice && pDevice->getControl() )
				{
					defUserAuth curAuth = m_cfg->m_UserMgr.check_Auth( pUser, pDevice->getType(), pDevice->getId() );
					if( !GSIOTUser::JudgeAuth( curAuth, defUserAuth_WO ) )
					{
						IQ re( IQ::Result, pMsg->getFrom(), pMsg->getId());
						re.addExtension( new XmppGSResult( XMLNS_GSIOT_EVENT, defGSReturn_NoAuth ) );
						XmppClientSend(re,"handleIq Send(Get ExtIotManager ACK)");
						return ;
					}

					switch( pDevice->getType() )
					{
					case IOT_DEVICE_Trigger:
						{
							TriggerControl *ctl = (TriggerControl*)pDevice->getControl();
							ctl->SetAGRunState( AGRunState );
							this->deviceClient->ModifyDevice( pDevice );
						}
						break;

					case IOT_DEVICE_Camera:
						{//jyc20170331 resume
							IPCameraBase *ctl = (IPCameraBase*)pDevice->getControl();
							ctl->SetAGRunState( AGRunState );
							this->ipcamClient->ModifyDevice( pDevice );
						}
						break;

					default:
						break;
					}
				}
			}
		}
	}

	if( needsort )
	{
		m_event->SortEvents();
	}

	// ack
	struTagParam TagParam;
	TagParam.isValid = true;
	TagParam.isResult = true;
	IQ re( IQ::Result, pMsg->getFrom(), pMsg->getId());
	re.addExtension( new XmppGSEvent(pExXmppGSEvent->GetSrcMethod(), pExXmppGSEvent->GetDevice(), (std::list<ControlEvent*> &)Events, 1, TagParam, true) );
	XmppClientSend(re,"handleIq Send(Set ExtIotEvent ACK)");
}

bool GSIOTClient::add_ControlEvent( ControlEvent *pSrc )
{
	if( pSrc->GetType() > Unknown_Event )
	{
		ControlEvent *pSrc_clone = pSrc->clone();
		m_event->AddEvent( pSrc_clone );
		pSrc->SetID( pSrc_clone->GetID() );
		pSrc->SetResult( defGSReturn_Success );
		return true;
	}

	pSrc->SetResult( defGSReturn_Err );
	return false;
}

bool GSIOTClient::edit_ControlEvent( ControlEvent *pSrc )
{
	std::list<ControlEvent *> evtList = m_event->GetEvents();
	std::list<ControlEvent *>::const_iterator it = evtList.begin();
	for(;it!=evtList.end();it++)
	{
		if( (*it)->GetDeviceType() == pSrc->GetDeviceType()
			&& (*it)->GetDeviceID() == pSrc->GetDeviceID()
			&& (*it)->GetType() == pSrc->GetType()
			&& (*it)->GetID() == pSrc->GetID()
			)
		{
			bool doUpdate = false;

			switch(pSrc->GetType())
			{
			case SMS_Event:
			case EMAIL_Event:
			case NOTICE_Event:
				{
					doUpdate = (*it)->doEditAttrFromAttrMgr( *pSrc );
					break;
				}

			case CONTROL_Event:
				{
					doUpdate = (*it)->doEditAttrFromAttrMgr( *pSrc );

					AutoControlEvent *aevtLocal = (AutoControlEvent*)(*it);
					AutoControlEvent *aevtSrc = (AutoControlEvent*)pSrc;
					doUpdate |= aevtLocal->UpdateForOther( aevtSrc );
					break;
				}
				
			case Eventthing_Event:
				{
					doUpdate = (*it)->doEditAttrFromAttrMgr( *pSrc );

					AutoEventthing *aevtLocal = (AutoEventthing*)(*it);
					AutoEventthing *aevtSrc = (AutoEventthing*)pSrc;
					if( aevtSrc->IsAllDevice() )
					{
						doUpdate = true;
						aevtLocal->SetAllDevice();
					}
					else
					{
						if( aevtSrc->GetTempDevice() )
						{
							doUpdate |= aevtLocal->UpdateForDev( aevtSrc->GetTempDevice() );
						}
						else
						{
							pSrc->SetResult( defGSReturn_Err );
							return false;
						}
					}

					aevtLocal->SetRunState( aevtSrc->GetRunState() );

					break;
				}

			default:
				pSrc->SetResult( defGSReturn_Err );
				return false;
			}

			if( doUpdate )
			{
				if( m_event->ModifyEvent( pSrc, NULL ) )
					pSrc->SetResult( defGSReturn_Success );
				else
					pSrc->SetResult( defGSReturn_Err );
			}

			return true;
		}
	}

	pSrc->SetResult( defGSReturn_NoExist );
	return false;
}

bool GSIOTClient::delete_ControlEvent( ControlEvent *pSrc )
{
	std::list<ControlEvent *> evtList = m_event->GetEvents();
	std::list<ControlEvent *>::const_iterator it = evtList.begin();
	for(;it!=evtList.end();it++)
	{
		if( (*it)->GetDeviceType() == pSrc->GetDeviceType()
			&& (*it)->GetDeviceID() == pSrc->GetDeviceID()
			&& (*it)->GetType() == pSrc->GetType()
			&& (*it)->GetID() == pSrc->GetID()
			)
		{
			m_event->DeleteEvent( (*it) );
			pSrc->SetResult( defGSReturn_Success );
			return true;
		}
	}

	pSrc->SetResult( defGSReturn_NoExist );
	return true;
}
//jyc20170330 resume
void GSIOTClient::handleIq_Set_XmppGSTalk( const XmppGSTalk *pXmpp, const IQ& iq, const GSIOTUser *pUser )
{
	if( !pXmpp )
	{
		return;
	}

//	if( !IsRUNCODEEnable(defCodeIndex_SYS_Talk) )  //jyc20170527 remove
//	{
//		//LOGMSGEX( defLOGNAME, defLOG_INFO, "XmppGSTalk failed, SYS_Talk Disable!" );
//
//		IQ re( IQ::Result, iq.from(), iq.id());
//		re.addExtension( new XmppGSResult( XMLNS_GSIOT_TALK, defGSReturn_FunDisable) );
//		XmppClientSend(re,"handleIq Send(Get ExtIotTalk ACK)");
//		return;
//	}
	
	defUserAuth curAuth = m_cfg->m_UserMgr.check_Auth( pUser, IOT_Module_talk, defAuth_ModuleDefaultID );
	if( !GSIOTUser::JudgeAuth( curAuth, defUserAuth_RO ) )
	{
		IQ re( IQ::Result, iq.from(), iq.id());
		re.addExtension( new XmppGSResult( XMLNS_GSIOT_TALK, defGSReturn_NoAuth ) );
		XmppClientSend(re,"handleIq Send(Get ExtIotTalk ACK)");
		return ;
	}

	const defvecDevKey &specdev = pXmpp->get_vecdev();
	for( defvecDevKey::const_iterator it=specdev.begin(); it!=specdev.end(); ++it )
	{
		curAuth = m_cfg->m_UserMgr.check_Auth( pUser, it->m_type, it->m_id );
		if( !GSIOTUser::JudgeAuth( curAuth, defUserAuth_WO ) )
		{
			IQ re( IQ::Result, iq.from(), iq.id());
			re.addExtension( new XmppGSResult( XMLNS_GSIOT_TALK, defGSReturn_NoAuth ) );
			XmppClientSend(re,"handleIq Send(Get ExtIotTalk ACK)");
			return ;
		}
	}

	switch( pXmpp->get_cmd() )
	{
	case XmppGSTalk::defTalkCmd_request:
		{

			const bool isOnlyOneTalk_new = true; 

			unsigned long QueueCount = 0;
			unsigned long PlayCount = 0;
			bool isOnlyOneTalk_cur = false;
			this->m_TalkMgr.GetCountInfo( QueueCount, PlayCount, isOnlyOneTalk_cur );

			const bool isplaying = ( PlayCount>0 );

			bool success = false;
			if( isplaying )
			{
				if( isOnlyOneTalk_new || isOnlyOneTalk_cur )
				{
					success = false;
				}
				else
				{
					if( PlayCount >= MAX_TALK )
					{
						success = false;
					}
					else
					{
						success = true;
					}
				}
			}
			else
			{
				success = true;
			}

			IQ re( IQ::Result, iq.from(), iq.id());
			re.addExtension( new XmppGSTalk( struTagParam(), 
				success ? XmppGSTalk::defTalkCmd_accept:XmppGSTalk::defTalkCmd_reject, pXmpp->get_url(), pXmpp->get_vecdev() ) );
			if( success )
				XmppClientSend(re,"handleIq Send(Get ExtIotTalk ACK request success)");
			else
				XmppClientSend(re,"handleIq Send(Get ExtIotTalk ACK request failed)");
		}
		break;

	case XmppGSTalk::defTalkCmd_session:
		{
			defGSReturn ret = this->m_TalkMgr.StartTalk( pXmpp->get_url(), iq.from().full(), iq.id(), pXmpp->get_vecdev(), true, true );

			if( macGSFailed(ret) )
			{
				IQ re( IQ::Result, iq.from(), iq.id());
				re.addExtension( new XmppGSTalk( struTagParam(), 
					defGSReturn_IsLock==ret?XmppGSTalk::defTalkCmd_reject:XmppGSTalk::defTalkCmd_quit, pXmpp->get_url(), pXmpp->get_vecdev() ) );
				XmppClientSend(re,"handleIq Send(Set ExtIotTalk ACK StartTalk failed)");
			}
		}
		break;

	case XmppGSTalk::defTalkCmd_adddev:
		{
			defGSReturn ret = this->m_TalkMgr.AdddevTalk( pXmpp->get_url(), iq.from().full(), iq.id(), pXmpp->get_vecdev() );
			if( macGSFailed(ret) )
			{
				IQ re( IQ::Result, iq.from(), iq.id());
				re.addExtension( new XmppGSTalk( struTagParam(), 
					XmppGSTalk::defTalkCmd_adddev, pXmpp->get_url(), pXmpp->get_vecdev(), false ) );
				XmppClientSend(re,"handleIq Send(Set ExtIotTalk ACK AdddevTalk failed)");
			}
		}
		break;

	case XmppGSTalk::defTalkCmd_removedev:
		{
			defGSReturn ret = this->m_TalkMgr.RemovedevTalk( pXmpp->get_url(), iq.from().full(), iq.id(), pXmpp->get_vecdev() );
			if( macGSFailed(ret) )
			{
				IQ re( IQ::Result, iq.from(), iq.id());
				re.addExtension( new XmppGSTalk( struTagParam(), 
					XmppGSTalk::defTalkCmd_removedev, pXmpp->get_url(), pXmpp->get_vecdev(), false ) );
				XmppClientSend(re,"handleIq Send(Set ExtIotTalk ACK RemovedevTalk failed)");
			}
		}
		break;

	case XmppGSTalk::defTalkCmd_keepalive:	
		{
			bool isOnlyOneTalk = false;
			if( this->m_TalkMgr.isPlaying_url( pXmpp->get_url(), isOnlyOneTalk, true ) )
			{
#if 1
				if( pXmpp->get_vecdev().empty() )
				{
					IQ re( IQ::Result, iq.from(), iq.id());
					re.addExtension( new XmppGSTalk( struTagParam(), 
						XmppGSTalk::defTalkCmd_keepalive, pXmpp->get_url(), pXmpp->get_vecdev() ) );
					XmppClientSend(re,"handleIq Send(Set ExtIotTalk ACK keepalive is playing)");
				}
				else
#endif
				{
					this->m_TalkMgr.UrlKey_push_cmd( pXmpp->get_url(), struNewPlay_Param( XmppGSTalk::defTalkCmd_keepalive, pXmpp->get_url(), iq.from().full(), iq.id(), false, pXmpp->get_vecdev() ) );
				}
			}
			else
			{
				IQ re( IQ::Result, iq.from(), iq.id());
				re.addExtension( new XmppGSTalk( struTagParam(), 
					XmppGSTalk::defTalkCmd_quit, pXmpp->get_url(), pXmpp->get_vecdev() ) );
				XmppClientSend(re,"handleIq Send(Set ExtIotTalk ACK keepalive is quit)");
			}
		}
		break;

	case XmppGSTalk::defTalkCmd_quit:		
		{
			bool isOnlyOneTalk = false;
			if( !pXmpp->get_url().empty() 
				&& !this->m_TalkMgr.isPlaying_url(pXmpp->get_url(), isOnlyOneTalk) )
			{
				IQ re( IQ::Result, iq.from(), iq.id());
				re.addExtension( new XmppGSTalk( struTagParam(), 
					XmppGSTalk::defTalkCmd_quit, pXmpp->get_url(), pXmpp->get_vecdev() ) );
				XmppClientSend(re,"handleIq Send(Set ExtIotTalk ACK quitcmd is not playing)");
			}

			this->m_TalkMgr.StopTalk( pXmpp->get_url() );
		}
		break;
		
	case XmppGSTalk::defTalkCmd_forcequit:
		{
			this->m_TalkMgr.StopTalk_AnyOne();
		}
		break;

	default:
		{
			if( pXmpp->get_strSrcCmd().empty() )
				LOGMSG( "handleIq recv ExtIotTalk failed! cmd is null" );
			else
				LOGMSG( "handleIq recv ExtIotTalk failed! unsupport cmd=\"%s\"", pXmpp->get_strSrcCmd().c_str() );
		}
		break;
	}
}

//jyc20170405 resume 
void GSIOTClient::handleIq_Set_XmppGSPlayback( const XmppGSPlayback *pXmpp, const IQ& iq, const GSIOTUser *pUser )
{
	if( !pXmpp )
	{
		return;
	}

	defUserAuth curAuth = m_cfg->m_UserMgr.check_Auth( pUser, IOT_Module_record, defAuth_ModuleDefaultID );
	if( !GSIOTUser::JudgeAuth( curAuth, defUserAuth_RO ) )
	{
		IQ re( IQ::Result, iq.from(), iq.id());
		re.addExtension( new XmppGSResult( XMLNS_GSIOT_PLAYBACK, defGSReturn_NoAuth ) );
		XmppClientSend(re,"handleIq Send(Get ExtIotPlayback ACK)");
		return ;
	}
 
	PlaybackCmd_push( pXmpp, iq ); 
}

void GSIOTClient::PlaybackCmd_ProcOneCmd( const struPlaybackCmd *pCmd )
{
	const XmppGSPlayback *pXmpp = pCmd->pXmpp;
	const JID &from_Jid = pCmd->from_Jid;
	const std::string &from_id = pCmd->from_id;

	int speedlevel = 0;

	// check time
	switch( pXmpp->get_state() )
	{
	case XmppGSPlayback::defPBState_StopAll:
		break;

	default:
		{
			if( timeGetTime()-pCmd->timestamp > 20000 )
			{
				LOGMSG( "handleIq recv ExtIotPlayback overtime 10s! jid=%s.", from_Jid.full().c_str() );
				return;
			}
		}
		break;
	}

	// proc
	switch( pXmpp->get_state() )
	{
	case XmppGSPlayback::defPBState_Start:
		{
			if( pXmpp->get_url().empty() && pXmpp->get_url_backup().empty() )
			{
				LOGMSG( "handleIq recv ExtIotPlayback failed! url is null." );
				return;
			}

			if( Playback_IsLimit() )
			{
				IQ re( IQ::Result, from_Jid, from_id );
				re.addExtension( new XmppGSResult( XMLNS_GSIOT_PLAYBACK, defGSReturn_ResLimit ) );
				XmppClientSend(re,"handleIq Send(Get ExtIotPlayback ACK err! Playback Channel Limit.)");
				return;
			}

			Playback_DeleteForJid( from_Jid.full() );

			struGSTime dtBegin;
			struGSTime dtEnd;
			if( 0==pXmpp->get_startdt() 
				|| 0==pXmpp->get_enddt() 
				|| pXmpp->get_enddt() < pXmpp->get_startdt()
				|| !g_UTCTime_To_struGSTime( pXmpp->get_startdt()-10, dtBegin )
				|| !g_UTCTime_To_struGSTime( pXmpp->get_enddt(), dtEnd ) )
			{
				IQ re( IQ::Result, from_Jid, from_id );
				re.addExtension( new XmppGSResult( XMLNS_GSIOT_PLAYBACK, defGSReturn_NoExist ) );
				XmppClientSend(re,"handleIq Send(Get ExtIotPlayback ACK err! param err.)");
				return;
			}

			LOGMSG( "PlaybackCmd_ProcOneCmd camid=%d, begin=%d-%d-%d %d:%d:%d+10?, end=%d-%d-%d %d:%d:%d",
				pXmpp->get_camera_id(),
				dtBegin.Year, dtBegin.Month, dtBegin.Day, dtBegin.Hour, dtBegin.Minute, dtBegin.Second,
				dtEnd.Year, dtEnd.Month, dtEnd.Day, dtEnd.Hour, dtEnd.Minute, dtEnd.Second );

			const GSIOTDevice *pDev = ipcamClient->GetIOTDevice( pXmpp->get_camera_id() );
			if( !pDev )
			{
				IQ re( IQ::Result, from_Jid, from_id );
				re.addExtension( new XmppGSResult( XMLNS_GSIOT_PLAYBACK, defGSReturn_NoExist ) );
				XmppClientSend(re,"handleIq Send(Get ExtIotPlayback ACK err! dev not found.)");
				return;
			}

			if( pDev->getControl() )
			{
				if( defRecMod_NoRec == ((IPCameraBase*)pDev->getControl())->GetRecCfg().getrec_mod() )
				{
					IQ re( IQ::Result, from_Jid, from_id );
					re.addExtension( new XmppGSResult( XMLNS_GSIOT_PLAYBACK, defGSReturn_FunDisable ) );
					XmppClientSend(re,"handleIq Send(Get ExtIotPlayback ACK err! dev NoRec.)");
					return;
				}
			}

			if( !IsRUNCODEEnable(defCodeIndex_TEST_DeCheckFilePlayback) )
			{
				const defGSReturn retFind = ((IPCameraBase*)pDev->getControl())->QuickSearchPlayback( &dtBegin, &dtEnd );
				if( macGSFailed(retFind) 
					&& defGSReturn_Null != retFind
					&& defGSReturn_TimeOut != retFind
					)
				{
					IQ re( IQ::Result, from_Jid, from_id );
					re.addExtension( new XmppGSResult( XMLNS_GSIOT_PLAYBACK, retFind ) );
					XmppClientSend( re, "handleIq Send(Get ExtIotPlayback ACK err! QuickSearchPlayback)" );
					return;
				}
			}

			GSIOTDevice *PlayCam = GSIOTClient::ClonePlaybackDev( pDev );
			if( !PlayCam )
			{
				IQ re( IQ::Result, from_Jid, from_id );
				re.addExtension( new XmppGSResult( XMLNS_GSIOT_PLAYBACK, defGSReturn_Err ) );
				XmppClientSend(re,"handleIq Send(Get ExtIotPlayback ACK err! dev new err.)");
				return;
			}

			IPCameraBase *camctl = ((IPCameraBase*)PlayCam->getControl());
			if( !camctl )
			{
				IQ re( IQ::Result, from_Jid, from_id );
				re.addExtension( new XmppGSResult( XMLNS_GSIOT_PLAYBACK, defGSReturn_Err ) );
				XmppClientSend(re,"handleIq Send(Get ExtIotPlayback ACK err! dev ctl new err.)");

				Playback_DeleteDevOne( PlayCam );
				return;
			}
			
			const int sound = pXmpp->get_sound();
			if( sound>=0 )
			{
				LOGMSG( "Playback_Start sound=%d, from=\"%s\"\r\n", sound, from_Jid.full().c_str() );
				camctl->GetStreamObj()->GetRTMPSendObj()->set_playback_sound( sound );
			}
			
			camctl->GetStreamObj()->GetRTMPSendObj()->set_wait_frist_key_frame();
			camctl->GetStreamObj()->OnPublishStart();

			const defGSReturn ret = camctl->Connect( false, NULL, true, &dtBegin, &dtEnd );
			if( macGSFailed(ret) )
			{
				IQ re( IQ::Result, from_Jid, from_id );
				re.addExtension( new XmppGSResult( XMLNS_GSIOT_PLAYBACK, ret ) );
				XmppClientSend(re,"handleIq Send(Get ExtIotPlayback ACK err! dev Connect err.)");

				Playback_DeleteDevOne( PlayCam );
				return;
			}

			std::string useUrl = pXmpp->get_url();

			const bool New_isRTMFP = g_IsRTMFP_url( useUrl );

			bool doConnect_RTMP = true; 
			bool doConnect_RTMFP = false; 

			if( RUNCODE_Get( defCodeIndex_SYS_Enable_RTMFP, defRunCodeValIndex_2 ) )
			{
				if( New_isRTMFP )
				{
					doConnect_RTMFP = true;
					doConnect_RTMP = false;
				}
			}

			if( doConnect_RTMFP )
			{
				camctl->GetStreamObj()->GetRTMPSendObj()->setPeerID( "" );
				camctl->GetStreamObj()->GetRTMPSendObj()->pushRTMPHandle( NULL, useUrl );

				if( IsRUNCODEEnable( defCodeIndex_RTMFP_UrlAddStreamID ) )
				{
					useUrl += "/";
					useUrl += camctl->GetStreamObj()->GetRTMPSendObj()->getStreamID();
				}
			}
			
			if( !camctl->GetStreamObj()->SendToRTMPServer( useUrl, true ) )
			{
				IQ re( IQ::Result, from_Jid, from_id );
				re.addExtension( new XmppGSResult( XMLNS_GSIOT_PLAYBACK, defGSReturn_Err ) );
				XmppClientSend( re, "handleIq Send(Get ExtIotPlayback ACK err! SendToRTMPServer err.)" );

				Playback_DeleteDevOne( PlayCam );
				return;
			}

			if( doConnect_RTMFP )
			{
				const int waittime = RUNCODE_Get( defCodeIndex_RTMFP_WaitConnTimeout );
				const DWORD dwStart = ::timeGetTime();
				while( camctl->GetStreamObj()->GetRTMPSendObj()->getPeerID().empty() && ::timeGetTime()-dwStart < waittime )
				{
					usleep( 100000 );
				}

				if( camctl->GetStreamObj()->GetRTMPSendObj()->getPeerID().empty() )
				{
					LOGMSG( "CameraBase(%s)::SendToRTMPServer wait rtmfp connect timeout! %dms\r\n", camctl->GetName().c_str(), ::timeGetTime()-dwStart );

					doConnect_RTMP = true; // 
				}
				else
				{
					camctl->GetStreamObj()->GetRTMPSendObj()->Connect( useUrl );
					LOGMSG( "CameraBase(%s)::SendToRTMPServer wait rtmfp connect success. %dms\r\n", camctl->GetName().c_str(), ::timeGetTime()-dwStart );
				}
			}

			if( doConnect_RTMP )
			{
				std::vector<std::string> vecurl;
				if( !New_isRTMFP ) vecurl.push_back( useUrl );
				const std::vector<std::string> &url_backup = pXmpp->get_url_backup();
				for( uint32_t i=0; i<url_backup.size(); ++i )
				{
					vecurl.push_back( url_backup[i] );
				}

				if( IsRUNCODEEnable( defCodeIndex_TEST_StreamServer ) )
				{
					const int val2 = RUNCODE_Get( defCodeIndex_TEST_StreamServer, defRunCodeValIndex_2 );
					const int val3 = RUNCODE_Get( defCodeIndex_TEST_StreamServer, defRunCodeValIndex_3 );

					char ssvr[32] ={0};
					snprintf( ssvr, sizeof( ssvr ), "%d.%d.%d.%d", val2/1000, val2%1000, val3/1000, val3%1000 );

					for( uint32_t iurlbak=0; iurlbak<vecurl.size(); ++iurlbak )
					{
						g_replace_all_distinct( vecurl[iurlbak], "www.gsss.cn", ssvr );

#ifdef _DEBUG
						LOGMSG( "pburlbak=%s", vecurl[iurlbak].c_str() );
#endif
					}
				}

				char cntbuf[64] ={0};
				if( IsRUNCODEEnable( defCodeIndex_SYS_UseUrlDifCnt ) )
				{
					static uint32_t s_url_difcnt = timeGetTime();

					for( uint32_t iurlbak=0; iurlbak<vecurl.size(); ++iurlbak )
					{
						vecurl[iurlbak] += std::string( "GS" );
						//vecurl[iurlbak] += itoa( timeGetTime() + s_url_difcnt++, cntbuf, 16 );
						sprintf(cntbuf, "%d", timeGetTime() + s_url_difcnt++);
						vecurl[iurlbak] += cntbuf;
					}
				}

				defRTMPConnectHandle RTMPhandle = RTMPSend::CreateRTMPInstance( vecurl, useUrl, pDev->getName().c_str() );
				if( !RTMPhandle )
				{
					LOGMSG( "playback(%s)::CreateRTMPInstance Connect rtmp failed\r\n", pDev->getName().c_str() );

					IQ re( IQ::Result, from_Jid, from_id );
					re.addExtension( new XmppGSResult( XMLNS_GSIOT_PLAYBACK, defGSReturn_ConnectSvrErr ) );
					XmppClientSend(re,"handleIq Send(Get ExtIotPlayback ACK err! ConnectSvrErr.)");

					Playback_DeleteDevOne( PlayCam );
					return;
				}

				camctl->GetStreamObj()->GetRTMPSendObj()->pushRTMPHandle( RTMPhandle, useUrl );
				camctl->GetStreamObj()->GetRTMPSendObj()->Connect( useUrl );
			}


			camctl->setStatus( "playing" );

			const std::string key = useUrl;

			if( !Playback_Add( from_Jid.full(), key, useUrl, camctl->GetStreamObj()->GetRTMPSendObj()->getPeerID(), camctl->GetStreamObj()->GetRTMPSendObj()->getStreamID(), PlayCam ) )
			{
				IQ re( IQ::Result, from_Jid, from_id );
				re.addExtension( new XmppGSResult( XMLNS_GSIOT_PLAYBACK, defGSReturn_Err ) );
				XmppClientSend(re,"handleIq Send(Get ExtIotPlayback ACK err! Playback_Add err.)");

				Playback_DeleteDevOne( PlayCam );
				return;
			}


			IQ re( IQ::Result, from_Jid, from_id);
			re.addExtension( new XmppGSPlayback( struTagParam(), 
				pXmpp->get_camera_id(), useUrl, camctl->GetStreamObj()->GetRTMPSendObj()->getPeerID(), camctl->GetStreamObj()->GetRTMPSendObj()->getStreamID(), key, XmppGSPlayback::defPBState_Start // key use url
				) );
			XmppClientSend(re,"handleIq Send(Get ExtIotPlayback ACK)");

			return ;
		}
		break;

	case XmppGSPlayback::defPBState_Stop:
		{
			if( pXmpp->get_key().empty() )
			{
				LOGMSG( "handleIq recv ExtIotPlayback failed! key is null." );
				return;
			}

			Playback_DeleteForJid( from_Jid.full() );

			Playback_CtrlResult( from_Jid, from_id, pXmpp, GSPlayBackCode_Stop );
			return ;
		}
		break;

	case XmppGSPlayback::defPBState_Set:
		{
			if( pXmpp->get_key().empty() )
			{
				LOGMSG( "handleIq recv ExtIotPlayback failed! key is null." );
				return;
			}

			const int sound = pXmpp->get_sound();
			if( sound>=0 )
			{
				Playback_SetForJid( from_Jid.full(), sound );
			}

			return ;
		}
		break;

	case XmppGSPlayback::defPBState_Get:
		{
			if( pXmpp->get_key().empty() )
			{
				LOGMSG( "handleIq recv ExtIotPlayback failed! key is null." );
				return;
			}

			const GSPlayBackCode_ ControlCode = Playback_CtrlForJid( from_Jid.full(), GSPlayBackCode_GetState, 0, 0, &speedlevel );

			Playback_CtrlResult( from_Jid, from_id, pXmpp, ControlCode );
			return ;
		}
		break;

	case XmppGSPlayback::defPBState_Pause:
		{
			if( pXmpp->get_key().empty() )
			{
				LOGMSG( "handleIq recv ExtIotPlayback failed! key is null." );
				return;
			}
			
			const GSPlayBackCode_ ControlCode = Playback_CtrlForJid( from_Jid.full(), GSPlayBackCode_PLAYPAUSE, 0, 0, &speedlevel );

			Playback_CtrlResult( from_Jid, from_id, pXmpp, ControlCode );
			return ;
		}
		break;

	case XmppGSPlayback::defPBState_Resume:
		{
			if( pXmpp->get_key().empty() )
			{
				LOGMSG( "handleIq recv ExtIotPlayback failed! key is null." );
				return;
			}

			const GSPlayBackCode_ ControlCode = Playback_CtrlForJid( from_Jid.full(), GSPlayBackCode_PLAYRESTART, 0, 0, &speedlevel );

			Playback_CtrlResult( from_Jid, from_id, pXmpp, ControlCode );
			return ;
		}
		break;

	case XmppGSPlayback::defPBState_NormalPlay:
		{
			if( pXmpp->get_key().empty() )
			{
				LOGMSG( "handleIq recv ExtIotPlayback failed! key is null." );
				return;
			}

			const GSPlayBackCode_ ControlCode = Playback_CtrlForJid( from_Jid.full(), GSPlayBackCode_PLAYNORMAL, 0, 0, &speedlevel );

			Playback_CtrlResult( from_Jid, from_id, pXmpp, ControlCode );
			return ;
		}
		break;

	case XmppGSPlayback::defPBState_FastPlay:
	case XmppGSPlayback::defPBState_FastPlayThrow:
	case XmppGSPlayback::defPBState_FastPlay1:
	case XmppGSPlayback::defPBState_FastPlay2:
		{
			if( pXmpp->get_key().empty() )
			{
				LOGMSG( "handleIq recv ExtIotPlayback failed! key is null." );
				return;
			}

			const int ThrowFrame = XmppGSPlayback::defPBState_FastPlayThrow == pXmpp->get_state() ? 1:0;

			if( XmppGSPlayback::defPBState_FastPlay1 == pXmpp->get_state() || XmppGSPlayback::defPBState_FastPlay2 == pXmpp->get_state() )
			{
				Playback_CtrlForJid( from_Jid.full(), GSPlayBackCode_PLAYNORMAL );
			}

			if( XmppGSPlayback::defPBState_FastPlay2 == pXmpp->get_state() )
			{
				Playback_CtrlForJid( from_Jid.full(), GSPlayBackCode_PLAYFAST, (void*)ThrowFrame );
			}

			const GSPlayBackCode_ ControlCode = Playback_CtrlForJid( from_Jid.full(), GSPlayBackCode_PLAYFAST, (void*)ThrowFrame, 0, &speedlevel );

			Playback_CtrlResult( from_Jid, from_id, pXmpp, ControlCode );
			return ;
		}
		break;
		
	case XmppGSPlayback::defPBState_SlowPlay:
	case XmppGSPlayback::defPBState_SlowPlay1:
	case XmppGSPlayback::defPBState_SlowPlay2:
		{
			if( pXmpp->get_key().empty() )
			{
				LOGMSG( "handleIq recv ExtIotPlayback failed! key is null." );
				return;
			}

			if( XmppGSPlayback::defPBState_SlowPlay1 == pXmpp->get_state() || XmppGSPlayback::defPBState_SlowPlay2 == pXmpp->get_state() )
			{
				Playback_CtrlForJid( from_Jid.full(), GSPlayBackCode_PLAYNORMAL );
			}

			if( XmppGSPlayback::defPBState_SlowPlay2 == pXmpp->get_state() )
			{
				Playback_CtrlForJid( from_Jid.full(), GSPlayBackCode_PLAYSLOW );
			}

			const GSPlayBackCode_ ControlCode = Playback_CtrlForJid( from_Jid.full(), GSPlayBackCode_PLAYSLOW, 0, 0, &speedlevel );

			Playback_CtrlResult( from_Jid, from_id, pXmpp, ControlCode );
			return ;
		}
		break;

	case XmppGSPlayback::defPBState_StopAll:
		{
			Playback_DeleteAll();

			return ;
		}
		break;

	default: // heartbeat
		{
			if( pXmpp->get_key().empty() )
			{
				LOGMSG( "handleIq recv ExtIotPlayback failed! key is null." );
				return;
			}
			
			Playback_UpdateSession( pXmpp->get_key() );

			return ;
		}
		break;
	}
}

void GSIOTClient::PlayMgrCmd_push( defPlayMgrCmd_ cmd, defUserAuth Auth, const IQ& iq, const int dev_id, const std::string &url, const std::vector<std::string> &url_backup )
{
	gloox::util::MutexGuard mutexguard( m_mutex_PlayMgr );

	if( m_lstPlayMgrCmd.size()>999 )
	{
		LOGMSG( "PlayMgrCmd list limit 999! throw jid=%s", iq.from().full().c_str() );
		return ;
	}

	m_lstPlayMgrCmd.push_back( new struPlayMgrCmd( cmd, Auth, iq.from(), iq.id(), dev_id, url, url_backup ) );
}

void GSIOTClient::PlayMgrCmd_SetCheckNow( bool CheckNow )
{
	m_PlayMgr_CheckNowFlag = CheckNow ? defPlayMgrCmd_CheckNow : defPlayMgrCmd_Unknown;
}

bool GSIOTClient::PlayMgrCmd_IsCheckNow()
{
	return ( defPlayMgrCmd_CheckNow == m_PlayMgr_CheckNowFlag );
}

void GSIOTClient::PlayMgrCmd_SetDevtimeNow( IOTDeviceType type, int id )
{
	std::set<int> NeedIDList; 
	//std::list<GSIOTDevice*> &cameraList = this->ipcamClient->GetCameraManager()->GetCameraList();
	const std::list<GSIOTDevice*> &cameraList = this->ipcamClient->GetCameraManager()->GetCameraList();
	std::list<GSIOTDevice*>::const_iterator it = cameraList.begin();
	for( ; it!=cameraList.end(); ++it )
	{
		if( (*it)->GetEnable() )
		{
			if( IOT_DEVICE_All == type )
			{
				NeedIDList.insert( (*it)->getId() );
			}
			else if( (*it)->getType()==type || (*it)->getId()==id )
			{
				NeedIDList.insert( (*it)->getId() );
				break;
			}
		}
	}

	PlayMgrCmd_SetDevtimeNowForList( NeedIDList );
}

void GSIOTClient::PlayMgrCmd_SetDevtimeNowForList( const std::set<int> &NeedIDList )
{
	gloox::util::MutexGuard mutexguard( m_mutex_PlayMgr );

	for( std::set<int>::const_iterator it = NeedIDList.begin(); it!=NeedIDList.end(); ++it )
	{
		m_check_all_devtime_NeedIDList.insert( *it );
	}
}

struPlayMgrCmd* GSIOTClient::PlayMgrCmd_pop()
{
	gloox::util::MutexGuard mutexguard( m_mutex_PlayMgr );

	if( m_lstPlayMgrCmd.empty() )
	{
		return NULL;
	}

	struPlayMgrCmd *pCmd = m_lstPlayMgrCmd.front();
	m_lstPlayMgrCmd.pop_front();
	return pCmd;
}

void GSIOTClient::PlayMgrCmd_clean()
{
	gloox::util::MutexGuard mutexguard( m_mutex_PlayMgr );

	if( m_lstPlayMgrCmd.empty() )
	{
		return;
	}

	for( std::list<struPlayMgrCmd*>::iterator it = m_lstPlayMgrCmd.begin(); it!=m_lstPlayMgrCmd.end(); ++it )
	{
		delete( *it );
	}

	m_lstPlayMgrCmd.clear();
}

// return false : no work
bool GSIOTClient::PlayMgrCmd_OnProc()
{
	struPlayMgrCmd *pCmd = PlayMgrCmd_pop();

	if( !pCmd )
	{
		return false;
	}

	PlayMgrCmd_ProcOneCmd( pCmd );
	delete( pCmd );
	return true;
}

void GSIOTClient::PlayMgrCmd_ProcOneCmd( const struPlayMgrCmd *pCmd )
{
	switch(pCmd->cmd)
	{
	case defPlayMgrCmd_Start:
		{
			defGSReturn ret = ipcamClient->PushToRTMPServer( pCmd->from_Jid, pCmd->dev_id, pCmd->url, pCmd->url_backup );

			if( macGSFailed(ret) )
			{
				IQ re( IQ::Result, pCmd->from_Jid, pCmd->from_id );
				re.addExtension( new XmppGSResult( XMLNS_GSIOT_CONTROL, ret, defNormResultMod_control_camplay ) );
				XmppClientSend(re,"handleIq Send(PlayMgrCmd_ProcOneCmd Start failed ACK)");
			}
			else
			{
				IQ re( IQ::Result, pCmd->from_Jid, pCmd->from_id );
				re.addExtension( new GSIOTControl( this->GetIOTDevice(IOT_DEVICE_Camera,pCmd->dev_id), pCmd->Auth ) );
				XmppClientSend(re,"handleIq Send(PlayMgrCmd_ProcOneCmd Start ACK)");
			}
		}
		break;

	case defPlayMgrCmd_Stop:
		{
			ipcamClient->StopRTMPSend( pCmd->from_Jid, pCmd->dev_id );

			IQ re( IQ::Result, pCmd->from_Jid, pCmd->from_id );
			re.addExtension( new GSIOTControl( this->GetIOTDevice(IOT_DEVICE_Camera,pCmd->dev_id), pCmd->Auth ) );
			XmppClientSend(re,"handleIq Send(PlayMgrCmd_ProcOneCmd Stop ACK)");
		}
		break;

	default:
		{
			//LOGMSGEX( defLOGNAME, defLOG_ERROR, "PlayMgrCmd_ProcOneCmd unknown cmd=%d", pCmd->cmd );
			return;
		}
	}
}

void GSIOTClient::PlayMgrCmd_ThreadCreate()
{
	m_isPlayMgrThreadExit = false;

	pthread_t id_1;  
    int ret=pthread_create(&id_1, NULL, PlayMgrProc_Thread, this );
    if(ret!=0)  
    {  
        printf("Create PlayMgrProcThread error!\n");   
		return; 
    } 
}

void GSIOTClient::handleIq_Set_XmppGSRelation( const GSMessage *pMsg )
{
	if( !pMsg )
		return;

	if( !pMsg->getpEx() )
		return;

	if( ExtIotRelation != pMsg->getpEx()->extensionType() )
		return;

	const XmppGSRelation *pExXmppGSRelation = (const XmppGSRelation*)pMsg->getpEx();

	bool success = m_cfg->SetRelation( pExXmppGSRelation->get_device_type(), pExXmppGSRelation->get_device_id(), pExXmppGSRelation->get_ChildList() );

	// ack
	struTagParam TagParam;
	TagParam.isValid = true;
	TagParam.isResult = true;
	IQ re( IQ::Result, pMsg->getFrom(), pMsg->getId());
	re.addExtension( new XmppGSRelation(TagParam, 
	                                    pExXmppGSRelation->get_device_type(), 
	                                    pExXmppGSRelation->get_device_id(), 
	                                    deflstRelationChild(), 
	                                    success?defGSReturn_Success:defGSReturn_Err ) );
	XmppClientSend(re,"handleIq Send(Set ExtIotRelation ACK)");
}

void GSIOTClient::handleIq_Get_XmppGSRelation( const XmppGSRelation *pXmpp, const IQ& iq, const GSIOTUser *pUser )
{
	defUserAuth curAuth = m_cfg->m_UserMgr.check_Auth( pUser, pXmpp->get_device_type(), pXmpp->get_device_id() );

	deflstRelationChild ChildList;
	if( GSIOTUser::JudgeAuth( curAuth, defUserAuth_RO ) )
	{
		deflstRelationChild tempChildList;
		m_cfg->GetRelation( pXmpp->get_device_type(), pXmpp->get_device_id(), tempChildList );

		for( deflstRelationChild::const_iterator it=tempChildList.begin(); it!=tempChildList.end(); ++it )
		{
			defUserAuth curAuthChild = m_cfg->m_UserMgr.check_Auth( pUser, it->child_dev_type, it->child_dev_id );
			if( GSIOTUser::JudgeAuth( curAuthChild, defUserAuth_RO ) )
			{
				ChildList.push_back( *it );
			}
		}
	}

	IQ re( IQ::Result, iq.from(), iq.id() );
	re.addExtension( new XmppGSRelation(struTagParam(), pXmpp->get_device_type(), pXmpp->get_device_id(), ChildList ) );
	XmppClientSend(re,"handleIq Send(Get ExtIotRelation ACK)");
}

//jyc20170405 resume
void GSIOTClient::handleIq_Set_XmppGSPreset( const GSMessage *pMsg )
{
	if( !pMsg )
		return;

	if( !pMsg->getpEx() )
		return;

	if( ExtIotPreset != pMsg->getpEx()->extensionType() )
		return;

	XmppGSPreset *pXmpp = (XmppGSPreset*)pMsg->getpEx();

	if( !pXmpp )
		return;

	defGSReturn result = defGSReturn_Success;
	defPresetQueue PresetList;

	if( IOT_DEVICE_Camera == pXmpp->get_device_type() )
	{
		const CPresetObj *pPresetIn = pXmpp->GetFristPreset();
		if( pPresetIn )
		{
			GSIOTDevice *device = this->GetIOTDevice( pXmpp->get_device_type(), pXmpp->get_device_id() );
			IPCameraBase *ctl = device?(IPCameraBase*)device->getControl():NULL;

			if( device && ctl )
			{
				if( ctl->GetAdvAttr().get_AdvAttr( defCamAdvAttr_PTZ_Preset ) )
				{
					switch( pXmpp->GetMethod() )
					{
					case XmppGSPreset::defPSMethod_goto:
						{

						}
						break;

					case XmppGSPreset::defPSMethod_add:
						{
							int newindex = ctl->GetUnusedIndex();
							if( newindex <= 0 )
							{
								result = defGSReturn_ResLimit;
								break;
							}
							
							result = ctl->CheckExist( 0, pPresetIn->GetObjName(), NULL );
							if( macGSFailed(result) )
							{
								break;
							}

							CPresetObj *pPresetNew = new CPresetObj( pPresetIn->GetObjName() );
							pPresetNew->SetIndex( newindex );
							
							if( this->deviceClient->GetDeviceManager()->Add_Preset( device, pPresetNew ) )
							{
								PresetList.push_back( pPresetNew->clone() );
							}
							else
							{
								result = defGSReturn_Err;
								delete pPresetNew;
							}
						}
						break;

					case XmppGSPreset::defPSMethod_del:
						{
							pXmpp->swap_PresetList( PresetList );

							for( defPresetQueue::const_iterator it=PresetList.begin(); it!=PresetList.end(); ++it )
							{
								CPresetObj *pPreset = *it;

								if( !this->deviceClient->GetDeviceManager()->Delete_Preset( device, pPreset->GetId() ) )
								{
									result = defGSReturn_Err;	
								}
							}
						}
						break;

					case XmppGSPreset::defPSMethod_edit:
						{
							pXmpp->swap_PresetList( PresetList );

							for( defPresetQueue::const_iterator it=PresetList.begin(); it!=PresetList.end(); ++it )
							{
								CPresetObj *pPreset = *it;

								result = ctl->CheckExist( pPreset->GetId(), pPreset->GetObjName(), NULL );
								if( macGSFailed(result) )
								{
									break;
								}

								CPresetObj *pPresetLocal = ctl->GetPreset(pPreset->GetId());
								if( pPresetLocal )
								{
									pPresetLocal->SetName( pPreset->GetObjName() );
									if( !this->deviceClient->GetDeviceManager()->DB_Modify_Preset( pPresetLocal, device->getType(), device->getId() ) )
									{
										result = defGSReturn_Err;
										break;
									}
								}
								else
								{
									result = defGSReturn_NoExist;
									break;
								}
							}
						}
						break;

					case XmppGSPreset::defPSMethod_setnew:
						{
							pXmpp->swap_PresetList( PresetList );

							CPresetObj *pPresetLocal = ctl->GetPreset(pPresetIn->GetId());
							if( pPresetLocal )
							{
								if( !ctl->SendPTZ( GSPTZ_SetNew_Preset, pPresetLocal->GetIndex() ) )
								{
									result = defGSReturn_Err;
								}
							}
							else
							{
								result = defGSReturn_NoExist;
							}
						}
						break;

					case XmppGSPreset::defPSMethod_sort:
						{
							pXmpp->swap_PresetList( PresetList );

							ctl->SortByPresetList( PresetList );

							if( !this->deviceClient->GetDeviceManager()->SaveSort_Preset( device ) )
							{
								result = defGSReturn_Err;
							}
						}
						break;

					default:
						result = defGSReturn_UnSupport;
						break;
					}
				}
				else
				{
					result = defGSReturn_FunDisable;
				}
			}
			else
			{
				result = defGSReturn_NoExist;
			}
		}
		else
		{
			result = defGSReturn_ErrParam;
		}
	}
	else
	{
		result = defGSReturn_UnSupport;
	}

	// ack
	IQ re( IQ::Result, pMsg->getFrom(), pMsg->getId());
	re.addExtension( new XmppGSPreset(struTagParam(true,true), pXmpp->GetSrcMethod(), pXmpp->get_device_type(), pXmpp->get_device_id(), PresetList, result ) );
	XmppClientSend(re,"handleIq Send(Set ExtIotPreset ACK)");
}

void GSIOTClient::handleIq_Get_XmppGSPreset( const XmppGSPreset *pXmpp, const IQ& iq, const GSIOTUser *pUser )
{
	defGSReturn result = defGSReturn_Null;

	defPresetQueue PresetList;
	if( IOT_DEVICE_Camera == pXmpp->get_device_type() )
	{
		defUserAuth curAuth = m_cfg->m_UserMgr.check_Auth( pUser, pXmpp->get_device_type(), pXmpp->get_device_id() );

		if( GSIOTUser::JudgeAuth( curAuth, defUserAuth_RO ) )
		{
			const GSIOTDevice *device = this->GetIOTDevice( pXmpp->get_device_type(), pXmpp->get_device_id() );
			if( device )
			{
				IPCameraBase *ctl = (IPCameraBase*)device->getControl();

				if( ctl->GetAdvAttr().get_AdvAttr( defCamAdvAttr_PTZ_Preset ) )
				{
					CPresetManager::ClonePresetQueue_Spec( PresetList, ctl->GetPresetList() );
				}
				else
				{
					result = defGSReturn_FunDisable;
				}
			}
			else
			{
				result = defGSReturn_NoExist;
			}
		}
		else
		{
			result = defGSReturn_NoAuth;
		}
	}
	else
	{
		result = defGSReturn_UnSupport;
	}

	IQ re( IQ::Result, iq.from(), iq.id() );
	re.addExtension( new XmppGSPreset(struTagParam(true,true), pXmpp->GetSrcMethod(), pXmpp->get_device_type(), pXmpp->get_device_id(), PresetList, result ) );
	XmppClientSend(re,"handleIq Send(Get ExtIotPreset ACK)");
}


void GSIOTClient::handleIq_Set_XmppGSVObj( const GSMessage *pMsg )
{
	if( !pMsg )
		return;

	if( !pMsg->getpEx() )
		return;

	if( ExtIotVObj != pMsg->getpEx()->extensionType() )
		return;

	XmppGSVObj *pXmpp = (XmppGSVObj*)pMsg->getpEx();

	if( !pXmpp )
		return;

	defGSReturn result = pXmpp->GetResult();
	defmapVObjConfig VObjCfgList = pXmpp->get_VObjCfgList();

	for( defmapVObjConfig::iterator it=VObjCfgList.begin(); it!=VObjCfgList.end(); ++it )
	{
		switch( pXmpp->GetMethod() )
		{
		case defCfgOprt_Add:
			{
				result = m_cfg->VObj_Add( it->second, NULL );
			}
			break;

		case defCfgOprt_Modify:
			{
				result = m_cfg->VObj_Modify( it->second, NULL );
			}
			break;

		case defCfgOprt_Delete:
			{
				result = m_cfg->VObj_Delete( it->second.vobj_type, it->second.id );
			}
			break;
		}
	}

	// ack
	IQ re( IQ::Result, pMsg->getFrom(), pMsg->getId());
	re.addExtension( new XmppGSVObj(struTagParam(true,true), pXmpp->GetSrcMethod(), VObjCfgList, result) );
	XmppClientSend(re,"handleIq Send(Set ExtIotVObj ACK)");
}

void GSIOTClient::handleIq_Get_XmppGSVObj( const XmppGSVObj *pXmpp, const IQ& iq, const GSIOTUser *pUser )
{
	const defmapVObjConfig &VObjCfgListAll = m_cfg->VObj_GetList();
	defmapVObjConfig VObjCfgListDest;

	for( defmapVObjConfig::const_iterator it=VObjCfgListAll.begin(); it!=VObjCfgListAll.end(); ++it )
	{
		if( !pXmpp->isAllType() )
		{
			if( !pXmpp->isInGetType( it->second.vobj_type ) )
			{
				continue;
			}
		}

		//const defUserAuth curAuth = m_cfg->m_UserMgr.check_Auth( pUser, it->second.vobj_type, it->second.id );
		//if( GSIOTUser::JudgeAuth( curAuth, defUserAuth_RO ) )
		{
			VObjCfgListDest[it->first] = it->second;
		}
	}

	IQ re( IQ::Result, iq.from(), iq.id() );
	re.addExtension( new XmppGSVObj(struTagParam(true,true), pXmpp->GetSrcMethod(), VObjCfgListDest, defGSReturn_Success ) );
	XmppClientSend(re,"handleIq Send(Get ExtIotVObj ACK)");
}

void GSIOTClient::handleIq_Get_XmppGSReport( const XmppGSReport *pXmpp, const IQ& iq, const GSIOTUser *pUser )
{
	XmppGSReport *pRetXmpp = new XmppGSReport(struTagParam(true,true));
	pRetXmpp->CopyParam( *pXmpp );
	pRetXmpp->m_ResultStat.AddrObjKey = pXmpp->m_AddrObjKey;
	pRetXmpp->m_ResultStat.data_dt_begin = g_struGSTime_To_UTCTime( pXmpp->m_dtBegin );
	pRetXmpp->m_ResultStat.data_dt_end = g_struGSTime_To_UTCTime( pXmpp->m_dtEnd );

	defUserAuth curAuth = m_cfg->m_UserMgr.check_Auth( pUser, pXmpp->m_AddrObjKey.dev_type, pXmpp->m_AddrObjKey.dev_id );

	if( GSIOTUser::JudgeAuth( curAuth, defUserAuth_RO ) )
	{
		switch(pXmpp->m_method)
		{
		case XmppGSReport::defRPMethod_minute:
			{
				pRetXmpp->m_result = GetDataStoreMgr()->QuerySrcValueLst_ForTimeRange_QueryStat( pRetXmpp->m_ResultStat, pRetXmpp->m_spanrate, pRetXmpp->m_ratefortype );
			}
			break;

		case XmppGSReport::defRPMethod_hour:
			{
				pRetXmpp->m_result = pRetXmpp->m_getstatminute ?
					GetDataStoreMgr()->QueryStatMinute_ForTime( pRetXmpp->m_ResultStat, pRetXmpp->m_lst_stat, pRetXmpp->m_Interval )
					:
					GetDataStoreMgr()->QueryStatData_ForTime_ForSpanmin( pRetXmpp->m_ResultStat.data_dt_begin, pRetXmpp->m_ResultStat.data_dt_end, pRetXmpp->m_ResultStat, pRetXmpp->m_Interval, pRetXmpp->m_spanrate, pRetXmpp->m_ratefortype );
			}
			break;

		case XmppGSReport::defRPMethod_day:
			{
				pRetXmpp->m_result =
					pRetXmpp->m_getstathour ?
					GetDataStoreMgr()->QueryStatData_ForTime
					(
					pRetXmpp->m_ResultStat.data_dt_begin, pRetXmpp->m_ResultStat.data_dt_end, pRetXmpp->m_ResultStat, pRetXmpp->m_mapRec_stat_day,
					pRetXmpp->m_getstathour, pRetXmpp->m_getdatalist, pRetXmpp->m_Interval, pRetXmpp->m_spanrate, pRetXmpp->m_ratefortype, true
					)
					:
					GetDataStoreMgr()->QueryStatDayRec_ForDayRange
					(
					pRetXmpp->m_ResultStat.data_dt_begin, pRetXmpp->m_ResultStat.data_dt_end, pRetXmpp->m_ResultStat, pRetXmpp->m_mapRec_stat_day,
					true, true, pRetXmpp->m_getdatalist, pRetXmpp->m_Interval, pRetXmpp->m_spanrate, pRetXmpp->m_ratefortype
					);
			}
			break;

		case XmppGSReport::defRPMethod_month:
			{
				pRetXmpp->m_result = GetDataStoreMgr()->QueryStatMonthRec_ForMonthRange
					(
					pRetXmpp->m_ResultStat.data_dt_begin, pRetXmpp->m_ResultStat.data_dt_end, pRetXmpp->m_ResultStat, pRetXmpp->m_mapRec_stat_month, pRetXmpp->m_mapRec_stat_day,
					pRetXmpp->m_getstatday, true, true
					);
			}
			break;
		}
	}
	else
	{
		// 
		pRetXmpp->m_result = defGSReturn_NoAuth;
	}

	if( !pRetXmpp->m_ResultStat.Stat.stat_valid || g_isNoDBRec( pRetXmpp->m_result ) )
	{
		pRetXmpp->m_result = defGSReturn_DBNoRec;
	}

	IQ re( IQ::Result, iq.from(), iq.id() );
	re.addExtension( pRetXmpp );
	XmppClientSend(re,"handleIq Send(Get ExtIotReport ACK)");
}

void GSIOTClient::handleIq_Set_XmppGSUpdate( const XmppGSUpdate *pXmpp, const IQ& iq, const GSIOTUser *pUser )
{
	if( !pXmpp )
	{
		return;
	}

	defUserAuth curAuth = m_cfg->m_UserMgr.check_Auth( pUser, IOT_Module_system, defAuth_ModuleDefaultID );
	if( !GSIOTUser::JudgeAuth( curAuth, defUserAuth_WO ) )
	{
		IQ re( IQ::Result, iq.from(), iq.id());
		re.addExtension( new XmppGSResult( XMLNS_GSIOT_UPDATE, defGSReturn_NoAuth ) );
		XmppClientSend(re,"handleIq Send(ExtIotUpdate ACK)");
		return ;
	}

	switch( pXmpp->get_state() )
	{
	case XmppGSUpdate::defUPState_check:
		{
			this->Update_Check_fromremote( iq.from(), iq.id() );
		}
		break;

	case XmppGSUpdate::defUPState_update:
	case XmppGSUpdate::defUPState_forceupdate:
		{
			std::string runparam;
			if( XmppGSUpdate::defUPState_forceupdate == pXmpp->get_state() )
			{
				runparam = "-forceupdate";
			}
			else
			{
				runparam =  "-update";
			}
			Update_DoUpdateNow_fromremote( iq.from(), iq.id(), runparam ); //jyc20170318 debug
		}
		break;

	default:
		LOGMSG( "un support, state=%d", pXmpp->get_state() );
		break;
	}

}


void GSIOTClient::handleSubscription( const Subscription& subscription )
{
	if(subscription.subtype() == Subscription::Subscribe){
		xmppClient->rosterManager()->ackSubscriptionRequest(subscription.from(),true);
	}
}

void GSIOTClient::OnTimer( int TimerID ) //heartbeat
{
	if( !m_running )
		return ;

	if( 2 == TimerID )
	{
		EventNoticeMsg_Check(); //jyc20160923
		return ;
	}

	if( 3 == TimerID )
	{
		//Playback_CheckSession();
		//Playback_ThreadCheck();
		return ;
	}
	
	if( 4 == TimerID )
	{
		xmppClient->whitespacePing();
		return ;
	}

	if( 5 == TimerID )
	{
		//this->CheckSystem(); //jyc20160923
		//this->CheckIOTPs();
		return ;
	}

	if( 1 != TimerID )
		return ;

	char strState_xmpp[256] = {0};
	gloox::ConnectionState state = xmppClient->state();
	switch( state )
	{
	case StateDisconnected:
		snprintf( strState_xmpp, sizeof(strState_xmpp), "xmpp curstate(%d) Disconnected", state );
		this->m_xmppReconnect = true;
		break;

	case StateConnecting:
		snprintf( strState_xmpp, sizeof(strState_xmpp), "xmpp curstate(%d) Connecting", state );
		break;

	case StateConnected:
		snprintf( strState_xmpp, sizeof(strState_xmpp), "xmpp curstate(%d) Connected", state );
		break;

	default:
		snprintf( strState_xmpp, sizeof(strState_xmpp), "xmpp curstate(%d)", state );
		break;
	}
	
	printf( "Heartbeat: %s\r\n", strState_xmpp );

	timeCount++;
	if(timeCount>10){

		printf( "GSIOT Version %s (build %s)\r\n", g_IOTGetVersion().c_str(), g_IOTGetBuildInfo().c_str() );

		//5min connect to server 
		xmppClient->whitespacePing();
		if(serverPingCount==0){
			printf( "xmppClient serverPingCount=0\r\n" );
			//this->m_xmppReconnect = true;
		}
		serverPingCount = 0;
	    timeCount = 0;
	}

	deviceClient->Check();

	CheckOverTimeControlMesssageQueue();

}

std::string GSIOTClient::GetConnectStateStr() const
{
	if( !xmppClient )
	{
		return std::string("δע�����");
	}

	switch( xmppClient->state() )
	{
	case StateDisconnected:
		return std::string("�����ж�");

	case StateConnecting:
		return std::string("������");

	case StateConnected:
		return std::string("��");

	default:
		break;
	}

	return std::string("");
}

void GSIOTClient::Run() //jyc20160826
{
	printf( "GSIOTClient::Run()\r\n" );

	LoadConfig();
	RTMPSend::Init( m_cfg->getSerialNumber() );

	//init devices
	deviceClient = new DeviceConnection(this);
	deviceClient->Run(m_cfg->getSerialPort());

	//init cameras  //jyc20170510 resume
	//ipcamClient = new IPCamConnection(this,this);
	ipcamClient = new IPCamConnection(this); //jyc20170511 modify
	ipcamClient->Connect(); //jyc20170519 debug

	defDBSavePresetQueue PresetQueue;
	deviceClient->GetDeviceManager()->LoadDB_Preset( PresetQueue );
}

void GSIOTClient::Connect()
{
	printf( "GSIOTClient::Connect()\r\n" );
	
	std::string strmac = m_cfg->getSerialNumber();
	std::string strjid = m_cfg->getSerialNumber()+"@"+XMPP_SERVER_DOMAIN;
	
	if(!CheckRegistered()){
		m_cfg->setJid(strjid);
		m_cfg->setPassword(getRandomCode());
		XmppRegister *reg = new XmppRegister(m_cfg->getSerialNumber(),m_cfg->getPassword());
		reg->start();
		bool state = reg->getState();
		delete(reg);
		if(!state){	
			printf( "GSIOTClient::Connect XmppRegister failed!!!" );
		    return;
		}
		m_cfg->SaveToFile();
		SetJidToServer( strjid, strmac ); //jyc20170319 resume
	}
	
	/*push stream timer*/
	timer = new TimerManager();
	timer->registerTimer(this,1,30);
	timer->registerTimer(this,2,2);		// ֪ͨ���
	timer->registerTimer(this,3,15);	// �طż��
	timer->registerTimer(this,4,60);	// �����ping
	timer->registerTimer(this,5,300);	// check system
	
	JID jid(m_cfg->getJid());
	jid.setResource("gsiot");
	xmppClient = new Client(jid,m_cfg->getPassword());
	
	//register iot protocol
	xmppClient->disco()->addFeature(XMLNS_GSIOT);
	xmppClient->disco()->addFeature(XMLNS_GSIOT_CONTROL);
	xmppClient->disco()->addFeature(XMLNS_GSIOT_DEVICE);
	xmppClient->disco()->addFeature(XMLNS_GSIOT_AUTHORITY);
	xmppClient->disco()->addFeature(XMLNS_GSIOT_AUTHORITY_USER);
	xmppClient->disco()->addFeature(XMLNS_GSIOT_MANAGER);
	xmppClient->disco()->addFeature(XMLNS_GSIOT_EVENT);
	xmppClient->disco()->addFeature(XMLNS_GSIOT_STATE);
	xmppClient->disco()->addFeature(XMLNS_GSIOT_Change);
	//jyc20170405 resume
//	if( IsRUNCODEEnable(defCodeIndex_SYS_Talk) )
//	{
//		//LOGMSGEX( defLOGNAME, defLOG_SYS, "SYS_Talk=true" );
//		xmppClient->disco()->addFeature(XMLNS_GSIOT_TALK);
//	}
//	else
//	{
//		//LOGMSGEX( defLOGNAME, defLOG_SYS, "SYS_Talk=false" );
//	}
	xmppClient->disco()->addFeature(XMLNS_GSIOT_TALK); //jyc20170526 modify

	xmppClient->disco()->addFeature(XMLNS_GSIOT_PLAYBACK);
	xmppClient->disco()->addFeature(XMLNS_GSIOT_RELATION);
	xmppClient->disco()->addFeature(XMLNS_GSIOT_Preset);
	xmppClient->disco()->addFeature(XMLNS_GSIOT_VObj);
	xmppClient->disco()->addFeature(XMLNS_GSIOT_Report);
	xmppClient->disco()->addFeature(XMLNS_GSIOT_MESSAGE);
	xmppClient->disco()->addFeature(XMLNS_GSIOT_UPDATE);
	xmppClient->registerStanzaExtension(new GSIOTInfo());
	xmppClient->registerStanzaExtension(new XmppGSResult(NULL)); 
	xmppClient->registerStanzaExtension(new GSIOTControl());
	xmppClient->registerStanzaExtension(new GSIOTDeviceInfo());
	xmppClient->registerStanzaExtension(new GSIOTHeartbeat());
	xmppClient->registerStanzaExtension(new XmppGSAuth(NULL));
	xmppClient->registerStanzaExtension(new XmppGSAuth_User(NULL));
	xmppClient->registerStanzaExtension(new XmppGSManager(NULL));
	xmppClient->registerStanzaExtension(new XmppGSEvent(NULL)); 
	xmppClient->registerStanzaExtension(new XmppGSState(NULL));
	xmppClient->registerStanzaExtension(new XmppGSChange(NULL));
	xmppClient->registerStanzaExtension(new XmppGSTalk(NULL));
	xmppClient->registerStanzaExtension(new XmppGSPlayback(NULL));
	xmppClient->registerStanzaExtension(new XmppGSRelation(NULL));
	xmppClient->registerStanzaExtension(new XmppGSPreset(NULL)); //jyc20170405 resume
	xmppClient->registerStanzaExtension(new XmppGSVObj(NULL));
	xmppClient->registerStanzaExtension(new XmppGSReport(NULL));
	xmppClient->registerStanzaExtension(new XmppGSMessage(NULL));
	xmppClient->registerStanzaExtension(new XmppGSUpdate(NULL)); //jyc20170319 resume
	xmppClient->registerIqHandler(this, ExtIot);
	xmppClient->registerIqHandler(this, ExtIotControl);
	xmppClient->registerIqHandler(this, ExtIotDeviceInfo);
	xmppClient->registerIqHandler(this, ExtIotHeartbeat);
	xmppClient->registerIqHandler(this, ExtIotAuthority);
	xmppClient->registerIqHandler(this, ExtIotAuthority_User);
	xmppClient->registerIqHandler(this, ExtIotManager);
	xmppClient->registerIqHandler(this, ExtIotEvent);
	xmppClient->registerIqHandler(this, ExtIotState); //dev red or green
	xmppClient->registerIqHandler(this, ExtIotChange);	
	xmppClient->registerIqHandler(this, ExtIotTalk);
	xmppClient->registerIqHandler(this, ExtIotPlayback);
	xmppClient->registerIqHandler(this, ExtIotRelation);
	xmppClient->registerIqHandler(this, ExtIotPreset);
	xmppClient->registerIqHandler(this, ExtIotVObj);
	xmppClient->registerIqHandler(this, ExtIotReport);
	xmppClient->registerIqHandler(this, ExtIotMessage);
	xmppClient->registerIqHandler(this, ExtIotUpdate);

	xmppClient->registerConnectionListener( this );
	//register direct access
	xmppClient->registerSubscriptionHandler(this);
	//help message
	xmppClient->registerMessageHandler(this);
	//server heartbeat
	xmppClient->registerIqHandler(this,ExtPing);
	
	m_running = true;
	m_isThreadExit = false;

	if( IsRUNCODEEnable(defCodeIndex_Dis_ChangeSaveDB) )
	{
		g_Changed( defCfgOprt_Modify, IOT_Obj_SYS, 0, 0 );
	}
	
	PlayMgrCmd_ThreadCreate(); //jyc20170405 resume //jyc20170519 debug
	Playback_ThreadCreate(); //jyc20170405 resume
	DataProc_ThreadCreate(); 
	//AlarmProc_ThreadCreate(); //jyc20170405 resume  but cpu=%110
	//ACProc_ThreadCreate(); //jyc20170405 no resume
	
	unsigned long reconnect_tick = timeGetTime();
	printf( "GSIOTClient Running...\r\n\r\n" );

	CHeartbeatGuard hbGuard( "XmppClient" );

	m_last_checkNetUseable_time = timeGetTime() - defcheckNetUseable_timeMax + (6*1000);
		
	while(m_running){
		hbGuard.alive();
#if 1
		ConnectionError ce = ConnNoError;
		if( xmppClient->connect( false ) )
		{
			m_xmppReconnect = false;
			while( ce == ConnNoError && m_running )
			{
				hbGuard.alive();
				if( m_xmppReconnect )
				{
					printf( "m_xmppReconnect is true, disconnect\n" );
					xmppClient->disconnect();
					break;
				}

				ce = xmppClient->recv(1000);
				
				//this->Update_UpdatedProc(); //jyc20160922 notice update

			}
			printf( "xmppClient->recv() return %d, m_xmppReconnect=%s\n", ce, m_xmppReconnect?"true":"false" );
		}
#else
	    xmppClient->connect(); // ����ʽ����
#endif

		uint32_t waittime= RUNCODE_Get(defCodeIndex_xmpp_ConnectInterval);

		if( waittime<6000 )
			waittime=6000;

		
		const unsigned long prev_span = timeGetTime()-reconnect_tick;
		if( prev_span > waittime*5 )
		{
			waittime=500;
		}
		reconnect_tick = timeGetTime();

		printf( ">>>>> xmppClient->connect() return. waittime=%d, prev_span=%lu\r\n", waittime, prev_span );

		DWORD dwStart = ::timeGetTime();
		while( m_running && ::timeGetTime()-dwStart < waittime )
		{
			usleep(1000);
		}
	}
	m_isThreadExit = true;
}

bool GSIOTClient::CheckRegistered()
{
	if(m_cfg->getJid().empty() || m_cfg->getPassword().empty()){
		return false;
	}
	return true;
}

void GSIOTClient::LoadConfig()
{
	if(m_cfg->getSerialNumber()==""){
		std::string macaddress;
		if(getMacAddress(macaddress)==0){
			m_cfg->setSerialNumber(macaddress);
		}
	}
}

bool GSIOTClient::SetJidToServer( const std::string &strjid, const std::string &strmac )
{
	char chreq_setjid[256] = {0};
	snprintf( chreq_setjid, sizeof(chreq_setjid), "api.gsss.cn/gsiot.ashx?method=SetJID&jid=%s&mac=%s", strjid.c_str(), strmac.c_str() );
	//jyc20170319 resume
	httpreq::Request req_setjid;
	std::string psHeaderSend;
	std::string psHeaderReceive;
	std::string psMessage;
	if( req_setjid.SendRequest( false, chreq_setjid, psHeaderSend, psHeaderReceive, psMessage ) )
	{
		printf( "SetJID to server send success. HeaderReceive=\"%s\", Message=\"%s\"", UTF8ToASCII(psHeaderReceive).c_str(), UTF8ToASCII(psMessage).c_str() );
		return true;
	}
	
	printf( "SetJID to server send failed." );
	return false;
}

GSIOTDevice* GSIOTClient::CloneCamDev( const GSIOTDevice *src, IPCameraType destType )
{
	if( !src )
		return NULL;

	GSIOTDevice *dev = new GSIOTDevice( *src );

	if( !src->getControl() )
		return dev;

	if( IOT_DEVICE_Camera != src->getType() )
		return dev;

	IPCameraBase *ctl = (IPCameraBase*)src->getControl();
	
	IPCameraType copyType = ctl->GetCameraType();
	if( CameraType_Unkown != destType )
	{
		copyType = destType;
	}

	IPCameraBase *cam = NULL;
	switch( copyType )
	{
	case CameraType_hik:
		//cam = new HikCamera( ctl->GetDeviceId(), ctl->GetName(), ctl->GetIPAddress(), ctl->GetPort(), ctl->GetUsername(), ctl->GetPassword(), ctl->getVer(), ctl->getPTZFlag(), ctl->getFocalFlag(), ctl->GetChannel(), ctl->GetStreamfmt() );
		break;
	}
	cam->UpdateReccfg( ctl->GetRecCfg() );
	cam->UpdateAudioCfg( ctl->GetAudioCfg() );
	cam->UpdateAdvAttr( ctl->GetAdvAttr() );
	dev->setControl( cam );

	return dev;
}

GSIOTDevice* GSIOTClient::ClonePlaybackDev( const GSIOTDevice *src )
{
	if( !src )
	{
		return NULL;
	}

	if( !src->getControl() )
	{
		return NULL;
	}

	IPCameraType destType = CameraType_Unkown;
	if( src->getControl() )
	{
		if( defRecMod_NoRec == ((IPCameraBase*)src->getControl())->GetRecCfg().getrec_mod() )
		{
			return NULL;
		}

		if( defRecMod_OnReordSvr == ((IPCameraBase*)src->getControl())->GetRecCfg().getrec_mod() )
		{
			const IPCameraType curType = ((IPCameraBase*)src->getControl())->GetRecCfg().getrec_svrtype();
			if( CameraType_Unkown == curType
				|| 
				( CameraType_Unkown != curType && CameraType_hik==curType )
				)
			{
				destType = CameraType_hik;
			}
		}
	}

	return GSIOTClient::CloneCamDev( src, destType );
}

ControlBase* GSIOTClient::CloneControl( const ControlBase *src, bool CreateLock )
{
	if( !src )
		return NULL;

	switch( src->GetType() )
	{
	case IOT_DEVICE_RS485:
		{
			return ((RS485DevControl*)src)->clone();
		}
	case IOT_DEVICE_Remote:
		{
			return ((RFRemoteControl*)src)->clone();
		}
	}

	printf( "GSIOTClient::CloneControl Error!!! type=%d\r\n", src->GetType() );
	return NULL;
}

DeviceAddress* GSIOTClient::CloneDeviceAddress( const DeviceAddress *src )
{
	if( src )
	{
		return (DeviceAddress*)(src->clone());
	}

	return NULL;
}

bool GSIOTClient::Compare_Device( const GSIOTDevice *devA, const GSIOTDevice *devB )
{
	if( devA == devB )
		return true;

	if( devA && devB )
	{
		//if( devA->GetLinkID() == devB->GetLinkID() && devA->getId() == devB->getId() && devA->getType() == devB->getType() )
		if( devA->getId() == devB->getId() && devA->getType() == devB->getType() )
		{
			return true;
		}
	}

	return false;
}

bool GSIOTClient::Compare_Control( const ControlBase *ctlA, const ControlBase *ctlB )
{
	if( ctlA == ctlB )
	{
		return true;
	}

	if( ctlA && ctlB )
	{
		if( ctlA->GetLinkID() == ctlB->GetLinkID()  && ctlA->GetExType() == ctlB->GetExType() )
		{
			switch( ctlA->GetExType() )
			{
			case IOT_DEVICE_RFDevice:
				{
					break;
				}
			case IOT_DEVICE_CANDevice:
				{
					break;
				}
			case IOT_DEVICE_RS485:
				{
					if( ((RS485DevControl*)ctlA)->GetDeviceid()==((RS485DevControl*)ctlB)->GetDeviceid() )
					{
						return true;
					}
				}
				break;
			case IOTDevice_AC_Ctl:
				{
					if( ((GSRemoteCtl_AC*)ctlA)->isSameRmtCtl( (GSRemoteCtl_AC*)ctlB ) )
					{
						return true;
					}
				}
				break;
			}
		}
	}

	return false;
}

bool GSIOTClient::Compare_Address( const DeviceAddress *AddrA, const DeviceAddress *AddrB )
{
	if( AddrA == AddrB )
		return true;

	if( AddrA && AddrB )
	{
		if( AddrA->GetAddress() == AddrB->GetAddress() )
		{
			return true;
		}
	}

	return false;
}

bool GSIOTClient::Compare_GSIOTObjBase( const GSIOTObjBase *ObjA, const GSIOTObjBase *ObjB )
{
	if( ObjA == ObjB )
		return true;

	if( ObjA && ObjB )
	{
		if( ObjA->GetObjType()==ObjB->GetObjType() && ObjA->GetId() == ObjB->GetId() )
		{
			return true;
		}
	}

	return false;
}

bool GSIOTClient::Compare_ControlAndAddress( const ControlBase *ctlA, const DeviceAddress *AddrA, const ControlBase *ctlB, const DeviceAddress *AddrB )
{
	if( !GSIOTClient::Compare_Control( ctlA, ctlB ) )
		return false;

	return Compare_Address( AddrA, AddrB );
}

void GSIOTClient::XmppClientSend( const IQ& iq, const char *callinfo )
{
	if( xmppClient )
	{
		XmppPrint( iq, callinfo );
		xmppClient->send( iq );
	}
}

static uint32_t s_XmppClientSend_msg_sno = 0;
void GSIOTClient::XmppClientSend_msg( const JID &to_jid, const std::string &strBody, const std::string &strSubject, const char *callinfo )
{
	if( xmppClient )
	{
		std::string msgid = util::int2string(++s_XmppClientSend_msg_sno);

#if 1
		EventNoticeMsg_Send( to_jid.full(), strSubject, strBody, "XmppClientSend_msg at once" );
#else
		struEventNoticeMsg *msgbuf = new struEventNoticeMsg( msgid, ::timeGetTime(), to_jid, strSubject, strBody, callinfo );
		if( !EventNoticeMsg_Add( msgbuf ) )
		{
			delete msgbuf;
		}
#endif

		Message msg( Message::Normal, to_jid, ASCIIToUTF8(strBody), ASCIIToUTF8(strSubject) );
		msg.setID( msgid );

		XmppPrint( msg, callinfo );
		xmppClient->send( msg ); //qjyc20160923
	}
}

void GSIOTClient::XmppClientSend_jidlist( const std::set<std::string> &jidlist, const std::string &strBody, const std::string &strSubject, const char *callinfo )
{
	LOGMSG( "XmppClientSend_jidlist num=%d\r\n", jidlist.size() );
	
	std::set<std::string>::const_iterator it = jidlist.begin();
	for( ; it!=jidlist.end(); ++it )
	{
		JID to_jid;
		to_jid.setJID( *it );

		if( to_jid )
		{
			XmppClientSend_msg( to_jid, strBody, strSubject, callinfo );
		}
	}
}

void GSIOTClient::XmppPrint( const Message& msg, const char *callinfo )
{
	XmppPrint( msg.tag(), callinfo, NULL );
}

void GSIOTClient::XmppPrint( const IQ& iq, const char *callinfo )
{
	XmppPrint( iq.tag(), callinfo, NULL, false); //20160612 add false
}

void GSIOTClient::XmppPrint( const Tag *ptag, const char *callinfo, const Stanza *stanza, bool dodel )
{
	std::string strxml;
	if( ptag )
	{
		strxml = ptag->xml();
		strxml = UTF8ToASCII( strxml );
	}
	else
	{
		strxml = "<no tag>";
	}
	printf( "GSIOT %s from=\"%s\", xml=\"%s\"\r\n", callinfo?callinfo:"", stanza?stanza->from().full().c_str():"NULL", strxml.c_str() );
	if( ptag && dodel )
	{
		delete ptag;
	}
}

GSIOTDevice* GSIOTClient::GetIOTDevice( IOTDeviceType deviceType, uint32_t deviceId ) const
{
	//jyc20170331 resume
	if( IOT_DEVICE_Camera == deviceType ){
		return ipcamClient->GetIOTDevice( deviceId );
	}

	return deviceClient->GetIOTDevice( deviceType, deviceId );
}

std::string GSIOTClient::GetAddrObjName( const GSIOTAddrObjKey &AddrObjKey ) const
{
	const GSIOTDevice *pDev = this->GetIOTDevice( AddrObjKey.dev_type, AddrObjKey.dev_id );
	if( !pDev )
	{
		return std::string("");
	}

	return pDev->getName() + "-" + GetDeviceAddressName( pDev, AddrObjKey.address_id );
}

std::string GSIOTClient::GetDeviceAddressName( const GSIOTDevice *device, uint32_t address ) const
{
	switch( device->getType() )
	{
	case IOT_DEVICE_Remote:
		{
			RFRemoteControl *ctl = (RFRemoteControl*)device->getControl();
			const RemoteButton *pbtn = ctl->GetButton( address );
			if( pbtn )
			{
				return pbtn->GetObjName();
			}
		}
		break;

	case IOT_DEVICE_RS485:
		{					
			RS485DevControl *ctl = (RS485DevControl *)device->getControl();
			const DeviceAddress *paddr = ctl->GetAddress( address );
			if( paddr )
			{
				return paddr->GetName();
			}
		}
		break;

	case IOT_DEVICE_Camera:
		{	//jyc20170331 resume				
			IPCameraBase *ctl = (IPCameraBase *)device->getControl();
			const CPresetObj *preset = ctl->GetPreset( address );
			if( preset )
			{
				return preset->GetObjName();
			}
		}
		break;
	}

	return std::string("");
}

bool GSIOTClient::DeleteDevice( GSIOTDevice *iotdevice )
{
	this->m_event->DeleteDeviceEvent( iotdevice->getType(), iotdevice->getId() );

	if( IOT_DEVICE_Camera == iotdevice->getType() )
	{   //jyc20170331 resume
		return this->ipcamClient->RemoveIPCamera( iotdevice );
	}

	return this->deviceClient->DeleteDevice( iotdevice );
}

bool GSIOTClient::ModifyDevice_Ver( GSIOTDevice *iotdevice, const std::string ver )
{
	if( !iotdevice )
		return false;

	if( !iotdevice->getControl() )
		return false;

	iotdevice->setVer( ver );

	switch( iotdevice->getControl()->GetType() )
	{
	case IOT_DEVICE_RS485:
		{
			RS485DevControl *pCtl = (RS485DevControl*)iotdevice->getControl();
			pCtl->setVer( ver );
		}
		break;
	}

	if( IOT_DEVICE_Camera == iotdevice->getType() )
	{   //jyc20170331 resume
		ipcamClient->ModifyDevice( iotdevice );
	}
	else
	{
		deviceClient->ModifyDevice( iotdevice );
	}

	return true;
}

//jyc20170405 resume
void GSIOTClient::PlaybackCmd_DeleteCmd( struPlaybackCmd *pCmd )
{
	if( pCmd )
	{
		if( pCmd->pXmpp )
		{
			delete pCmd->pXmpp;
			pCmd->pXmpp = NULL;
		}

		delete pCmd;
	}
}

void GSIOTClient::PlaybackCmd_push( const XmppGSPlayback *pXmpp, const IQ& iq )
{
	gloox::util::MutexGuard mutexguard( m_mutex_lstPlaybackList );

	if( m_lstPlaybackCmd.size()>999 )
	{
		LOGMSG( "PlaybackCmd list limit 999! throw jid=%s", iq.from().full().c_str() );
		return ;
	}

	m_lstPlaybackCmd.push_back( new struPlaybackCmd( iq.from(), iq.id(), pXmpp ) );
}

struPlaybackCmd* GSIOTClient::PlaybackCmd_pop()
{
	gloox::util::MutexGuard mutexguard( m_mutex_lstPlaybackList );

	m_PlaybackThreadTick = timeGetTime();

	if( m_lstPlaybackCmd.empty() )
	{
		return NULL;
	}

	struPlaybackCmd *pCmd = m_lstPlaybackCmd.front();
	m_lstPlaybackCmd.pop_front();
	return pCmd;
}

void GSIOTClient::PlaybackCmd_clean()
{
	gloox::util::MutexGuard mutexguard( m_mutex_lstPlaybackList );

	if( m_lstPlaybackCmd.empty() )
	{
		return;
	}

	for( std::list<struPlaybackCmd*>::iterator it = m_lstPlaybackCmd.begin(); it!=m_lstPlaybackCmd.end(); ++it )
	{
		PlaybackCmd_DeleteCmd( *it );
	}

	m_lstPlaybackCmd.clear();
}

// return false : no work
bool GSIOTClient::PlaybackCmd_OnProc()
{
	struPlaybackCmd *pCmd = PlaybackCmd_pop();

	if( !pCmd )
	{
		return false;
	}

	PlaybackCmd_ProcOneCmd( pCmd );
	PlaybackCmd_DeleteCmd( pCmd );
	return true;
}

void GSIOTClient::Playback_ThreadCreate()
{
	m_PlaybackThreadCreateCount++;
	if( m_PlaybackThreadCreateCount > 99 )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "PlaybackProcThread Running(count=%d) limit error.", m_PlaybackThreadCreateCount );
		return;
	}

	m_isPlayBackThreadExit = false;

	pthread_t id_1;  
    int ret=pthread_create(&id_1, NULL, PlaybackProc_Thread, this );
    if(ret!=0)  
    {  
        printf("Create PlaybackProc_Thread error!\n");   
		return; 
    } 
}

void GSIOTClient::Playback_ThreadCheck()
{
	bool check_timeout = false;
	
	m_mutex_lstPlaybackList.lock();

	if( timeGetTime()-m_PlaybackThreadTick > 60000 )
	{
		check_timeout = true;
	}

	m_mutex_lstPlaybackList.unlock();

	if( check_timeout )
	{
		Playback_ThreadCreate();
	}
}

void GSIOTClient::Playback_ThreadPrinthb()
{
	LOGMSG( "PlaybackProcThread heartbeat(threadcount=%d)", m_PlaybackThreadCreateCount );
}

void GSIOTClient::Playback_DeleteDevOne( GSIOTDevice *device )
{
	((IPCameraBase*)(device->getControl()))->OnDisconnct();
	((IPCameraBase*)(device->getControl()))->StopRTMPSendAll();
	delete device;
}

bool GSIOTClient::Playback_IsLimit()
{
	const int PlaybackChannelLimit = RUNCODE_Get(defCodeIndex_SYS_PlaybackChannelLimit);

	gloox::util::MutexGuard mutexguard( m_mutex_lstPlaybackList );

	if( m_lstPlaybackList.size() >= PlaybackChannelLimit )
	{
		return true;
	}

	return false;
}

uint32_t GSIOTClient::Playback_GetNowCount()
{
	gloox::util::MutexGuard mutexguard( m_mutex_lstPlaybackList );

	return m_lstPlaybackList.size();
}

void GSIOTClient::Playback_GetInfoList( std::map<std::string,struPlaybackSession> &getlstPlaybackList )
{
	gloox::util::MutexGuard mutexguard( m_mutex_lstPlaybackList );

	std::map<std::string, struPlaybackSession>::iterator it = m_lstPlaybackList.begin();
	while( it != m_lstPlaybackList.end() )
	{
		it->second.lastUpdateTS = ((IPCameraBase*)(it->second.dev->getControl()))->GetSessionLastUpdateTime( c_NullStr );
		it++;
	}

	getlstPlaybackList = m_lstPlaybackList;
}

bool GSIOTClient::Playback_Exist( const std::string &key )
{
	gloox::util::MutexGuard mutexguard( m_mutex_lstPlaybackList );

	if( m_lstPlaybackList.find( key ) != m_lstPlaybackList.end() )
	{
		return true;
	}

	return false;
}

bool GSIOTClient::Playback_Add( const std::string &from_id, const std::string &key, const std::string &url, const std::string &peerid, const std::string &streamid, GSIOTDevice *device )
{
	if( !device )
		return false;

	if( !device->getControl() )
		return false;

	if( Playback_IsLimit() )
		return false;

	gloox::util::MutexGuard mutexguard( m_mutex_lstPlaybackList );

	if( m_lstPlaybackList.find( key ) != m_lstPlaybackList.end() )
	{
		return false;
	}

	m_lstPlaybackList[key] = struPlaybackSession(key, from_id, url, peerid, streamid, device->getName(), device);
	((IPCameraBase*)(device->getControl()))->UpdateSession( JID(defPlayback_SessionName) );
	return true;
}

void GSIOTClient::Playback_Delete( const std::string &key )
{
	gloox::util::MutexGuard mutexguard( m_mutex_lstPlaybackList );

	std::map<std::string,struPlaybackSession>::iterator it = m_lstPlaybackList.find( key );
	if( it != m_lstPlaybackList.end() )
	{
		Playback_DeleteDevOne( it->second.dev );
		m_lstPlaybackList.erase( it );
	}
}

void GSIOTClient::Playback_DeleteForJid( const std::string &from_jid )
{
	gloox::util::MutexGuard mutexguard( m_mutex_lstPlaybackList );

	std::map<std::string,struPlaybackSession>::iterator it = m_lstPlaybackList.begin();
	while( it != m_lstPlaybackList.end() )
	{
		if( from_jid == it->second.from_jid )
		{
			Playback_DeleteDevOne( it->second.dev );
			m_lstPlaybackList.erase( it );
			it = m_lstPlaybackList.begin();
		}
		else
		{
			it++;
		}
	}
}

void GSIOTClient::Playback_DeleteAll()
{
	gloox::util::MutexGuard mutexguard( m_mutex_lstPlaybackList );

	if( m_lstPlaybackList.empty() )
		return ;

	LOGMSG( "Playback_DeleteAll" );

	std::map<std::string,struPlaybackSession>::iterator it = m_lstPlaybackList.begin();
	while( it!=m_lstPlaybackList.end() )
	{
		Playback_DeleteDevOne( it->second.dev );
		m_lstPlaybackList.erase(it);
		it = m_lstPlaybackList.begin();
	}
}

void GSIOTClient::Playback_SetForJid( const std::string &from_jid, int sound )
{
	gloox::util::MutexGuard mutexguard( m_mutex_lstPlaybackList );

	std::map<std::string,struPlaybackSession>::iterator it = m_lstPlaybackList.begin();
	while( it != m_lstPlaybackList.end() )
	{
		if( from_jid == it->second.from_jid )
		{
			if( sound>=0 )
			{
				LOGMSG( "Playback_SetForJid sound=%d, from=\"%s\"\r\n", sound, from_jid.c_str() );
				((IPCameraBase*)(it->second.dev->getControl()))->GetStreamObj()->GetRTMPSendObj()->set_playback_sound( sound );
			}

			break;
		}
		else
		{
			it++;
		}
	}
}

GSPlayBackCode_ GSIOTClient::Playback_CtrlForJid( const std::string &from_jid, GSPlayBackCode_ ControlCode, void *pInBuffer, uint32_t InLen, void *pOutBuffer, uint32_t *pOutLen )
{
	gloox::util::MutexGuard mutexguard( m_mutex_lstPlaybackList );

	std::map<std::string,struPlaybackSession>::iterator it = m_lstPlaybackList.begin();
	while( it != m_lstPlaybackList.end() )
	{
		if( from_jid == it->second.from_jid )
		{
			LOGMSG( "Playback_CtrlForJid Ctrl=%d, from=\"%s\"", ControlCode, from_jid.c_str() );

			if( GSPlayBackCode_GetState != ControlCode )
			{
				PlayBackControl_nolock( (IPCameraBase*)(it->second.dev->getControl()), ControlCode, pInBuffer, InLen, pOutBuffer, pOutLen );
			}

			if( pOutBuffer )
			{
				*(int*)pOutBuffer = ((IPCameraBase*)(it->second.dev->getControl()))->GetStreamObj()->GetRTMPSendObj()->GetPlayBackCtrl_speedlevel();
			}

			return ((IPCameraBase*)(it->second.dev->getControl()))->GetStreamObj()->GetRTMPSendObj()->GetPlayBackCtrl_Code();
		}
		else
		{
			it++;
		}
	}

	return GSPlayBackCode_Stop;
}

void GSIOTClient::Playback_CtrlResult( const JID &from_Jid, const std::string &from_id, const XmppGSPlayback *pXmppSrc, const GSPlayBackCode_ ControlCode, void *pOutBuffer, uint32_t *pOutLen )
{
	XmppGSPlayback::defPBState state = XmppGSPlayback::defPBState_Unknown;
	switch( ControlCode )
	{
	case GSPlayBackCode_Stop:
		{
			state = XmppGSPlayback::defPBState_Stop;
			break;
		}

	case GSPlayBackCode_PLAYPAUSE:
		{
			state = XmppGSPlayback::defPBState_Pause;
			break;
		}

	case GSPlayBackCode_PLAYFAST:
		{
			if( pOutBuffer )
			{
				int speedlevel = *(int*)pOutBuffer;
				if( 2==speedlevel )
				{
					state = XmppGSPlayback::defPBState_FastPlay2;
					break;
				}
				else if( 1==speedlevel )
				{
					state = XmppGSPlayback::defPBState_FastPlay1;
					break;
				}
			}

			state = XmppGSPlayback::defPBState_FastPlay;
			break;
		}

	case GSPlayBackCode_PLAYSLOW:
		{

			if( pOutBuffer )
			{
				int speedlevel = *(int*)pOutBuffer;
				if( -2==speedlevel )
				{
					state = XmppGSPlayback::defPBState_SlowPlay2;
					break;
				}
				else if( -1==speedlevel )
				{
					state = XmppGSPlayback::defPBState_SlowPlay1;
					break;
				}
			}

			state = XmppGSPlayback::defPBState_SlowPlay;
			break;
		}

	case GSPlayBackCode_PLAYNORMAL:
	default:
		{
			state = XmppGSPlayback::defPBState_NormalPlay;
			break;
		}
	}

	IQ re( IQ::Result, from_Jid, from_id);
	re.addExtension( new XmppGSPlayback( struTagParam(), 
		pXmppSrc->get_camera_id(), pXmppSrc->get_url(), pXmppSrc->get_peerid(), pXmppSrc->get_streamid(), pXmppSrc->get_url(), state // key use url
		) );
	XmppClientSend(re,"handleIq Send(Playback_CtrlResult)");
}

//test
int GSIOTClient::PlayBackControl_GetCurState_test( GSPlayBackCode_ &curPB_Code, int &curPB_speedlevel, int &curPB_ThrowFrame )
{
	curPB_Code = GSPlayBackCode_NULL;
	curPB_speedlevel = 0;
	curPB_ThrowFrame = 0;

	gloox::util::MutexGuard mutexguard( m_mutex_lstPlaybackList );

	std::map<std::string,struPlaybackSession>::iterator it = m_lstPlaybackList.begin();
	while( it != m_lstPlaybackList.end() )
	{
		LOGMSG( "PlayBackControl_GetCurState_test found\r\n" );
		IPCameraBase *pcam = (IPCameraBase*)(it->second.dev->getControl());
		curPB_Code = pcam->GetStreamObj()->GetRTMPSendObj()->GetPlayBackCtrl_Code();
		curPB_speedlevel = pcam->GetStreamObj()->GetRTMPSendObj()->GetPlayBackCtrl_speedlevel();
		curPB_ThrowFrame = pcam->GetStreamObj()->GetRTMPSendObj()->GetPlayBackCtrl_ThrowFrame();
		return 1;
	}

	LOGMSG( "PlayBackControl_GetCurState_test not found\r\n" );
	return -1;
}

//test
int GSIOTClient::PlayBackControl_test( GSPlayBackCode_ ControlCode, void *pInBuffer, uint32_t InLen, void *pOutBuffer, uint32_t *pOutLen )
{
	gloox::util::MutexGuard mutexguard( m_mutex_lstPlaybackList );

	std::map<std::string,struPlaybackSession>::iterator it = m_lstPlaybackList.begin();
	while( it != m_lstPlaybackList.end() )
	{
		int ret = 1;
		if( GSPlayBackCode_GetState != ControlCode )
		{
			ret = this->PlayBackControl_nolock( (IPCameraBase*)(it->second.dev->getControl()), ControlCode, pInBuffer, InLen, pOutBuffer, pOutLen );
		}

		LOGMSG( "PlayBackControl_test found state=%d\r\n", ((IPCameraBase*)(it->second.dev->getControl()))->GetStreamObj()->GetRTMPSendObj()->GetPlayBackCtrl_Code() );

		return ret;
	}

	LOGMSG( "PlayBackControl_test not found\r\n" );
	return -1;
}

int GSIOTClient::PlayBackControl_nolock( IPCameraBase *pcam, GSPlayBackCode_ ControlCode, void *pInBuffer, uint32_t InLen, void *pOutBuffer, uint32_t *pOutLen )
{
	LOGMSG( "PlayBackControl_nolock(%s) code=%d(%s), InValue=%u\r\n", pcam->GetName().c_str(), ControlCode, get_GSPlayBackCode_Name(ControlCode).c_str(), (uint32_t)pInBuffer );

	switch( ControlCode )
	{
	case GSPlayBackCode_SkipTime:
	case GSPlayBackCode_PLAYFAST:
	case GSPlayBackCode_PLAYSLOW:
	case GSPlayBackCode_PLAYNORMAL:
	case GSPlayBackCode_PLAYPAUSE:
	case GSPlayBackCode_PLAYRESTART:
		{
			return pcam->GetStreamObj()->GetRTMPSendObj()->PlayBackControl( ControlCode, (uint32_t)pInBuffer );
		}
		break;

	default:
		{
			int ret = pcam->PlayBackControl( ControlCode, pInBuffer, InLen, pOutBuffer, pOutLen );
			if( ret < 0 )
			{
				LOGMSG( "PlayBackControl_nolock failed\r\n" );
			}
			else
			{
				LOGMSG( "PlayBackControl_nolock success\r\n" );

				if( GSPlayBackCode_PlaySetTime == ControlCode 
					|| GSPlayBackCode_PLAYSETPOS == ControlCode
					)
				{
					pcam->GetStreamObj()->GetRTMPSendObj()->ClearListVideoPacket();
					//pcam->GetStreamObj()->GetRTMPSendObj()->ReKeyListVideoPacket();
				}
			}

			return ret;
		}
		break;
	}

	return -1;
}

void GSIOTClient::Playback_UpdateSession( const std::string &key )
{
	gloox::util::MutexGuard mutexguard( m_mutex_lstPlaybackList );

	std::map<std::string,struPlaybackSession>::iterator it = m_lstPlaybackList.find( key );
	if( it != m_lstPlaybackList.end() )
	{
		((IPCameraBase*)(it->second.dev->getControl()))->UpdateSession( JID(defPlayback_SessionName) );
	}
}

void GSIOTClient::Playback_CheckSession()
{
	gloox::util::MutexGuard mutexguard( m_mutex_lstPlaybackList );

	if( m_lstPlaybackList.empty() )
		return ;

	LOGMSG( "Playback_CheckSession, SessionCount=%d", m_lstPlaybackList.size() );

	std::map<std::string,struPlaybackSession>::iterator it = m_lstPlaybackList.begin();
	while( it!=m_lstPlaybackList.end() )
	{
		((IPCameraBase*)(it->second.dev->getControl()))->CheckSession( 1, true, false );

		if( ((IPCameraBase*)(it->second.dev->getControl()))->GetSessionCount() <=0 )
		{
			LOGMSG( "Playback Session Timeout, devid=%d, devname=%s", it->second.dev->getId(), it->second.dev->getName().c_str() );

			Playback_DeleteDevOne( it->second.dev );
			m_lstPlaybackList.erase(it);
			it = m_lstPlaybackList.begin();
			continue;
		}

		uint32_t pos = -1;
		uint32_t outlen = sizeof(pos);
		((IPCameraBase*)(it->second.dev->getControl()))->PlayBackControl( GSPlayBackCode_PLAYGETPOS, 0, 0, &pos, &outlen );

		if( pos<0 || pos >= 100 )
		{
			// notice stop
			JID to_jid(it->second.from_jid);
			IQ re( IQ::Result, to_jid );
			re.addExtension( new XmppGSPlayback( struTagParam(), 
				it->second.dev->getId(), it->second.url, it->second.peerid, it->second.streamid, it->first, XmppGSPlayback::defPBState_Stop // key use url
				) );
			XmppClientSend(re,"handleIq Send(Playback_CheckSession Play end notice)");

			// delete
			Playback_DeleteDevOne( it->second.dev );
			m_lstPlaybackList.erase(it);
			it = m_lstPlaybackList.begin();
			continue;
		}

		if( !it->second.check() )
		{
			// notice stop
			JID to_jid( it->second.from_jid );
			IQ re( IQ::Result, to_jid );
			re.addExtension( new XmppGSPlayback( struTagParam(),
				it->second.dev->getId(), it->second.url, it->second.peerid, it->second.streamid, it->first, XmppGSPlayback::defPBState_Stop // key use url
				) );
			XmppClientSend( re, "handleIq Send(Playback_CheckSession Play overtime notice)" );

			// delete
			Playback_DeleteDevOne( it->second.dev );
			m_lstPlaybackList.erase( it );
			it = m_lstPlaybackList.begin();
			continue;
		}

		++it;
	}
} //jyc20170405 resume until here

bool GSIOTClient::EventNoticeMsg_Add( struEventNoticeMsg *msg )
{
	if( !msg )
		return false;

	gloox::util::MutexGuard mutexguard( m_mutex_lstEventNoticeMsg );

	if( m_lstEventNoticeMsg.find( msg->id ) != m_lstEventNoticeMsg.end() )
	{
		return false;
	}

	m_lstEventNoticeMsg[msg->id] = msg;

	return true;
}

void GSIOTClient::EventNoticeMsg_Remove( const std::string &id )
{
	gloox::util::MutexGuard mutexguard( m_mutex_lstEventNoticeMsg );

	std::map<std::string,struEventNoticeMsg*>::iterator it = m_lstEventNoticeMsg.find( id );
	if( it != m_lstEventNoticeMsg.end() )
	{
		delete it->second;
		m_lstEventNoticeMsg.erase( it );
	}
}

void GSIOTClient::EventNoticeMsg_Check()
{
	gloox::util::MutexGuard mutexguard( m_mutex_lstEventNoticeMsg );

	if( m_lstEventNoticeMsg.empty() )
		return ;

	LOGMSG( "EventNoticeMsg_Check, Count=%d", m_lstEventNoticeMsg.size() );

	std::map<std::string,struEventNoticeMsg*>::iterator it = m_lstEventNoticeMsg.begin();
	while( it!=m_lstEventNoticeMsg.end() )
	{
		if( ::timeGetTime() - it->second->starttime >= 10000 )
		{
			LOGMSG( "EventNoticeMsg Timeout, id=%s, to_jid=%s, Subject=%s", it->second->id.c_str(), it->second->to_jid.full().c_str(), it->second->strSubject.c_str() );

			// sent to server
#if 1
			EventNoticeMsg_Send( it->second->to_jid.full(), it->second->strSubject, it->second->strBody, "handleIq Send(EventNoticeMsg_Check Timeout, sendto svr)" );
#else
			IQ re( IQ::Set, JID("webservice@gsss.cn/iotserver-side") );
			re.addExtension( new XmppGSMessage( struTagParam(), it->second->to_jid.full(), it->second->strSubject, it->second->strBody ) );
			XmppClientSend(re,"handleIq Send(EventNoticeMsg_Check Timeout, sendto svr)");
#endif

			// erase
			delete it->second;
			m_lstEventNoticeMsg.erase(it);
			it = m_lstEventNoticeMsg.begin();
			continue;
		}
		
		++it;
	}
}

void GSIOTClient::EventNoticeMsg_Send( const std::string &tojid, const std::string &subject, const std::string &body, const char *callinfo )
{
	IQ re( IQ::Set, JID("webservice@gsss.cn/iotserver-side") );
	re.addExtension( new XmppGSMessage( struTagParam(), tojid, subject, body ) );
	XmppClientSend( re, callinfo );
}

int _System(const char * cmd, char *pRetMsg, int msg_len)  //jyc20170319 add
{  
    FILE * fp;  
    char * p = NULL;  
    int res = -1;  
    if (cmd == NULL || pRetMsg == NULL || msg_len < 0)  
    {  
        printf("Param Error!\n");  
        return -1;  
    }  
    if ((fp = popen(cmd, "r") ) == NULL)  
    {  
        printf("Popen Error!\n");  
        return -2;  
    }  
    else  
    {  
        memset(pRetMsg, 0, msg_len);  
        //get lastest result  
        while(fgets(pRetMsg, msg_len, fp) != NULL)  
        {  
            printf("Msg:%s",pRetMsg); //print all info  
        }  
  
        if ( (res = pclose(fp)) == -1)  
        {  
            printf("close popenerror!\n");  
            return -3;  
        }  
        pRetMsg[strlen(pRetMsg)-1] = '\0';  
        return 0;  
    }  
} 

bool GSIOTClient::Update_Check( std::string &strVerLocal, std::string &strVerNew )
{
	m_retCheckUpdate = false;
	strVerLocal = "";
	strVerNew = "";
	m_strVerLocal = "";
	m_strVerNew = "";
	//std::string strPath = getAppPath();

	// get local version  jyc notice 
	/*char bufval[256] = {0};
	const DWORD nSize = sizeof(bufval);
	DWORD ReadSize = ::GetPrivateProfileStringA( "sys", "version", "0",  //jyc20170318 debug
		bufval,
		nSize,
		(std::string(defFilePath)+"\\version.ini").c_str()
		);*/
#ifdef OS_UBUNTU_FLAG   //jyc20170319 add
	char *cmd = "pwd";  
#else
	char *cmd = "opkg list-installed|grep gsiot";
	//system("cd /root"); //jyc20170321 add
#endif
    char a8Result[128] = {0};  
    int res = 0;  
    res  = _System(cmd, a8Result, sizeof(a8Result));  
	
	m_strVerLocal = &a8Result[8];  //jyc20170319 modify

	printf("ret = %d \na8Result = %s\nlength = %d \n", res, m_strVerLocal.c_str(), strlen(a8Result));

	//std::string reqstr = "http://api.gsss.cn/iot_ctl_update.ashx";
	std::string reqstr = "http://api.gsss.cn/iot_ctl_update.ashx?getupdate=box_update";
	httpreq::Request req_setjid;
	std::string psHeaderSend;
	std::string psHeaderReceive;
	std::string psMessage;
	if( !req_setjid.SendRequest( false, reqstr.c_str(), psHeaderSend, psHeaderReceive, psMessage ) )  //?ver=20131200
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "SendRequest to server send failed." );
		return false;
	}
	//LOGMSGEX( defLOGNAME, defLOG_INFO, "SendRequest to server send success. HeaderReceive=\"%s\", Message=\"%s\"", UTF8ToASCII(psHeaderReceive).c_str(), UTF8ToASCII(psMessage).c_str() );	
	std::string copy = psMessage;
	printf("copy=%s",copy.c_str());
	int ret = 0; 
	if( ( ret = m_parser.feed( copy ) ) >= 0 )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "parser err. ret=%d", ret );
		return false;
	}
	strVerLocal = m_strVerLocal;
	strVerNew = m_strVerNew;
	return m_retCheckUpdate;
}

void GSIOTClient::Update_Check_fromremote( const JID &fromjid, const std::string& from_id )
{
	sg_blUpdatedProc++;
	std::string strVerLocal;
	std::string strVerNew;
	XmppGSUpdate::defUPState state = XmppGSUpdate::defUPState_Unknown;

	if( this->Update_Check( strVerLocal, strVerNew ) )
	{
		if( strVerLocal == strVerNew )
		{
			state = XmppGSUpdate::defUPState_latest;
		}
		else
		{
			state = XmppGSUpdate::defUPState_update;
		}
	}
	else
	{
		state = XmppGSUpdate::defUPState_checkfailed;
	}
	IQ re( IQ::Result, fromjid, from_id );
	re.addExtension( new XmppGSUpdate( struTagParam(), 
		strVerLocal, strVerNew, state
		) );
	XmppClientSend(re,"handleIq Send(ExtIotUpdate check ACK)");
}

void GSIOTClient::handleTag( Tag* tag )
{
	std::string strPath = getAppPath();

	Tag *tmgr = tag;
	if( tag->name() != "update" )
	{
		printf( "not fond update tag." );
		return;
	}

	if( tmgr->findChild("version") )
	{
		m_strVerNew = tmgr->findChild("version")->cdata();
	}
	else
	{
		printf( "not get new ver!" );
		return;
	}

	m_retCheckUpdate = true;

	printf( "localver=%s, newver=%s", m_strVerLocal.c_str(), m_strVerNew.c_str() );
}

bool GSIOTClient::Update_DoUpdateNow( uint32_t &err, std::string runparam )
{
	err = 0;
	/*LOGMSGEX( defLOGWatch, defLOG_SYS, "update program...\r\n" );
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "run update" );
	HINSTANCE h = ShellExecuteA( NULL, "open", (std::string(defFilePath)+"\\"+defFileName_Update).c_str(), runparam.c_str(), NULL, SW_SHOWNORMAL );
	if( (uint32_t)h > 32 )
		return true;

	err = (uint32_t)h;
	*/
#ifndef OS_UBUNTU_FLAG
	char *cmd1 = "opkg install /root/gsbox.ipk";
	char *cmd2 = "opkg install /root/gsiot.ipk";
    char a8Result[128] = {0};
	char debugmsg[64] = "echo \"$(date)\" 00000000000 >>/root/err.log";	
	//system(debugmsg);
	system("/root/getgsipk.sh");
	//system(debugmsg);
	//system("cd /root"); //jyc20170321 add
	_System(cmd1, a8Result, sizeof(a8Result));
	if(memcmp(a8Result,"Configuring",11))return false;
	memcpy(&debugmsg[15],a8Result,(sizeof(a8Result)>11)?11:sizeof(a8Result));
	system(debugmsg);
	//system("cd /root"); //jyc20170321 add
	_System(cmd2, a8Result, sizeof(a8Result));
	if(memcmp(a8Result,"Configuring",11))return false;
	memcpy(&debugmsg[15],a8Result,(sizeof(a8Result)>11)?11:sizeof(a8Result));
	system(debugmsg);	
	return true;
#endif
	return false;
}

void GSIOTClient::Update_DoUpdateNow_fromremote( const JID &fromjid, const std::string& from_id, std::string runparam )
{
	uint32_t err = 0;

	if( this->Update_DoUpdateNow( err, runparam ) )  //jyc20170321 modify
	{
		IQ re( IQ::Result, fromjid, from_id);
		re.addExtension( new XmppGSUpdate( struTagParam(), 
			"", "", XmppGSUpdate::defUPState_successupdated //XmppGSUpdate::defUPState_progress 
			) );
		XmppClientSend(re,"handleIq Send(ExtIotUpdate update ACK)");
	}
	else
	{
		IQ re( IQ::Result, fromjid, from_id);
		re.addExtension( new XmppGSUpdate( struTagParam(), 
			"", "", XmppGSUpdate::defUPState_updatefailed
			) );
		XmppClientSend(re,"handleIq Send(ExtIotUpdate update ACK)");
	}
}

void GSIOTClient::Update_UpdatedProc()
{
	if( sg_blUpdatedProc )
	{
		return;
	}

	static uint32_t st_Update_UpdatedProc = timeGetTime();
	if( timeGetTime()-st_Update_UpdatedProc < 5000 )
		return;

	sg_blUpdatedProc++;

	char bufval_fromjid[256] = {0};
	/*const DWORD nSize_fromjid = sizeof(bufval_fromjid);
	ReadSize = ::GetPrivateProfileStringA( 
		"update", 
		"fromjid", "", 
		bufval_fromjid,
		nSize_fromjid,
		(std::string(defFilePath)+"\\temp.ini").c_str()
		);*/

	std::string fromjid = bufval_fromjid;
	//fromjid = "chen009@gsss.cn";//test
	if( fromjid.empty() )
	{
		return;
	}

	IQ re( IQ::Set, fromjid );
	re.addExtension( new XmppGSUpdate( struTagParam(), 
		"", "", XmppGSUpdate::defUPState_successupdated
		) );
	XmppClientSend(re,"handleIq Send(ExtIotUpdate successupdated ACK)");
}


void* GSIOTClient::OnTalkNotify( const XmppGSTalk::defTalkCmd cmd, const std::string &url, const std::string &from_Jid, const std::string &from_id, bool isSyncReturn, const defvecDevKey &vecdev, bool result, IOTDeviceType getdev_type, int getdev_id )
{
	switch( cmd )
	{
	case XmppGSTalk::defTalkCmd_session:	// 会话
		{
			IQ re( IQ::Result, JID(from_Jid), from_id );
			re.addExtension( new XmppGSTalk( struTagParam(),
				XmppGSTalk::defTalkCmd_session, url, vecdev ) );
			XmppClientSend( re, "OnTalkReturn session success" );
		}
		break;

	case XmppGSTalk::defTalkCmd_adddev:	// 增加对讲设备
		{
			IQ re( isSyncReturn?IQ::Result:IQ::Set, JID(from_Jid), isSyncReturn?from_id:"" );
			re.addExtension( new XmppGSTalk( struTagParam(),
				XmppGSTalk::defTalkCmd_adddev,  url, vecdev, result ) );
			XmppClientSend( re, "OnTalkReturn adddev" );
		}
		break;

	case XmppGSTalk::defTalkCmd_removedev:	// 移除对讲设备
		{
			IQ re( isSyncReturn?IQ::Result:IQ::Set, JID(from_Jid), isSyncReturn?from_id:"" );
			re.addExtension( new XmppGSTalk( struTagParam(),
				XmppGSTalk::defTalkCmd_removedev,  url, vecdev, result ) );
			XmppClientSend( re, "OnTalkReturn removedev" );
		}
		break;

	case XmppGSTalk::defTalkCmd_keepalive:	// 心跳
		{
			IQ re( isSyncReturn?IQ::Result:IQ::Set, JID(from_Jid), isSyncReturn?from_id:"" );
			re.addExtension( new XmppGSTalk( struTagParam(),
				XmppGSTalk::defTalkCmd_keepalive,  url, vecdev, result ) );
			XmppClientSend( re, "OnTalkReturn keepalive" );
		}
		break;

	case XmppGSTalk::defTalkCmd_quit:		// 结束
		{
#if 1
			IQ re( isSyncReturn?IQ::Result:IQ::Set, JID(from_Jid), isSyncReturn?from_id:"" );
			re.addExtension( new XmppGSTalk( struTagParam(),
				XmppGSTalk::defTalkCmd_quit, url, vecdev, result ) );
			XmppClientSend( re, "OnTalkReturn quit ret/set" );
#else
			if( isSyncReturn )
			{
				IQ re( IQ::Result, JID(from_Jid), from_id );
				re.addExtension( new XmppGSTalk( struTagParam(),
					XmppGSTalk::defTalkCmd_quit, url, vecdev, result ) );
				XmppClientSend( re, "OnTalkReturn quit isSyncReturn" );
			}
			else
			{
				Message msg( Message::Normal, JID(from_Jid), ASCIIToUTF8(url), ASCIIToUTF8(std::string(XMPP_MESSAGE_PREHEAD)+"TalkQuit") );
				XmppPrint( msg, "OnTalkReturn quit msg" );
				xmppClient->send( msg );
			}
#endif
		}
		break;

		// 内部命令
	case XmppGSTalk::defTalkSelfCmd_GetDevice:
		{
			if( IOT_DEVICE_Camera == getdev_type )
			{
				return ipcamClient->GetIOTDevice( getdev_id );
			}

			return NULL; // 暂不支持其它设备
		}
		break;

	default:
		{
//			LOGMSGEX( defLOGNAME, defLOG_ERROR, "OnTalkReturn unsupport cmd=%d!!!" );
		}
		break;
	}

	return NULL;
}

void GSIOTClient::DataProc_ThreadCreate()
{
	if( IsRUNCODEEnable(defCodeIndex_Dis_RunDataProc) )
	{
		return ;
	}

	m_isDataProcThreadExit = false;

	pthread_t id_1;  
    int ret=pthread_create(&id_1, NULL, DataProc_Thread, this );
    if(ret!=0)  
    {  
        printf("Create DataProcThread error!\n");
		return; 
    } 
}

// 
bool GSIOTClient::DataProc()
{
	static uint32_t s_DataProc_Polling_count = 1;
	s_DataProc_Polling_count++;

#if defined(defForceDataSave)
	if( IsRUNCODEEnable(defCodeIndex_TEST_ForceDataSave) )
	{
		LOGMSG( "DataProc Polling ForceDataSave!" );
	}
	else
#endif
	{
		if( 0==(s_DataProc_Polling_count%5) )
		{
			LOGMSG( "DataProc Polling(5n)" );
		}
	}

	const time_t curUTCTime = g_GetUTCTime();

	const bool LED_Enable = IsRUNCODEEnable( defCodeIndex_LED_Config );
	const int LED_Mod = RUNCODE_Get( defCodeIndex_LED_Config, defRunCodeValIndex_2 );
	const uint32_t LED_ValueOvertime = RUNCODE_Get( defCodeIndex_LED_ValueOvertime );
	const bool LED_ValueOvertime_Show = RUNCODE_Get( defCodeIndex_LED_ValueOvertime, defRunCodeValIndex_2 );
	bool hasUpdateLedShow = false;
	int LedShowMaxCount = 0;
	//std::map<std::string,struLEDShow> lstLEDShow; //<sortkey,strshow>

	std::list<GSIOTDevice*>::const_iterator it = IotDeviceList.begin();
	for( ; m_running && it!=IotDeviceList.end(); ++it )
	{
		const GSIOTDevice *iotdevice = (*it);

		if( !iotdevice->GetEnable() )
		{
			continue;
		}

		if( !iotdevice->getControl() )
		{
			continue;
		}

		ControlBase *control = iotdevice->getControl();
		switch( control->GetType() )
		{
		case IOT_DEVICE_RS485:
			{
				const defUseable useable = iotdevice->get_all_useable_state();

				RS485DevControl *ctl = (RS485DevControl*)control;
				if( ctl )
				{
					bool doSendDev = false;

					const defAddressQueue& AddressList = ctl->GetAddressList();
					std::list<DeviceAddress*>::const_iterator it = AddressList.begin();
					for( ; m_running && it!=AddressList.end(); ++it )
					{
						DeviceAddress *address = *it;
						
						if( !address->GetEnable() )
							continue;

						if( !g_isNeedSaveType(address->GetType()) )
							continue;

						const GSIOTAddrObjKey AddrObjKey( iotdevice->getType(), iotdevice->getId(), address->GetType(), address->GetAddress() );
						//jyc20160929 trouble in here no data insert
						if( m_DataStoreMgr->insertdata_CheckSaveInvalid( AddrObjKey, useable>0 ) )
						{
							gloox::util::MutexGuard mutexguard( m_mutex_DataStore );

							const size_t DataSaveBufSize = m_lstDataSaveBuf.size();
							if( DataSaveBufSize<10000 )
							{
								m_lstDataSaveBuf.push_back( new struDataSave( g_GetUTCTime(), \
								      iotdevice->getType(), iotdevice->getId(), address->GetType(), \
								      address->GetAddress(), defDataFlag_Invalid, c_ZeroStr, c_NullStr ) );
							}
							else if( DataSaveBufSize > 100 )
							{
								LOGMSG( "lstDataSaveBuf max, size=%d", m_lstDataSaveBuf.size() );
							}
						}

						//
						int polltime = 60*1000;

						switch( address->GetType() )
						{
						case IOT_DEVICE_Wind:
							{
								polltime = 6*1000;
							}
							break;

						case IOT_DEVICE_CO2:
						case IOT_DEVICE_HCHO:
						//case IOT_DEVICE_PM25:
							//break;

						case IOT_DEVICE_Temperature:
						case IOT_DEVICE_Humidity:
						default:
							{
								polltime = 60*1000;
							}
							break;
						}

#if defined(defForceDataSave)
						if( IsRUNCODEEnable(defCodeIndex_TEST_ForceDataSave) )
						{
							polltime = 8*1000;
						}
#endif

						switch( address->GetType() )
						{
						case IOT_DEVICE_CO2:
						case IOT_DEVICE_HCHO:
						//case IOT_DEVICE_PM25:
							//break;
								
						case IOT_DEVICE_Temperature:
						case IOT_DEVICE_Humidity:
						case IOT_DEVICE_Wind:
							{
								if( LED_Enable )
								{
									//jyc20160922 delete
								}

								if( doSendDev )
								{
									continue;
								}

								const int MultiReadCount = address->PopMultiReadCount();
								bool isOld = ( MultiReadCount > 0 );
								uint32_t noUpdateTime = 0;
								bool isLowSampTime = false;

								const bool curisTimePoint = g_isTimePoint(curUTCTime,address->GetType());

								if( !isOld )
								{
									const std::string strCurValue = address->GetCurValue( &isOld, &noUpdateTime, polltime, &isLowSampTime, curisTimePoint );
								}

								if( !isLowSampTime )
								{
									if( !isOld )
									{
										isOld = ( curisTimePoint && g_TransToTimePoint(curUTCTime, address->GetType(), false) \
										         !=g_TransToTimePoint(address->GetLastSaveTime(), address->GetType(), true) ); // 
									}
								}

								// 
								if( isOld )
								{
									GSIOTDevice *sendDev = iotdevice->clone( false );
									RS485DevControl *sendctl = (RS485DevControl*)sendDev->getControl();
									if( sendctl )
									{
										if( IsRUNCODEEnable(defCodeIndex_SYS_DataSamp_DoBatch) )
										{
											doSendDev = sendctl->IsCanBtachRead();
										}

										address->NowSampTick();
										sendctl->SetCommand( defModbusCmd_Read );
										this->SendControl( iotdevice->getType(), sendDev, doSendDev?NULL:address );

										LOGMSG( "Polling(%s) MRead=%d : dev(%d,%s) addr(%d%s)", curisTimePoint?"TimePoint":"", \
										       MultiReadCount, iotdevice->getId(), iotdevice->getName().c_str(), doSendDev?0:address->GetAddress(), doSendDev?"all":"" );
									}

									macCheckAndDel_Obj(sendctl);
								}
							}
							break;
						}

					}
				}
			}
			break;
		}
	}

	if( LED_Enable )
	{
		//jyc20160922 delete
	}
	return true;
}

void GSIOTClient::DataSave()
{
	defvecDataSave vecDataSave;
	const DWORD dwStart = ::timeGetTime();
	while( m_DataStoreMgr )
	{
		DataSaveBuf_Pop( vecDataSave );
		if( vecDataSave.empty() )
		{
			break;
		}

		m_DataStoreMgr->insertdata( vecDataSave );
		g_delete_vecDataSave( vecDataSave );

		if( ::timeGetTime()-dwStart > 900 )
		{
			break;
		}
	}
}

void GSIOTClient::DataStatCheck()
{
	m_DataStoreMgr->CheckStat();
}

bool GSIOTClient::DataSaveBuf_Pop( defvecDataSave &vecDataSave )
{
	gloox::util::MutexGuard mutexguard( m_mutex_DataStore );

	if( m_lstDataSaveBuf.empty() )
		return false;

	struDataSave *p = m_lstDataSaveBuf.front();
	m_lstDataSaveBuf.pop_front();
	vecDataSave.push_back( p );
	const time_t curBatchTime = p->data_dt;

	while( !m_lstDataSaveBuf.empty() )
	{
		struDataSave *p = m_lstDataSaveBuf.front();

		if( !CDataStoreMgr::IsSameDBSave( curBatchTime, p->data_dt ) )
		{
			break;
		}

		m_lstDataSaveBuf.pop_front();
		vecDataSave.push_back( p );

		if( vecDataSave.size() >= 50 )
		{
			break;
		}
	}

	return true;
}

//
void GSIOTClient::AlarmProc_ThreadCreate()
{
	m_isAlarmProcThreadExit = false;

	//HANDLE   hth1;
	//unsigned  uiThread1ID;
	//hth1 = (HANDLE)_beginthreadex( NULL, 0, AlarmProcThread, this, 0, &uiThread1ID );
	//CloseHandle(hth1);
	pthread_t id_1;  
    int ret=pthread_create(&id_1, NULL, AlarmProc_Thread, this );
    if(ret!=0)  
    {  
        printf("Create DataProcThread error!\n");
		return; 
    } 
}

//
bool GSIOTClient::AlarmProc()
{   //jyc20170331 resume but have trouble 
	const DWORD dwStart = ::timeGetTime();
	bool isdo = true;
	//jyc20160922 delete all about camera
	return isdo;
}

// 
bool GSIOTClient::AlarmCheck()
{
	bool isChanged = false;
	
	/* //jyc20170331 resume but trouble in run
	std::list<GSIOTDevice*> &cameraList = this->ipcamClient->GetCameraManager()->GetCameraList();
	std::list<GSIOTDevice*>::iterator it = cameraList.begin();
	for( ; it!=cameraList.end(); ++it )
	{
		GSIOTDevice *pDev = (*it);
		IPCameraBase *cam = (IPCameraBase*)(*it)->getControl();
		if( !cam )
			continue;

		const bool isResume = cam->CheckResumeCurAlarmState( pDev->GetEnable() );
		isChanged |= isResume;

		if( isResume )
		{
			if( IsRUNCODEEnable(defCodeIndex_SYS_CamAlarmDebugLog) )
			{
				LOGMSG( "AlarmCheck AlarmResume(%s,%s:%d:%d)\r\n", pDev->getName().c_str(), cam->GetIPAddress().c_str(), cam->GetPort(), cam->GetChannel() );
			}
		}
	}*/
	return isChanged;
}

void GSIOTClient::check_all_NetUseable( const bool CheckNow )
{
	if( !IsRUNCODEEnable(defCodeIndex_SYS_AutoCheckNetUseable) )
	{
		return ;
	}

	bool doCheck = CheckNow;

	if( CheckNow )
	{
		m_last_checkNetUseable_camid = 0;
	}

	if( !doCheck && 0!=m_last_checkNetUseable_camid )
	{
		doCheck = true;
	}

	const uint32_t dwStart = timeGetTime();
	uint32_t timeMax = defcheckNetUseable_timeMax;
	if( !doCheck )
	{
		if( dwStart-m_last_checkNetUseable_time > timeMax )
		{
			doCheck = true;
		}
	}

	if( !doCheck )
	{
		return;
	}
	return; //jyc20170405 debug have trouble
	uint32_t thisCheckCount = 0;
	std::list<GSIOTDevice*> &cameraList = this->ipcamClient->GetCameraManager()->GetCameraList();
	std::list<GSIOTDevice*>::const_iterator it = cameraList.begin();
	for( ; it!=cameraList.end(); ++it )
	{
		if( m_last_checkNetUseable_camid )
		{
			if( (*it)->getId() == m_last_checkNetUseable_camid )
			{
				m_last_checkNetUseable_camid = 0;
			}

			continue;
		}

		IPCameraBase *cam = (IPCameraBase*)(*it)->getControl();
		if(cam)
		{
			if( (*it)->GetEnable() )
			{
				thisCheckCount++;
				bool isChanged = false;
				cam->check_NetUseable( &isChanged );

				if( isChanged )
				{
					m_handler->OnDeviceNotify( defDeviceNotify_StateChanged, (*it), NULL );
				}
			}
		}

		if( timeGetTime()-dwStart > 2000 )
		{
			m_last_checkNetUseable_camid = (*it)->getId();

			LOGMSG( "check_all_NetUseable long time=%ums, lastid=%d, count=%u, timeMax=%u", timeGetTime()-dwStart, m_last_checkNetUseable_camid, thisCheckCount, timeMax );
			return;
		}
	}

	m_last_checkNetUseable_camid = 0;
	m_last_checkNetUseable_time = timeGetTime();
	LOGMSG( "check_all_NetUseable all end, usetime=%ums, count=%u, timeMax=%u", timeGetTime()-dwStart, thisCheckCount, timeMax );
}

// 
void GSIOTClient::check_all_devtime( const bool CheckNow )
{
	if( !IsRUNCODEEnable(defCodeIndex_SYS_CheckDevTimeCfg) )
	{
		return;
	}

	bool doCheck = CheckNow;

	static uint32_t s_last_check_check_all_devtime = timeGetTime();
	const uint32_t dwStart = timeGetTime();
	if( !doCheck )
	{
		if( dwStart-s_last_check_check_all_devtime > 30000 )
		{
			doCheck = true;
		}
	}

	if( !doCheck )
	{
		return;
	}

	s_last_check_check_all_devtime = dwStart;

	SYSTEMTIME st;
	memset( &st, 0, sizeof(st) );
	::GetLocalTime(&st);

	const int check_all_devtime_iday = RUNCODE_Get(defCodeIndex_SYS_CheckDevTimeSave,defRunCodeValIndex_1);

	if( !CheckNow && check_all_devtime_iday == st.wDay )
		return;

	const int checkRangeBegin = RUNCODE_Get(defCodeIndex_SYS_CheckDevTimeCfg,defRunCodeValIndex_2);
	const int checkRangeEnd = RUNCODE_Get(defCodeIndex_SYS_CheckDevTimeCfg,defRunCodeValIndex_3);

	// checkRangeBegin to checkRangeEnd 
	if( !CheckNow && (st.wHour<checkRangeBegin || st.wHour>=checkRangeEnd) )
		return;
	
	LOGMSG( "check_all_devtime: hourrange(%d-%d)%s", checkRangeBegin, checkRangeEnd, CheckNow?" CheckNow!":"" );

	this->m_RunCodeMgr.SetCodeAndSaveDb( defCodeIndex_SYS_CheckDevTimeSave, st.wDay );

	PlayMgrCmd_SetDevtimeNow();
}

//
void GSIOTClient::check_all_devtime_proc()
{
	std::set<int> NeedIDList; // 

	{
		gloox::util::MutexGuard mutexguard( m_mutex_PlayMgr );
		NeedIDList.swap( m_check_all_devtime_NeedIDList );
	}

	if( NeedIDList.empty() )
	{
		return;
	}

	LOGMSG( "check_all_devtime_proc: start" );

	const uint32_t dwStart = timeGetTime();
	uint32_t thisCheckCount = 0;
	char buf[64] = {0};

	struGSTime newtime;
	g_struGSTime_GetCurTime( newtime );
	uint32_t newtime_lastget = timeGetTime();

	while( !NeedIDList.empty() )
	{
		std::set<int>::const_iterator itNeed = NeedIDList.begin();
		const int id = *itNeed;
		NeedIDList.erase(itNeed);
		thisCheckCount++;
		
		GSIOTDevice *pDev = this->ipcamClient->GetIOTDevice(id);
		if( pDev && pDev->GetEnable() )
		{
			IPCameraBase *cam = (IPCameraBase*)pDev->getControl();
			if(cam)
			{
				// 
				//const std::string key = std::string(cam->GetIPAddress()) + itoa( cam->GetPort(), buf, 10 );
				sprintf(buf, "%d", cam->GetPort());
				const std::string key = std::string(cam->GetIPAddress()) + buf; //jyc20170405 modify
				if( !check_all_devtime_IsChecked(key) )
				{
					m_check_all_devtime_CheckedIPList.insert( key );
					g_reget_struGSTime_GetCurTime( newtime, newtime_lastget, timeGetTime() );

					cam->SetCamTime( newtime );
				}

				// 
				if( !cam->isGetSelf(true) )
				{
					const std::string strip = cam->ConnUse_ip(true);
					const uint32_t port = cam->ConnUse_port(true);
					//const std::string key = strip + itoa( port, buf, 10 );
					sprintf(buf, "%d", port);
					const std::string key = strip + buf; //jyc20170405 modify
					
					if( !check_all_devtime_IsChecked(key) )
					{
						m_check_all_devtime_CheckedIPList.insert( key );
						g_reget_struGSTime_GetCurTime( newtime, newtime_lastget, timeGetTime() );

						switch( cam->GetCameraType() )
						{
						case CameraType_hik: //jyc20170405 remove
							//HikCamera::SetCamTime_Spec( newtime, pDev->getName(), (char*)strip.c_str(), port, cam->ConnUse_username(true), cam->ConnUse_password(true) );
							break;

						default:
							break;
						}
					}
				}
			}
		}

		if( timeGetTime()-dwStart > 2000 )
		{
			LOGMSG( "check_all_devtime_proc: long time=%ums, count=%u", timeGetTime()-dwStart, thisCheckCount );
			return;
		}
	}

	LOGMSG( "check_all_devtime_proc: end all, usetime=%ums, count=%u\r\n", timeGetTime()-dwStart, thisCheckCount );
}

bool GSIOTClient::check_all_devtime_IsChecked( const std::string &key )
{
	std::set<std::string>::const_iterator itIP = m_check_all_devtime_CheckedIPList.find( key );

	return ( itIP != m_check_all_devtime_CheckedIPList.end() );
}

bool GSIOTClient::hasNetUseableFailed() const
{
	const std::list<GSIOTDevice*> &cameraList = this->ipcamClient->GetCameraManager()->GetCameraList();
	std::list<GSIOTDevice*>::const_iterator it = cameraList.begin();
	for( ; it!=cameraList.end(); ++it )
	{
		IPCameraBase *cam = (IPCameraBase*)(*it)->getControl();
		if(cam)
		{
			if( (*it)->GetEnable() )
			{
				if( defUseable_OK != cam->get_NetUseable() )
					return false;
			}
		}
	}

	return true;
}

//jyc20160919 notice
GSAGCurState_ GSIOTClient::GetAlarmGuardCurState() const
{
	if( !GetAlarmGuardGlobalFlag() )
	{
		return GSAGCurState_UnArmed;
	}

	const GSAGCurState_ curevt = GetAllEventsState();

	if( GSAGCurState_UnArmed == curevt )
	{
		return GSAGCurState_UnArmed;
	}
	else if( g_IsValidCurTimeInAlarmGuardState() )
	{
		return curevt;
	}

	return GSAGCurState_WaitTimeArmed;
}

// guard global flag
int GSIOTClient::GetAlarmGuardGlobalFlag() const
{
	return RUNCODE_Get(defCodeIndex_SYS_AlarmGuardGlobalFlag);
}

void GSIOTClient::SetAlarmGuardGlobalFlag( int flag ) //for camera
{
	this->m_RunCodeMgr.SetCodeAndSaveDb( defCodeIndex_SYS_AlarmGuardGlobalFlag, flag );	
	//jyc20170331 resume
	const std::list<GSIOTDevice*> devices = ipcamClient->GetCameraManager()->GetCameraList();
	for( std::list<GSIOTDevice*>::const_iterator it=devices.begin(); it!=devices.end(); ++it )
	{
		GSIOTDevice* dev = (*it);
		if( IOT_DEVICE_Camera != dev->getType() )
			continue;

		IPCameraBase *cam = (IPCameraBase*)dev->getControl();
		if( cam )
		{
			cam->SetCurAlarmState( defAlarmState_UnInit );
		}
	}
}

std::string GSIOTClient::GetSimpleInfo( const GSIOTDevice *const iotdevice )
{
	if( !iotdevice )
	{
		return std::string("");
	}

	std::string str;

	switch( iotdevice->getType() )
	{
	case IOT_DEVICE_Camera:
		{   //jyc20170331 resume
			if( iotdevice->getControl() )
			{
				IPCameraBase *cam = (IPCameraBase*)iotdevice->getControl();
				char buf[64] = {0};

				str = cam->GetIPAddress();
				str += ":";
				//str += itoa( cam->GetPort(), buf, 10 );
				str += snprintf(buf, sizeof(buf), "%d", cam->GetPort());
				str += ":";
				//str += itoa( cam->GetChannel(), buf, 10 );
				str += snprintf(buf, sizeof(buf), "%d", cam->GetChannel());

				if( IsRUNCODEEnable( defCodeIndex_SYS_AutoPublishEnable ) )
				{
					if( cam->GetAdvAttr().get_AdvAttr( defCamAdvAttr_AutoPublish ) )
					{
						str += ", PublishEnable"; //chinese to english
					}
				}

				if( cam->GetAdvAttr().get_AdvAttr( defCamAdvAttr_AutoConnect ) )
				{
					str += ", AutoConnect";
				}

				if( cam->GetAdvAttr().get_AdvAttr( defCamAdvAttr_SupportAlarm ) )
				{
					str += ", SupportAlarm";
				}

				if( defRecMod_NoRec == cam->GetRecCfg().getrec_mod() )
				{
					str += ", no videotape";
				}

				if( defAudioSource_Null != cam->GetAudioCfg().get_Audio_Source() )
				{
					str += ", ";
					str += CAudioCfg::getstr_AudioSource( cam->GetAudioCfg().get_Audio_Source() ).c_str();
				}

				if( 0 != cam->GetStreamfmt() )
				{
					str += ", Streamfmt";
					//str += itoa( cam->GetStreamfmt(), buf, 10 );
					str += snprintf(buf, sizeof(buf), "%d", cam->GetStreamfmt());
				}

				if( 2000 != cam->getBufferTime() )
				{
					str += ", videobufpara=";
					//str += itoa( cam->getBufferTime(), buf, 10 );
					str += snprintf(buf, sizeof(buf), "%d", cam->getBufferTime());
				}
			}
		}
		break;

	case IOT_DEVICE_RS485:
		{
			RS485DevControl *ctl = (RS485DevControl*)iotdevice->getControl();

			if( ctl )
			{
				const defAddressQueue& que = ctl->GetAddressList();
				char buf[64] = {0};

				std::string strobjname;
				uint32_t enableObjCount = 0;
				defAddressQueue::const_iterator it = que.begin();
				defAddressQueue::const_iterator itEnd = que.end();
				for( ; it!=itEnd; ++it )
				{
					DeviceAddress *pCurAddr = *it;
					if( !pCurAddr->GetEnable() )
						continue;

					enableObjCount++;

					strobjname += pCurAddr->GetObjName();
					strobjname += ", ";

					if( strobjname.size() > 255 )
						break;
				}

				str = "485 addr:";
				//str += itoa( ctl->GetDeviceid(), buf, 10 );
				sprintf(buf, "%d", ctl->GetDeviceid()); //jyc20170515 modify
				str += buf;
				str += ", child dev";
				//str += itoa( enableObjCount, buf, 10 );
				sprintf(buf, "%d", enableObjCount); //jyc21070515 modify
				str += buf;
				str += " ; ";
				str += strobjname;
			}
		}
		break;

	case IOT_DEVICE_Remote:
		{
			RFRemoteControl *pctl = (RFRemoteControl*)iotdevice->getControl();
			if( pctl )
			{
				const defButtonQueue& que = pctl->GetButtonList();
				char buf[64] = {0};

				std::string strobjname;
				const uint32_t enableObjCount = pctl->GetEnableCount();
				defButtonQueue::const_iterator it = que.begin();
				defButtonQueue::const_iterator itEnd = que.end();
				for( ; it!=itEnd; ++it )
				{
					RemoteButton *pCurButton = *it;
					if( !pCurButton->GetEnable() )
						continue;
					
					strobjname += pCurButton->GetObjName();
					strobjname += ", ";

					if( strobjname.size() > 255 )
						break;
				}

				const std::string cfgdesc = pctl->GetCfgDesc();

				if( cfgdesc.empty() )
				{
					//str += itoa( enableObjCount, buf, 10 );
					sprintf(buf, "%d", enableObjCount);
					str += buf;
					str += " button; ";
				}
				else
				{
					str += cfgdesc;
					str += "; ";
					//str += itoa( enableObjCount, buf, 10 );
					sprintf(buf, "%d", enableObjCount);
					str += buf;
					str += " obj; ";
				}

				str += strobjname;
			}
		}
		break;

	case IOT_DEVICE_Trigger:
		return GetSimpleInfo_ForSupportAlarm( iotdevice );

	default:
		break;
	}

	return str;
}

// jyc20160919
std::string GSIOTClient::GetSimpleInfo_ForSupportAlarm( const GSIOTDevice *const iotdevice )
{
	if( !iotdevice )
	{
		return std::string("");
	}

	std::string str;

	switch( iotdevice->getType() )
	{
	case IOT_DEVICE_Camera:
	case IOT_DEVICE_Trigger:
		{
			int count_Force = 0;

			int count_SMS_Event = 0;
			int count_EMAIL_Event = 0;
			int count_NOTICE_Event = 0;
			int count_CONTROL_Event = 0;
			int count_Eventthing_Event = 0;
			int count_CALL_Event = 0;

			std::list<ControlEvent*> &eventList = this->GetEvent()->GetEvents();
			std::list<ControlEvent*>::const_iterator it = eventList.begin();
			for( ; it!=eventList.end(); ++it )
			{
				if( !(*it)->GetEnable() )
					continue;

				if( (*it)->GetDeviceType() == iotdevice->getType() && (*it)->GetDeviceID() == iotdevice->getId() )
				{
					if( (*it)->isForce() )
					{
						count_Force++;
					}

					switch( (*it)->GetType() )
					{
					case SMS_Event:			count_SMS_Event++;			break;
					case EMAIL_Event:		count_EMAIL_Event++;		break;
					case NOTICE_Event:		count_NOTICE_Event++;		break;
					case CONTROL_Event:		count_CONTROL_Event++;		break;
					case Eventthing_Event:	count_Eventthing_Event++;	break;
					case CALL_Event:		count_CALL_Event++;			break;
					}
				}
			}

			char buf[64] = {0};
			if( count_Force>0 )				{ sprintf(buf, "%d", count_Force);				str +=buf; str += "always do; "; }
			if( count_SMS_Event>0 )			{ sprintf(buf, "%d", count_SMS_Event);			str +=buf; str += " "; str += GSIOTEvent::GetEventTypeToString(SMS_Event);		str += ", "; }
			if( count_EMAIL_Event>0 )		{ sprintf(buf, "%d", count_EMAIL_Event);		str +=buf; str += " "; str += GSIOTEvent::GetEventTypeToString(EMAIL_Event);		str += ", "; }
			if( count_NOTICE_Event>0 )		{ sprintf(buf, "%d", count_NOTICE_Event);		str +=buf; str += " "; str += GSIOTEvent::GetEventTypeToString(NOTICE_Event);		str += ", "; }
			if( count_CONTROL_Event>0 )		{ sprintf(buf, "%d", count_CONTROL_Event);		str +=buf; str += " "; str += GSIOTEvent::GetEventTypeToString(CONTROL_Event);	str += ", "; }
			if( count_Eventthing_Event>0 )	{ sprintf(buf, "%d", count_Eventthing_Event);	str +=buf; str += " "; str += GSIOTEvent::GetEventTypeToString(Eventthing_Event);	str += ", "; }
			if( count_CALL_Event>0 )		{ sprintf(buf, "%d", count_CALL_Event);			str +=buf; str += " "; str += GSIOTEvent::GetEventTypeToString(CALL_Event);		str += ", "; }
		}
		break;

	default:
		break;
	}

	return str;
}



std::string GSIOTClient::getstr_ForRelation( const deflstRelationChild &ChildList )
{
	std::string str;

	for( deflstRelationChild::const_iterator it=ChildList.begin(); it!=ChildList.end(); ++it )
	{
		GSIOTDevice *dev = this->GetIOTDevice( it->child_dev_type, it->child_dev_id );
		if( !dev )
			continue;

		if( !str.empty() )
		{
			str += ", ";
		}
		
		str += dev->getName();
	}

	return str;
}

defGSReturn GSIOTClient::SendControl( const IOTDeviceType DevType, const GSIOTDevice *device, const GSIOTObjBase *obj, const uint32_t overtime, const uint32_t QueueOverTime, const uint32_t nextInterval, const bool isSync )
{
	if( device && IOT_DEVICE_Remote==DevType )
	{
		switch( device->getExType() )
		{
		case IOTDevice_AC_Ctl:
		{
			if( isSync )
			{
				const GSRemoteCtl_AC *acctl = (GSRemoteCtl_AC*)device->getControl();
				if( acctl && defFactory_ZK == acctl->get_factory() )
				{
					if( m_IGSMessageHandler )
					{
						return m_IGSMessageHandler->OnControlOperate( defCtrlOprt_SendControl, device->getExType(), device, obj );
					}

					return defGSReturn_Err;
				}

				return defGSReturn_UnSupport;
			}

			GSIOTDevice *sendDev = device->clone( false );
			RFRemoteControl *sendctl = (RFRemoteControl*)sendDev->getControl();
			if( obj ) sendctl->ButtonQueueChangeToOne( obj->GetId() );

			RFRemoteControl *ctl = (RFRemoteControl*)device->getControl();
			sendctl->SetCmd( ctl->GetCmd() );

			PushControlMesssageQueue( new ControlMessage( JID(), "", sendDev, obj?sendctl->GetButton( obj->GetId() ):NULL, QueueOverTime ) );
			return defGSReturn_SuccExecuted;
		}
		break;
		
		case IOTDevice_Combo_Ctl:
		{
			struGSTime curdt;
			g_struGSTime_GetCurTime( curdt );
			this->DoAlarmDevice( device, true, 1, true, device->GetStrAlmBody( true, curdt ), device->GetStrAlmSubject( true ) );
			break;
		}

		default:
			break;
		}
	}

	return this->deviceClient->SendControl( DevType, device, obj, overtime, QueueOverTime, nextInterval );
}




