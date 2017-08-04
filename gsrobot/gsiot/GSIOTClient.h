#ifndef GSIOTCLIENT_H
#define GSIOTCLIENT_H

#include "GSIOTConfig.h"
#include "GSIOTEvent.h"
#include "../IDeviceHandler.h"
#include "../DeviceConnection.h"
#include "../IPCamConnection.h"  //jyc20170322 resume
#include "../ControlBase.h"

#include "gloox/adhoccommandprovider.h" //jyc20170224 modify
#include "gloox/disco.h"
#include "gloox/adhoc.h"#include "gloox/tag.h"
#include "gloox/loghandler.h"
#include "gloox/logsink.h"
#include "gloox/message.h"
#include "gloox/util.h"

#include "gloox/gloox.h"
#include "gloox/client.h"
#include "gloox/connectionlistener.h"
#include "gloox/iqhandler.h"
#include "gloox/subscriptionhandler.h"
#include "gloox/messagehandler.h"
#include "GSIOTDevice.h"
#include "../ITimerHandler.h"
#include "../TimerManager.h"
#include "../ControlMessage.h"
#include "../xmpp/XmppGSAuth.h"
#include "../xmpp/XmppGSManager.h"
#include "../xmpp/XmppGSEvent.h"
#include "../xmpp/XmppGSState.h"
#include "../xmpp/XmppGSTalk.h" //jyc20170330 resume
#include "../xmpp/XmppGSPlayback.h" //jyc20170405 resume
#include "../xmpp/XmppGSRelation.h"
#include "../xmpp/XmppGSPreset.h"
#include "../xmpp/XmppGSVObj.h"
//#include "../xmpp/XmppGSTrans.h"
#include "../xmpp/XmppGSReport.h"
#include "../xmpp/XmppGSUpdate.h"
#include "../IGSMessageHandler.h"
//#include "Parser.h"  
#include "../GSRemoteTalkMgr.h"
#include "../RunCode.h"
#include "../DataStoreMgr.h"

namespace httpreq
{
	#include "../HttpRequest.h"
}


#define LOGMSG printf

// jyc20170405 resume
struct struCamAlarmRecv_key
{
	std::string sDeviceIP;		// 
	int nPort;					// 
	int channel;

	struCamAlarmRecv_key( const char *in_sDeviceIP, const int in_nPort, const int in_channel )
		: nPort(in_nPort), channel(in_channel)
	{
		if( in_sDeviceIP )
		{
			sDeviceIP = in_sDeviceIP;
		}
	}

	bool operator == ( const struCamAlarmRecv_key &other ) const
	{
		if( other.sDeviceIP != this->sDeviceIP )
			return false;

		if( other.channel != this->channel )
			return false;

		if( other.nPort != this->nPort )
			return false;

		return true;
	}
};

// 
struct struCamAlarmRecv
{
	uint32_t ts;				// 
	struGSTime dt;				// 
	struCamAlarmRecv_key key;	// key
	IPCameraType CameraType;	// 
	std::string alarmstr;		// 

	struCamAlarmRecv()
		: ts(0), key(NULL,0,defInvalidCamCh), CameraType(CameraType_Unkown)
	{
		memset( &dt, 0, sizeof(struGSTime) );
	}

	struCamAlarmRecv( const uint32_t in_ts, const struCamAlarmRecv_key &in_key, const IPCameraType in_CameraType, const char *in_alarmstr )
		:ts(in_ts), key(in_key), CameraType(in_CameraType)
	{
		memset( &dt, 0, sizeof(struGSTime) );

		if( in_alarmstr )
		{
			alarmstr = in_alarmstr;
		}
	}

	struCamAlarmRecv( const uint32_t in_ts, const char *in_sDeviceIP, const int in_nPort, const int in_channel, const IPCameraType in_CameraType, const char *in_alarmstr )
		: ts(in_ts), key(in_sDeviceIP, in_nPort, in_channel), CameraType(in_CameraType)
	{
		memset( &dt, 0, sizeof(struGSTime) );

		if( in_alarmstr )
		{
			alarmstr = in_alarmstr;
		}
	}

