#include "gloox/message.h" //jyc20170222 add

#include "gloox/rostermanager.h" //jyc20170224 add

#include "GSIOTClient.h"
#include "GSIOTInfo.h"
#include "GSIOTDevice.h"
#include "GSIOTControl.h"
#include "GSIOTDeviceInfo.h"
#include "GSIOTHeartbeat.h"

#include "RS485DevControl.h"
#include "RFRemoteControl.h"

#include "XmppGSResult.h"
#include "AutoEventthing.h" //jyc20170222 add

#include "XmppGSMessage.h"
#include "XmppRegister.h"
#include "XmppGSChange.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>   

#include <pthread.h> //jyc20160922

#define defcheckNetUseable_timeMax (5*60*1000)

//#define defForceDataSave  //jyc20170228 debug

static int sg_blUpdatedProc = 0;  //jyc20170318 resume

namespace httpreq  //jyc20170318 resume
{
	#include "HttpRequest.cpp"
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
	//return 0;
}


GSIOTClient::GSIOTClient( IDeviceHandler *handler, const std::string &RunParam )
	: m_parser(this), m_PreInitState(false), m_cfg(NULL), m_event(NULL), timer(NULL), deviceClient(NULL),
	xmppClient(NULL),timeCount(0),serverPingCount(0), m_handler(handler), m_running(false), 
	m_IGSMessageHandler(NULL), m_ITriggerDebugHandler(NULL), 
	m_EnableTriggerDebug(false),m_last_checkNetUseable_camid(0)
{
	g_SYS_SetGSIOTClient( this ); //jyc20160826
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

	//20160823 voice delete
	//APlayer::Init();
	//this->m_TalkMgr.set_ITalkNotify( this );

	m_PreInitState = true;
	m_isThreadExit = true;
	//m_isPlayBackThreadExit = true;
	//m_isPlayMgrThreadExit = true;
	//m_PlaybackThreadTick = timeGetTime();
	//m_PlaybackThreadCreateCount = 0;

	//m_PlayMgr_CheckNowFlag = defPlayMgrCmd_Unknown;

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
			//&& m_isPlayBackThreadExit
			//&& m_isPlayMgrThreadExit
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
	   //xmppClient->removeStanzaExtension(ExtIotTalk);
	   //xmppClient->removeIqHandler(this, ExtIotTalk);
	   //xmppClient->removeStanzaExtension(ExtIotPlayback);
	   //xmppClient->removeIqHandler(this, ExtIotPlayback);
	   xmppClient->removeStanzaExtension(ExtIotRelation);
	   xmppClient->removeIqHandler(this, ExtIotRelation);
	   //xmppClient->removeStanzaExtension(ExtIotPreset);
	   //xmppClient->removeIqHandler(this, ExtIotPreset);
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
	
	//PlaybackCmd_clean();
	//Playback_DeleteAll();

	//if(ipcamClient){
	//	delete(ipcamClient);
	//}

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
			//for( ; it!=itEnd ; ++it )  //jyc20170303 notice 10s
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
				if( pCurAddr )  //jyc20170303 remove 
				{
					bool isChanged = false;
					pCurCtl->check_NetUseable_RecvFailed( &isChanged );

					if( isChanged && m_handler )
					{
						//m_handler->OnDeviceNotify( defDeviceNotify_StateChanged, pCurDev, pCurAddr );
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
		return defUseable_OK;//return deviceClient->GetPortState()>0 ? defUseable_OK:defUseable_Err;
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
				/* //jyc20170305 debug
				if(pCurDev->getId()==2&&pCurDev->getType()==12){
					printf("test4 id=%d name=%s getlinkid=%d  linkid=%d......\n",
					       pCurDev->getId(),pCurDev->getName().c_str(),pCurDev->GetLinkID(),LinkID);
					printf("pctllinkid=%d ctllinkid=%d..........\n",
					       pCurCtl->GetLinkID(),ctl->GetLinkID());
					printf("pctlid=%d ctlid=%d..........\n",
					       ((RS485DevControl*)pCurCtl)->GetDeviceid(),((RS485DevControl*)ctl)->GetDeviceid());
				}*/

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
				printf( "NoticeJid is invalid! no noticejid\r\n" ); //jyc20160919
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

				case IOT_DEVICE_Camera:
					{

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

			case IOT_DEVICE_Camera:
				{
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

		case IOT_DEVICE_Camera:
			{
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
	/*jyc20160919
	devices = ipcamClient->GetCameraManager()->GetCameraList();
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

		case IOT_DEVICE_Camera:
			{
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
	}//*/

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
	/*jyc20160922*/
	//SQLite::Database *db = isEditCam ? ipcamClient->GetCameraManager()->get_db() : deviceClient->GetDeviceManager()->get_db();
	SQLite::Database *db = deviceClient->GetDeviceManager()->get_db();
	UseDbTransAction dbta(db);

	//const std::list<GSIOTDevice*> devices = isEditCam ? ipcamClient->GetCameraManager()->GetCameraList() : deviceClient->GetDeviceManager()->GetDeviceList();
	const std::list<GSIOTDevice*> devices = deviceClient->GetDeviceManager()->GetDeviceList();

	
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

		case IOT_DEVICE_Camera:
			{

			}
			break;

		default:
			continue;
		}

		if( isEditCam )
		{
			//ipcamClient->ModifyDevice( dev ); //jyc20160922
		}
		else
		{
			deviceClient->ModifyDevice( dev );
		}
	}//*/
}

void GSIOTClient::SetITriggerDebugHandler( ITriggerDebugHandler *handler )
{
	this->m_ITriggerDebugHandler = handler; //jyc20160919
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
					//handleIq_Set_XmppGSPreset( pMsg );
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

	//XmppPrint( iq, "test recv.......\n" );  //jyc20170227 debug recv message

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

				XmppGSRelation *pExXmppGSRelation = (XmppGSRelation*)iq.findExtension(ExtIotRelation);
				if( pExXmppGSRelation )
				{
					handleIq_Get_XmppGSRelation( pExXmppGSRelation, iq, pUser );
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
				/*jyc20170306 remove
				XmppGSTrans *pExXmppGSTrans = (XmppGSTrans*)iq.findExtension( ExtIotTrans );
				if( pExXmppGSTrans )
				{
					handleIq_Get_XmppGSTrans( pExXmppGSTrans, iq, pUser );
					return true;
				}
				*/
				
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
				if(heartbeat){
					printf("heartbeat\n");
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

	//const std::string DefaultNoticeJid = m_cfg->GetNoticeJid();
	//if( !DefaultNoticeJid.empty() )
	//{
	//	GSIOTUser *pUser = m_cfg->m_UserMgr.GetUser( DefaultNoticeJid );

	//	if( !pUser->get_UserFlag(defUserFlag_NoticeGroup) )
	//	{
	//		pUser->set_UserFlag( defUserFlag_NoticeGroup, true );
	//	}
	//}

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
	{
		if( IOT_DEVICE_Camera == pDeviceSrc->getType() )
		{
			
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
				//ipcamClient->ModifyDevice( pLocalDevice );
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
				//return edit_CamDevControl( pLocalDevice, pDeviceSrc );
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
						{
							
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
		return std::string("未注册服务");
	}

	switch( xmppClient->state() )
	{
	case StateDisconnected:
		return std::string("连接中断");

	case StateConnecting:
		return std::string("连接中");

	case StateConnected:
		return std::string("正常");

	default:
		break;
	}

	return std::string("");
}

void GSIOTClient::Run() //jyc20160826
{
	printf( "GSIOTClient::Run()\r\n" );

	LoadConfig();

	//init devices
	deviceClient = new DeviceConnection(this);
	deviceClient->Run(m_cfg->getSerialPort());

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
	timer->registerTimer(this,2,2);		// 通知检测
	timer->registerTimer(this,3,15);	// 回放检测
	timer->registerTimer(this,4,60);	// 间隔发ping
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
	xmppClient->disco()->addFeature(XMLNS_GSIOT_RELATION);
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
	xmppClient->registerStanzaExtension(new XmppGSRelation(NULL));
	xmppClient->registerStanzaExtension(new XmppGSVObj(NULL));
	xmppClient->registerStanzaExtension(new XmppGSReport(NULL));
	xmppClient->registerStanzaExtension(new XmppGSMessage(NULL));
	xmppClient->registerStanzaExtension(new XmppGSUpdate(NULL)); //jyc20170319 add	
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
	xmppClient->registerIqHandler(this, ExtIotRelation);
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

	DataProc_ThreadCreate(); 
	//AlarmProc_ThreadCreate(); //jyc20160922 camera alarm  //jyc20170222 no use
	
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
	    xmppClient->connect(); // 阻塞式连接
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
	//jyc20160823
	if( IOT_DEVICE_Camera == deviceType ){
	//	return ipcamClient->GetIOTDevice( deviceId );
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
		{					

		}
		break;
	}

	return std::string("");
}

bool GSIOTClient::DeleteDevice( GSIOTDevice *iotdevice )
{
	this->m_event->DeleteDeviceEvent( iotdevice->getType(), iotdevice->getId() );

	if( IOT_DEVICE_Camera == iotdevice->getType() )
	{
		//return this->ipcamClient->RemoveIPCamera( iotdevice );
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
	{
		//ipcamClient->ModifyDevice( iotdevice );
	}
	else
	{
		deviceClient->ModifyDevice( iotdevice );
	}

	return true;
}

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


void GSIOTClient::DataProc_ThreadCreate()
{
	if( IsRUNCODEEnable(defCodeIndex_Dis_RunDataProc) )
	{
		return ;
	}

	m_isDataProcThreadExit = false;

	//HANDLE   hth1;
	//unsigned  uiThread1ID;
	//hth1 = (HANDLE)_beginthreadex( NULL, 0, DataProcThread, this, 0, &uiThread1ID );
	//CloseHandle(hth1);

	pthread_t id_1;  
    int i,ret;  

    ret=pthread_create(&id_1, NULL, DataProc_Thread, this );  
    if(ret!=0)  
    {  
        printf("Create DataProcThread error!\n");  
		//return -1;  
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
    int i,ret;  

    ret=pthread_create(&id_1, NULL, AlarmProc_Thread, this );  
    if(ret!=0)  
    {  
        printf("Create DataProcThread error!\n");  
		//return -1;  
		return; 
    } 
}

//
bool GSIOTClient::AlarmProc()
{
	const DWORD dwStart = ::timeGetTime();
	bool isdo = true;
	//jyc20160922 delete all about camera
	return isdo;
}

// 
bool GSIOTClient::AlarmCheck()
{
	bool isChanged = false;

	//jyc20160922 delete , all about camera 
	return isChanged;
}

//jyc20160919
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
}

// 
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
		{
			
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
				str += sprintf(buf, "%d", ctl->GetDeviceid());
				str += ", child dev";
				//str += itoa( enableObjCount, buf, 10 );
				str += sprintf(buf, "%d", enableObjCount);
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
					str += sprintf(buf, "%d", enableObjCount);
					str += " button; ";
				}
				else
				{
					str += cfgdesc;
					str += "; ";
					//str += itoa( enableObjCount, buf, 10 );
					str += sprintf(buf, "%d", enableObjCount);
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
			if( count_Force>0 )				{ str += sprintf(buf, "%d", count_Force);				str += "always do; "; }
			if( count_SMS_Event>0 )			{ str += sprintf(buf, "%d", count_SMS_Event);			str += " "; str += GSIOTEvent::GetEventTypeToString(SMS_Event);		str += ", "; }
			if( count_EMAIL_Event>0 )		{ str += sprintf(buf, "%d", count_EMAIL_Event);			str += " "; str += GSIOTEvent::GetEventTypeToString(EMAIL_Event);		str += ", "; }
			if( count_NOTICE_Event>0 )		{ str += sprintf(buf, "%d", count_NOTICE_Event);		str += " "; str += GSIOTEvent::GetEventTypeToString(NOTICE_Event);		str += ", "; }
			if( count_CONTROL_Event>0 )		{ str += sprintf(buf, "%d", count_CONTROL_Event);		str += " "; str += GSIOTEvent::GetEventTypeToString(CONTROL_Event);	str += ", "; }
			if( count_Eventthing_Event>0 )	{ str += sprintf(buf, "%d", count_Eventthing_Event);	str += " "; str += GSIOTEvent::GetEventTypeToString(Eventthing_Event);	str += ", "; }
			if( count_CALL_Event>0 )		{ str += sprintf(buf, "%d", count_CALL_Event);			str += " "; str += GSIOTEvent::GetEventTypeToString(CALL_Event);		str += ", "; }
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