	bool isOverTime( const uint32_t curts ) const
	{
		if( curts-ts > 5*60*1000 )
		{
			return true;
		}

		return false;
	}
};
typedef std::list<struCamAlarmRecv> deflstCamAlarmRecv;// 


// 
struct struPlaybackCmd
{
	XmppGSPlayback *pXmpp;
	JID from_Jid;
	std::string from_id;
	uint32_t timestamp;

	struPlaybackCmd()
		: pXmpp(NULL)
	{
		timestamp = timeGetTime();
	}

	struPlaybackCmd( const JID &in_from_Jid, const std::string &in_from_id, const XmppGSPlayback *in_pXmpp )
		: from_Jid(in_from_Jid), from_id(in_from_id)
	{
		pXmpp = (XmppGSPlayback*)in_pXmpp->clone();
		timestamp = timeGetTime();
	}
};

// 
struct struPlaybackSession
{
	uint32_t ts;
	uint32_t lastUpdateTS;
	std::string key;
	std::string from_jid;
	std::string url;
	std::string peerid;
	std::string streamid;
	std::string devname;
	GSIOTDevice *dev;

	struPlaybackSession();
	struPlaybackSession( const std::string &in_key, const std::string &in_from_jid, const std::string &in_url, const std::string &in_peerid, const std::string &in_streamid, const std::string &in_devname, GSIOTDevice *in_dev );

	bool check() const;
};

enum defPlayMgrCmd_
{
	defPlayMgrCmd_Unknown	= 0,
	defPlayMgrCmd_Start		= 1,
	defPlayMgrCmd_Stop		= 2,
	
	defPlayMgrCmd_CheckNow	= 91,
};
struct struPlayMgrCmd
{
	uint32_t timestamp;
	defPlayMgrCmd_ cmd;
	defUserAuth Auth;
	JID from_Jid;
	std::string from_id;
	int dev_id;
	std::string url;
	std::vector<std::string> url_backup; // 

	struPlayMgrCmd()
		: cmd(defPlayMgrCmd_Unknown), Auth(defUserAuth_Null), dev_id(0)
	{
		timestamp = timeGetTime();
	}

	struPlayMgrCmd( defPlayMgrCmd_ in_cmd, defUserAuth in_Auth, const JID &in_from_Jid, const std::string &in_from_id, const int in_dev_id, const std::string &in_url, const std::vector<std::string> &in_url_backup )
		: cmd(in_cmd), Auth(in_Auth), from_Jid(in_from_Jid), from_id(in_from_id), dev_id(in_dev_id), url(in_url), url_backup(in_url_backup)
	{
		timestamp = timeGetTime();
	}
};
// jyc20170405 resume until here

struct struEventNoticeMsg
{
	std::string id;
	uint32_t starttime;
	JID to_jid;
	std::string strSubject;
	std::string strBody;
	std::string callinfo;

	struEventNoticeMsg()
		: starttime(0)
	{
	}

	struEventNoticeMsg( const std::string &in_id, uint32_t in_starttime, const JID &in_to_jid, const std::string &in_strSubject, const std::string &in_strBody, const char *in_callinfo )
		: id(in_id), starttime(in_starttime), to_jid(in_to_jid), strSubject(in_strSubject), strBody(in_strBody), callinfo(in_callinfo)
	{
	}
};

using namespace gloox;

std::string g_IOTGetVersion();
std::string g_IOTGetBuildInfo();

//jyc20170510 add public ITalkNotify, public ICameraAlarmRecv
class GSIOTClient: public IGSClientExFunc, public IDeviceHandler, IqHandler, SubscriptionHandler, ConnectionListener,
	MessageHandler, ITimerHandler, public TagHandler ,public ITalkNotify//, public ICameraAlarmRecv
{
private:
	GSIOTConfig *m_cfg;
	GSIOTEvent *m_event;
	gloox::util::Mutex m_mutex_ctlMessageList;
	std::list<ControlMessage *> ctlMessageList;
	std::list<GSIOTDevice *> IotDeviceList;
	DeviceConnection *deviceClient;
	IPCamConnection *ipcamClient; //jyc20170322 resume 
	gloox::Client *xmppClient;
	TimerManager *timer;
	IDeviceHandler *m_handler;
	bool serverPingCount;
	int timeCount;
	//JID m_NoticeJid;
	IGSMessageHandler *m_IGSMessageHandler;
	ITriggerDebugHandler *m_ITriggerDebugHandler;
	bool m_EnableTriggerDebug;

	gloox::util::Mutex m_mutex_lstPlaybackList; //jyc20170405 resume
	std::map<std::string,struPlaybackSession> m_lstPlaybackList; // <key,value>
	std::list<struPlaybackCmd*> m_lstPlaybackCmd; // 

	gloox::util::Mutex m_mutex_PlayMgr; //jyc20170405 resume
	std::list<struPlayMgrCmd*> m_lstPlayMgrCmd; // 
	defPlayMgrCmd_ m_PlayMgr_CheckNowFlag;  // 

	gloox::util::Mutex m_mutex_lstGSMessage;
	std::list<GSMessage*> m_lstGSMessage;

	gloox::util::Mutex m_mutex_lstEventNoticeMsg;
	std::map<std::string,struEventNoticeMsg*> m_lstEventNoticeMsg; // <string id,value>

private:
	void Register();
	void OnDeviceStatusChange();

public:
	GSIOTClient( IDeviceHandler *handler, const std::string &RunParam );
	~GSIOTClient(void);

	static GSIOTDevice* CloneCamDev( const GSIOTDevice *src, IPCameraType destType=CameraType_Unkown );
	static GSIOTDevice* ClonePlaybackDev( const GSIOTDevice *src );
	static ControlBase* CloneControl( const ControlBase *src, bool CreateLock=true );
	static DeviceAddress* CloneDeviceAddress( const DeviceAddress *src );
	static bool Compare_Device( const GSIOTDevice *devA, const GSIOTDevice *devB );
	static bool Compare_Control( const ControlBase *ctlA, const ControlBase *ctlB );
	static bool Compare_Address( const DeviceAddress *AddrA, const DeviceAddress *AddrB );
	static bool Compare_GSIOTObjBase( const GSIOTObjBase *ObjA, const GSIOTObjBase *ObjB );
	static bool Compare_ControlAndAddress( const ControlBase *ctlA, const DeviceAddress *AddrA, const ControlBase *ctlB, const DeviceAddress *AddrB );

	void RunCodeInit();

	CRunCodeMgr m_RunCodeMgr;
	GSRemoteTalkMgr m_TalkMgr; //jyc20170330 resume

	void PreInit( const std::string &RunParam );
	bool GetPreInitState() const
	{
		return m_PreInitState;
	}
	
	void CheckSystem();
	void CheckIOTPs();
	void MemoryContainer_PrintState( bool printall, const char *pinfo );
	
	virtual CRunCodeMgr& get_RunCodeMgr()
	{
		return m_RunCodeMgr;
	}
	virtual void OnTimeOverForCmdRecv( const defLinkID LinkID, const IOTDeviceType DevType, 
	                                  const uint32_t DevID, const uint32_t addr );
	virtual defUseable get_all_useable_state_ForLinkID( defLinkID LinkID );

	void OnDeviceNotify( defDeviceNotify_ notify, GSIOTDevice *iotdevice, DeviceAddress *addr );
	void OnDeviceDisconnect(GSIOTDevice *iotdevice);
	void OnDeviceConnect(GSIOTDevice *iotdevice);
	void OnDeviceData( defLinkID LinkID, GSIOTDevice *iotdevice, ControlBase *ctl, GSIOTObjBase *addr );
	void OnDeviceData_ProcOne( defLinkID LinkID, GSIOTDevice *iotdevice, ControlBase *ctl, DeviceAddress *addr );
	//jyc20170405 resume have trouble
	virtual void* OnTalkNotify( const XmppGSTalk::defTalkCmd cmd, const std::string &url, const std::string &from_Jid, const std::string &from_id, bool isSyncReturn, const defvecDevKey &vecdev, bool result=true, IOTDeviceType getdev_type=IOT_DEVICE_Unknown, int getdev_id=0 );

	void DoAlarmDevice( const GSIOTDevice *iotdevice, const bool AGRunState, const int AlarmGuardGlobalFlag, const bool IsValidCurTime, const std::string &strAlmBody, const std::string &strAlmSubject );
	bool DoControlEvent( const IOTDeviceType DevType, const uint32_t DevID, ControlEvent *ctlevt, const bool isAlarm, const std::string &strAlmBody, const std::string &strAlmSubject, const char *callinfo, const bool isTest=false, const char *teststr=NULL );
	void DoControlEvent_Eventthing( const AutoEventthing *aEvt, const ControlEvent *ctlevt, const char *callinfo, const bool isTest );
	void DoControlEvent_Eventthing_Event( GSIOTDevice *dev, const AutoEventthing *aEvt, const ControlEvent *ctlevt, const char *callinfo, const bool isTest );
	GSAGCurState_ GetAllEventsState() const;
	void SetAllEventsState( const bool AGRunState, const char *callinfo, const bool forcesave );
private:
	void SetAllEventsState_do( const bool AGRunState, const bool forcesave, bool isEditCam );

public:
	GSAGCurState_ GetAlarmGuardCurState() const;

	int GetAlarmGuardGlobalFlag() const;
	void SetAlarmGuardGlobalFlag( int flag );

	void SetIGSMessageHandler( IGSMessageHandler *handler );
	void SetITriggerDebugHandler( ITriggerDebugHandler *handler );
	bool GetEnableTriggerDebugFlag() const
	{
		return m_EnableTriggerDebug;
	}
	void EnableTriggerDebug( bool enable )
	{
		m_EnableTriggerDebug = enable;
	}
	void AddGSMessage( GSMessage *pMsg );
	GSMessage* PopGSMessage();
	void OnGSMessageProcess();

	/*xmpp handler*/
	virtual void onConnect();
	virtual void onDisconnect( ConnectionError e );
	virtual bool onTLSConnect( const CertInfo& info );
	void handleIqID( const IQ& iq, int context );
	bool handleIq( const IQ& iq );
	void handleSubscription( const Subscription& subscription );
	void handleMessage( const Message& msg, MessageSession* session = 0 );
	
	void handleIq_Get_XmppGSAuth_User( const XmppGSAuth_User *pExXmppGSAuth_User, const IQ& iq, const GSIOTUser *pUser );
	void handleIq_Get_XmppGSAuth( const XmppGSAuth *pExXmppGSAuth, const IQ& iq, const GSIOTUser *pUser );
	void handleIq_Set_XmppGSAuth( const GSMessage *pMsg );
	void handleIq_Set_XmppGSManager( const GSMessage *pMsg );
	void handleIq_Set_XmppGSEvent( const GSMessage *pMsg );
	void handleIq_Set_XmppGSTalk( const XmppGSTalk *pXmpp, const IQ& iq, const GSIOTUser *pUser ); //jyc20170330 resume
	void handleIq_Set_XmppGSPlayback( const XmppGSPlayback *pXmpp, const IQ& iq, const GSIOTUser *pUser ); //jyc20170405 resume
	void handleIq_Set_XmppGSRelation( const GSMessage *pMsg );
	void handleIq_Get_XmppGSRelation( const XmppGSRelation *pXmpp, const IQ& iq, const GSIOTUser *pUser );
	void handleIq_Set_XmppGSPreset( const GSMessage *pMsg );
	void handleIq_Get_XmppGSPreset( const XmppGSPreset *pXmpp, const IQ& iq, const GSIOTUser *pUser );
	void handleIq_Get_XmppGSReport( const XmppGSReport *pXmpp, const IQ& iq, const GSIOTUser *pUser );
	void handleIq_Set_XmppGSVObj( const GSMessage *pMsg );
	void handleIq_Get_XmppGSVObj( const XmppGSVObj *pXmpp, const IQ& iq, const GSIOTUser *pUser );
	//void handleIq_Get_XmppGSTrans( const XmppGSTrans *pXmpp, const IQ& iq, const GSIOTUser *pUser );
	void handleIq_Set_XmppGSUpdate( const XmppGSUpdate *pXmpp, const IQ& iq, const GSIOTUser *pUser );

	// --------------------------
	// 处理网络过来的设备管理信息
public:
	bool add_GSIOTDevice( GSIOTDevice *pDeviceSrc );

	bool edit_GSIOTDevice( GSIOTDevice *pDeviceSrc );
private:
	bool edit_CamDevControl( GSIOTDevice *pLocalDevice, GSIOTDevice *pDeviceSrc );
	bool edit_RS485DevControl( GSIOTDevice *pLocalDevice, GSIOTDevice *pDeviceSrc );
	bool edit_RFRemoteControl( GSIOTDevice *pLocalDevice, GSIOTDevice *pDeviceSrc );

public:
	bool delete_GSIOTDevice( GSIOTDevice *pDeviceSrc );
	
	bool add_ControlEvent( ControlEvent *pSrc );
	bool edit_ControlEvent( ControlEvent *pSrc );
	bool delete_ControlEvent( ControlEvent *pSrc );

	// --------------------------
public:
	static void XmppPrint( const Tag *ptag, const char *callinfo, const Stanza *stanza, bool dodel=true );
	//static void XmppPrint( const Tag *ptag, const char *callinfo, const Stanza *stanza, bool dodel );
	static void XmppPrint( const IQ& iq, const char *callinfo );
	static void XmppPrint( const Message& msg, const char *callinfo );
	void XmppClientSend( const IQ& iq, const char *callinfo );
	void XmppClientSend_msg( const JID &to_jid, const std::string &strBody, const std::string &strSubject, const char *callinfo );
	void XmppClientSend_jidlist( const std::set<std::string> &jidlist, const std::string &strBody, const std::string &strSubject, const char *callinfo );

	/*timer*/
	void OnTimer( int TimerID );
	std::string GetConnectStateStr() const;

	void Run();
	void Connect();
	void Stop(void);
	bool CheckRegistered();
	void LoadConfig();
	bool SetJidToServer( const std::string &strjid, const std::string &strmac );
	void ResetNoticeJid();

	DeviceConnection *GetDeviceConnection() 
	{
		return this->deviceClient;
	}
	
	IPCamConnection *GetIPCameraConnection() //jyc20170405 resume
	{
		return this->ipcamClient;
	}

	GSIOTEvent *GetEvent()
	{
		return this->m_event;
	}

	GSIOTConfig *GetConfig()
	{
		return this->m_cfg;
	}

	const std::string& GetStartTime() const
	{
		return m_str_IOT_starttime;
	}
	
	GSIOTDevice* GetIOTDevice( IOTDeviceType deviceType, uint32_t deviceId ) const;
	std::string GetAddrObjName( const GSIOTAddrObjKey &AddrObjKey ) const;
	std::string GetDeviceAddressName( const GSIOTDevice *device, uint32_t address ) const;
	bool DeleteDevice( GSIOTDevice *iotdevice );
	bool ModifyDevice_Ver( GSIOTDevice *iotdevice, const std::string ver );

	void PushControlMesssageQueue( ControlMessage *pCtlMsg );
	bool CheckControlMesssageQueue( GSIOTDevice *device, DeviceAddress *addr, JID jid, std::string id );
	ControlMessage* PopControlMesssageQueue( GSIOTDevice *device, ControlBase *ctl, DeviceAddress *addr, IOTDeviceType specType=IOT_DEVICE_Unknown, IOTDeviceType specExType=IOT_DEVICE_Unknown );
	void CheckOverTimeControlMesssageQueue();
	void FinalClearControlMesssageQueue();

	// 录像回放命令队列处理  jyc20170405 resume
	static void PlaybackCmd_DeleteCmd( struPlaybackCmd *pCmd );
	void PlaybackCmd_push( const XmppGSPlayback *pXmpp, const IQ& iq );
	struPlaybackCmd* PlaybackCmd_pop();
	void PlaybackCmd_clean();
	bool PlaybackCmd_OnProc();
	void PlaybackCmd_ProcOneCmd( const struPlaybackCmd *pCmd );

	void Playback_ThreadCreate();
	void Playback_ThreadCheck();
	void Playback_ThreadPrinthb();

	// 录像回放回话队列处理
	static void Playback_DeleteDevOne( GSIOTDevice *device );
	bool Playback_IsLimit();
	uint32_t Playback_GetNowCount();
	void Playback_GetInfoList( std::map<std::string,struPlaybackSession> &getlstPlaybackList );
	bool Playback_Exist( const std::string &key );
	bool Playback_Add( const std::string &from_id, const std::string &key, const std::string &url, const std::string &peerid, const std::string &streamid, GSIOTDevice *device );
	void Playback_Delete( const std::string &key );
	void Playback_DeleteForJid( const std::string &from_jid );
	void Playback_DeleteAll();
	void Playback_SetForJid( const std::string &from_jid, int sound );
	GSPlayBackCode_ Playback_CtrlForJid( const std::string &from_jid, GSPlayBackCode_ ControlCode, void *pInBuffer = NULL, uint32_t InLen = 0, void *pOutBuffer = NULL, uint32_t *pOutLen = NULL );
	void Playback_CtrlResult( const JID &from_Jid, const std::string &from_id, const XmppGSPlayback *pXmppSrc, const GSPlayBackCode_ ControlCode, void *pOutBuffer = NULL, uint32_t *pOutLen = NULL );
	void Playback_UpdateSession( const std::string &key );
	void Playback_CheckSession();
	int PlayBackControl_GetCurState_test( GSPlayBackCode_ &curPB_Code, int &curPB_speedlevel, int &curPB_ThrowFrame );
	int PlayBackControl_test( GSPlayBackCode_ ControlCode, void *pInBuffer = NULL, uint32_t InLen = 0, void *pOutBuffer = NULL, uint32_t *pOutLen = NULL );

private:
	int PlayBackControl_nolock( IPCameraBase *pcam, GSPlayBackCode_ ControlCode, void *pInBuffer = NULL, uint32_t InLen = 0, void *pOutBuffer = NULL, uint32_t *pOutLen = NULL );

public:
	// 视频播放命令队列处理
	void PlayMgrCmd_push( defPlayMgrCmd_ cmd, defUserAuth Auth, const IQ& iq, const int dev_id, const std::string &url, const std::vector<std::string> &url_backup );
	void PlayMgrCmd_SetCheckNow( bool CheckNow=true );
	bool PlayMgrCmd_IsCheckNow();
	void PlayMgrCmd_SetDevtimeNow( IOTDeviceType type=IOT_DEVICE_All, int id=0 );
	void PlayMgrCmd_SetDevtimeNowForList( const std::set<int> &NeedIDList );
	struPlayMgrCmd* PlayMgrCmd_pop();
	void PlayMgrCmd_clean();
	bool PlayMgrCmd_OnProc();
	void PlayMgrCmd_ProcOneCmd( const struPlayMgrCmd *pCmd );
	void PlayMgrCmd_ThreadCreate();  //jyc20170405 resume until here

	bool EventNoticeMsg_Add( struEventNoticeMsg *msg );
	void EventNoticeMsg_Remove( const std::string &id );
	void EventNoticeMsg_Check();
	void EventNoticeMsg_Send( const std::string &tojid, const std::string &subject, const std::string &body, const char *callinfo );

	bool Update_Check( std::string &strVerLocal, std::string &strVerNew );
	void Update_Check_fromremote( const JID &fromjid, const std::string& from_id );
	virtual void handleTag( Tag* tag );
	bool Update_DoUpdateNow( uint32_t &err, std::string runparam = "-update" );
	void Update_DoUpdateNow_fromremote( const JID &fromjid, const std::string& from_id = EmptyString, std::string runparam = "-update" );
	void Update_UpdatedProc();

	bool is_running() const
	{
		return m_running;
	}

	void OnPlayBackThreadExit()
	{
		m_isPlayBackThreadExit = true;
	}

	void OnPlayMgrThreadExit()
	{
		m_isPlayMgrThreadExit = true;
	}

	void DataProc_ThreadCreate();
	bool DataProc();
	void DataSave();
	void DataStatCheck();
	void OnDataProcThreadExit()
	{
		m_isDataProcThreadExit = true;
	}

	CDataStoreMgr* GetDataStoreMgr( bool autoCreate=false )
	{
		if( autoCreate && !m_DataStoreMgr )
		{
			m_DataStoreMgr = new CDataStoreMgr();
		}

		return m_DataStoreMgr;
	}

	// AC门禁
	bool ACProc();
	bool ACProcOne();
	void ACProc_ThreadCreate();
	void ACProcThreadExit()
	{
		m_isACProcThreadExit = true;
	}

	// 告警处理相关
	//virtual void OnCameraAlarmRecv( const bool isAlarm, const IPCameraType CameraType, const char *sDeviceIP, const int nPort, const int channel, const char *alarmstr );
	bool CameraAlarmRecvMap_push( const IPCameraType CameraType, const char *sDeviceIP, const int nPort, const int channel, const char *alarmstr );
	bool CameraAlarmRecvMap_pop( struCamAlarmRecv &CamAlarmRecv );
	int CameraAlarmRecvMap_size(); //jyc20170405 resume until here
	void AlarmProc_ThreadCreate();
	bool AlarmProc();
	bool AlarmCheck();
	void OnAlarmProcThreadExit()//告警处理线程退出
	{
		m_isAlarmProcThreadExit = true;
	}

	void check_all_NetUseable( const bool CheckNow=false );
	void check_all_devtime( const bool CheckNow=false );
	void check_all_devtime_proc();
	bool check_all_devtime_IsChecked( const std::string &key );
	bool hasNetUseableFailed() const;
	
	std::string GetSimpleInfo( const GSIOTDevice *const iotdevice );
	std::string GetSimpleInfo_ForSupportAlarm( const GSIOTDevice *const iotdevice );
	
	std::string getstr_ForRelation( const deflstRelationChild &ChildList );

	void LED_RefreshNow()
	{
		m_lastShowLedTick = timeGetTime() - 999*1000;
	}
	
	defGSReturn SendControl( const IOTDeviceType DevType, const GSIOTDevice *device, const GSIOTObjBase *obj, const uint32_t overtime=defNormSendCtlOvertime, const uint32_t QueueOverTime=defNormMsgOvertime, const uint32_t nextInterval=1, const bool isSync=false );

private:
	bool m_PreInitState;
	void CheckOverTimeControlMesssageQueue_nolock();
	bool m_running;

	time_t m_IOT_starttime;
	std::string m_str_IOT_starttime;
	bool m_xmppReconnect;

	Parser m_parser;
	std::string m_strVerLocal;
	std::string m_strVerNew;
	bool m_retCheckUpdate;
	bool m_isThreadExit;
	bool m_isPlayBackThreadExit;
	bool m_isPlayMgrThreadExit;

	uint32_t m_PlaybackThreadTick;
	uint32_t m_PlaybackThreadCreateCount;

	bool m_isDataProcThreadExit;
	
	// 告警处理相关
	gloox::util::Mutex m_mutex_AlarmProc;
	bool m_isAlarmProcThreadExit;
	//deflstCamAlarmRecv m_lstCamAlarmRecv;//摄像机告警原始信息缓冲队列

	int m_last_checkNetUseable_camid;
	uint32_t m_last_checkNetUseable_time;

	// check dev time
	std::set<int> m_check_all_devtime_NeedIDList; // 准备进行校时的队列
	std::set<std::string> m_check_all_devtime_CheckedIPList; // 已校时

	// 数据存储相关
	CDataStoreMgr *m_DataStoreMgr;
	gloox::util::Mutex m_mutex_DataStore;
	std::list<struDataSave*> m_lstDataSaveBuf;
	struDataSave* DataSaveBuf_Pop();
	bool DataSaveBuf_Pop( defvecDataSave &vecDataSave );

	// LED Show
	uint32_t m_lastShowLedTick;

	// AC
	uint32_t m_lastcheck_AC;
	bool m_isACProcThreadExit;
};

#endif
