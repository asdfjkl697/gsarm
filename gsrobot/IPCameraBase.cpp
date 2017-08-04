#include "IPCameraBase.h"
#include "RTMPSend.h"
//#include <Windows.h>
#include "gloox/util.h"
#include "RunCode.h"
#include "common.h"
#include "audio/AudioCap_AAC.h"
#include "audio/AudioCap_G711.h"
#include "audio/AudioTestDefine.h"

static char *s_defCamAdvAttr_names[] =
{
	"是否支持智能跟踪",
	"是否支持手动跟踪",
	"是否支持手动定位",
	"是否支持摄像机对讲",
	"是否保持自动连接",
	"是否支持守望功能",
	"是否在空闲时保持守望",
	"是否支持预置点",
	"是否支持区域选择缩放",
	"是否具有告警功能",
	"是否具有云台斜角方向控制",
	"是否保持发布/自动发布",
	""
};

/////////////////////////////////////////////////

void struCameraFixParm::Reset()
{
	FixTSMod = defFixTSMod_NoFix;

	lasttime_Recal = 0;
	parmFps = 0;

	span_Min = defDefault_CameraFixParm_span_Min;
	span_Max = defDefault_CameraFixParm_span_Max;
	fix_Min = defDefault_CameraFixParm_fix_Min;
	fix_Max = defDefault_CameraFixParm_fix_Max;
	spanWarn = defDefault_CameraFixParm_spanWarn;

	mfactor_span_Min = defDefault_CameraFixParm_mfactor_span_Min;
	mfactor_span_Max = defDefault_CameraFixParm_mfactor_span_Max;
	mfactor_fix_Min = defDefault_CameraFixParm_mfactor_fix_Min;
	mfactor_fix_Max = defDefault_CameraFixParm_mfactor_fix_Max;
	mfactor_spanWarn = defDefault_CameraFixParm_mfactor_spanWarn;

	exparam = NULL;
}
#define macCheckParam( param ) if( param<defDefault_CameraFixParm_LOWER || param>defDefault_CameraFixParm_UPPER ) param = defDefault_CameraFixParm_##param;
void struCameraFixParm::CheckParam()
{
	macCheckParam( span_Min );
	macCheckParam( span_Max );
	macCheckParam( fix_Min );
	macCheckParam( fix_Max );
	//spanWarn不限制;

	macCheckParam( mfactor_span_Min );
	macCheckParam( mfactor_span_Max );
	macCheckParam( mfactor_fix_Min );
	macCheckParam( mfactor_fix_Max );
	//mfactor_spanWarn不限制;
}

void struCameraFixParm::Recal( uint32_t cur_fps, uint32_t timespan, const char *loginfo, bool isForceRecal )
{
	bool doRecal = isForceRecal;
	bool doprint = false;
	uint32_t nowtime = timeGetTime();

	if( cur_fps != parmFps )
	{
		doprint = true;
	}

	if( defFixTSMod_Auto == FixTSMod )
	{
		if( !doRecal && cur_fps != parmFps )
		{
			doRecal = true;
		}

		if( !doRecal && timespan>0 && nowtime-lasttime_Recal>timespan )
		{
			doRecal = true;
		}

		if( doRecal && cur_fps>5 )
		{
			// �ȳ���1000�����100�����ֱ�ӳ���10
			span_Min = uint32_t(mfactor_span_Min*10/cur_fps);
			span_Max = uint32_t(mfactor_span_Max*10/cur_fps);
			fix_Min = uint32_t(mfactor_fix_Min*10/cur_fps);
			fix_Max = uint32_t(mfactor_fix_Max*10/cur_fps);
			spanWarn = uint32_t(mfactor_spanWarn*10/cur_fps);

			lasttime_Recal = nowtime;

			doprint = true;
		}
	}

	parmFps = cur_fps;

	std::string str = "";

	if( loginfo )
		str = loginfo;

	if( doRecal )
		str += ":Recal";

	if( doprint )
		this->print( str.c_str() );
}

void struCameraFixParm::print( const char *pinfo )
{
	LOGMSG( "CameraFixParm(Mod=%d(%s))::(%s) fps=%d {span(Min=%d,Max=%d), fix(Min=%d,Max=%d), spanWarn=%d}; mfactor{span(Min=%d,Max=%d), fix(Min=%d,Max=%d), spanWarn=%d};\r\n",
		FixTSMod,
		util::deflookup( FixTSMod, s_name_defFixTSMod, "?" ).c_str(),
		pinfo?pinfo:"",
		parmFps,

		span_Min,
		span_Max,
		fix_Min,
		fix_Max,
		spanWarn,

		mfactor_span_Min,
		mfactor_span_Max,
		mfactor_fix_Min,
		mfactor_fix_Max,
		mfactor_spanWarn
		);
}

/////////////////////////////////////////////////

void struCameraState::Reset()
{
	isSetFirst = false;
	startTime = timeGetTime();
	sno = 0;
	validPktCount = 0;
	prev_FPS = 0;
	prev_ts = 0;
	prev_tskey = 0;
	prev_time = 0;
	exparam = NULL;
}

/////////////////////////////////////////////////

bool CCamAdvAttr::isShow_AdvAttr( defCamAdvAttr attr )
{
	switch(attr)
	{
	case defCamAdvAttr_Max_:
		return false;

	case defCamAdvAttr_CamTalk:
		return ( IsRUNCODEEnable(defCodeIndex_SYS_CamTalk) );

	case defCamAdvAttr_AutoPublish:
		return (IsRUNCODEEnable( defCodeIndex_SYS_AutoPublishEnable ));

	default:
		break;
	}

	return true;
}

const char* CCamAdvAttr::get_AdvAttr_Name( defCamAdvAttr attr )
{
	if( attr <defCamAdvAttr_Min_ || attr >=defCamAdvAttr_Max_ )
	{
		return s_defCamAdvAttr_names[defCamAdvAttr_Max_];
	}

	return s_defCamAdvAttr_names[attr];
}

bool CCamAdvAttr::get_AdvAttr( defCamAdvAttr attr ) const
{
	if( attr<0 || (size_t)attr>=m_AdvAttribute.size() )
	{
		assert(false);
		return false;
	}

	return m_AdvAttribute.test( attr );
}

bool CCamAdvAttr::set_AdvAttr( defCamAdvAttr attr, bool val )
{
	if( attr<0 || (size_t)attr>=m_AdvAttribute.size() )
	{
		assert(false);
		return false;
	}

	m_AdvAttribute.set( attr, val );
	return true;
}

uint32_t CCamAdvAttr::get_AdvAttr_uintfull() const
{
	return m_AdvAttribute.to_ulong();
}

bool CCamAdvAttr::set_AdvAttr_uintfull( uint32_t fullval )
{
	std::bitset<defCamAdvAttr_Max_> attrfv((_ULonglong)fullval);
	m_AdvAttribute = attrfv;
	return true;
}

/////////////////////////////////////////////////

IPCameraBase::IPCameraBase(const std::string& deviceID, const std::string& name,
		const std::string& ipaddress,uint32_t port,const std::string& username,
		const std::string& password, const std::string& ver, GSPtzFlag PTZFlag, GSFocalFlag FocalFlag,
		uint32_t channel, uint32_t streamfmt )
		:CameraControl(this,"rtmp",ver,"","stoped", PTZFlag, FocalFlag),m_deviceID(deviceID), m_name(name),
		m_ipaddress(ipaddress),m_port(port), m_username(username),m_password(password),
		m_channel(channel), m_streamfmt(streamfmt), m_NetUseable(defUseable_OK), m_curVideoBitrate(0),
		m_camera_hwnd(0), m_playhandle(0), m_isPlayBack(false), m_prev_tick_PrintFrame(0), m_prev_tick_PrintFrame_Audio(0)
{
	this->SetFrameState(false);

	m_pLocalCapAudio = NULL;
	ReCreateLocalCapAudio();

	stream = new MediaStream(this, name);

	prev_isState_Publish = false;
	prev_isState_Url = false;
	prev_isState_Session = false;
	prev_FrameState = false;
	prev_IsConnect = false;
	prev_checktime = timeGetTime();

	m_src_first_ts_video = 0;
	m_src_first_ts_audio = 0;
	m_src_prev_fix = 0;

	// state
	m_EnableTrack = 0;
	m_PTZ_ParkAction = 0;

	m_CurAlarmState = defAlarmState_UnInit;
	m_lastupdate_CurAlarmState = timeGetTime();
}

IPCameraBase::~IPCameraBase(void)
{
	m_mutex_sessionList.lock();

	while(sessionList.size()>0){
		RTMPSession *s = sessionList.front();
		delete(s);
		sessionList.pop_front();
	}	
	sessionList.clear();

	m_mutex_sessionList.unlock();

	if(stream->IsPublish()){
		stream->StopRTMPSend();
	}
	delete(stream);
	stream = NULL;

	if( m_pLocalCapAudio )
	{
		delete m_pLocalCapAudio;
		m_pLocalCapAudio = NULL;
	}
}

bool IPCameraBase::doEditAttrFromAttrMgr( const EditAttrMgr &attrMgr, defCfgOprt_ oprt )
{
	if( attrMgr.GetEditAttrMap().empty() )
		return false;

	bool doUpdate = false;
	std::string outAttrValue;

	if( attrMgr.FindEditAttr( "ipaddress", outAttrValue ) )
	{
		doUpdate = true;
		this->SetIPAddress( outAttrValue );
	}

	if( attrMgr.FindEditAttr( "port", outAttrValue ) )
	{
		doUpdate = true;
		this->setPort( (uint32_t)atoi(outAttrValue.c_str()) );
	}

	if( attrMgr.FindEditAttr( "username", outAttrValue ) )
	{
		doUpdate = true;
		this->SetUsername( outAttrValue );
	}

	if( attrMgr.FindEditAttr( "password", outAttrValue ) )
	{
		doUpdate = true;
		this->SetPassword( outAttrValue );
	}

	if( attrMgr.FindEditAttr( "channel", outAttrValue ) )
	{
		doUpdate = true;
		this->SetChannel( atoi(outAttrValue.c_str()) );
	}

	if( attrMgr.FindEditAttr( "streamfmt", outAttrValue ) )
	{
		doUpdate = true;
		this->SetStreamfmt( atoi(outAttrValue.c_str()) );
	}

	if( attrMgr.FindEditAttr( "ptz", outAttrValue ) )
	{
		doUpdate = true;
		this->setPTZFlag( (GSPtzFlag)atoi(outAttrValue.c_str()) );
	}

	if( attrMgr.FindEditAttr( "focal", outAttrValue ) )
	{
		doUpdate = true;
		this->setFocalFlag( (GSFocalFlag)atoi(outAttrValue.c_str()) );
	}

	if( attrMgr.FindEditAttr( "buffertime", outAttrValue ) )
	{
		doUpdate = true;
		this->setBufferTime( atoi(outAttrValue.c_str()) );
	}

	if( attrMgr.FindEditAttr( "device_id", outAttrValue ) )
	{
		doUpdate = true;
		this->SetDeviceId( outAttrValue );
	}

	return doUpdate;
}

bool IPCameraBase::IsPublish()
{
	return this->stream->IsPublish();
}

#define defRTMP_TEST_URLBASE "rtmp://test.gsss.cn/gslive/"
defGSReturn IPCameraBase::SendToRTMPServer( const JID& jid, const std::string& url, const std::vector<std::string> &url_backup )
{
	if( !this->IsConnect() )
	{
		{
			this->SetFrameState( true ); // 预复位
		}
	}

	// // 先更新会话，然后检查，如果之前是已经发布，那么更新后应该至少有两个会话
	bool isAdded = false;
	const uint32_t BeforeSessionCount = this->UpdateSession(jid, &isAdded);
	const uint32_t AfterSessionCount = this->GetSessionCount();
	const bool is_publish_base = this->IsPublish();
	const bool isPublished = isAdded ? ( is_publish_base && BeforeSessionCount>1 ) : is_publish_base;

	LOGMSG( "CameraBase(%s)::SendToRTMPServer(AfterSessionCount=%d, isAdded=%d)...\r\n", this->m_name.c_str(), AfterSessionCount, isAdded );

	// 处于过渡状态，IsPublish未重置，但其实已经正在退出发布
	if( BeforeSessionCount>1 && AfterSessionCount <= 1 && this->IsPublish() )
	{
		DWORD dwStartWait = ::timeGetTime();
		while( this->IsPublish()
			&& ::timeGetTime()-dwStartWait<5000 )
		{
			usleep(1000);//Sleep(1);
		}

		LOGMSG( "CameraBase(%s)::SendToRTMPServer wait publishFlag Reset, BeforeSessionCount=%d, AfterSessionCount=$d\r\n", this->m_name.c_str(), BeforeSessionCount, AfterSessionCount );
	}


	std::string useUrl = url;

	if( IsRUNCODEEnable(defCodeIndex_TEST_RTMP_TestMod) )
	{
		if( RUNCODE_Get(defCodeIndex_TEST_RTMP_TestMod,defRunCodeValIndex_2) )
		{
			useUrl = std::string(defRTMP_TEST_URLBASE) + m_name;
		}
		else
		{
			std::string seed;
			std::string::size_type spos = 0;
			while(true)
			{
				std::string::size_type spos_prev = spos;
				spos = url.find( '/', spos+1 );
				if( spos == std::string::npos )
				{
					seed = url.substr( spos_prev+1 );
					break;
				}
			}

			useUrl = std::string(defRTMP_TEST_URLBASE) + seed;
		}
	}

	if( !isPublished )
	{
		this->stream->StopRTMPSend();
		this->stream->GetRTMPSendObj()->set_wait_frist_key_frame();
	}

	bool doMakeKeyFrame = true;
	if( !this->IsConnect() )
	{
		doMakeKeyFrame = false;
		const defGSReturn ConnectRet = this->Connect(true);
		if( macGSFailed(ConnectRet) )
		{
			//doMakeKeyFrame = false;
			LOGMSG( "CameraBase(%s)::SendToRTMPServer Connect cam failed, ConnectRet=%d:%s\r\n", this->m_name.c_str(), ConnectRet, g_Trans_GSReturn(ConnectRet).c_str() );
			return ConnectRet;
		}
	}

	//--------------------------
	// 对于未发布的，根据发过来的url进行连接
	// 对于已发布的，旧的为RTMFP，新的为RTMP则进行RTMP的连接，两者共存，RTMP优先使用，否则不进行新的连接
	// 效果，未发布则总是进行新的连接；已发布RTMP则不再进行新连接；已发布RTMFP则进行新的RTMP连接，并RTMFP仍保留不会断开、与RTMP共存，但RTMP优先使用。
	const bool Old_isRTMFP = g_IsRTMFP_url( this->getUrl() );	// 本地已经存在的url是否是RTMFP
	const bool New_isRTMFP = g_IsRTMFP_url( useUrl );			// 远程发过来的新url是否是RTMFP

	bool doConnect_RTMP = false; // 是否进行RTMP连接
	bool doConnect_RTMFP = false; // 是否进行RTMFP连接

	if( IsRUNCODEEnable(defCodeIndex_SYS_Enable_RTMFP) )
	{
		if( !isPublished )
		{
			if( New_isRTMFP )
			{
				doConnect_RTMFP = true;
			}
			else
			{
				doConnect_RTMP = true;
			}
		}
		else if( Old_isRTMFP )
		{
			// do rtmp
			if( !New_isRTMFP )
			{
				doConnect_RTMP = true;
			}
			else
			{
				if( stream->GetRTMPSendObj()->getPeerID().empty() )
				{
					doConnect_RTMP = true;
				}
			}

			if( !doConnect_RTMP )
			{
				const int RTMFP_SessionLimit = RUNCODE_Get( defCodeIndex_RTMFP_SessionLimit );
				const int curRTMFPSessionCount = stream->GetRTMPSendObj()->getRTMFPSessionCount();
				if( curRTMFPSessionCount >= RTMFP_SessionLimit )
				{
					doConnect_RTMP = true;

					LOGMSG( "CameraBase(%s)::RTMFP_SessionLimit cur=%d, limit=%d\r\n", this->m_name.c_str(), curRTMFPSessionCount, RTMFP_SessionLimit );
				}
			}
		}
	}
	else
	{
		if( !isPublished )
		{
			doConnect_RTMP = true;
		}
	}

	if( doConnect_RTMFP )
	{
		this->UpdateSession( JID(defRTMFPSession_strjid) );

		stream->GetRTMPSendObj()->setPeerID( "" );
		stream->GetRTMPSendObj()->pushRTMPHandle( NULL, useUrl );

		// 后面
		if( IsRUNCODEEnable( defCodeIndex_RTMFP_UrlAddStreamID ) )
		{
			useUrl += "/";
			useUrl += stream->GetRTMPSendObj()->getStreamID();
		}
	}

	//等待视频基本信息完成

	if( !isPublished )
	{
		if( !stream->SendToRTMPServer(useUrl) )
		{
			return defGSReturn_Err;
		}
	}

	if( doMakeKeyFrame )
	{
		this->MakeKeyFrame( false );
	}

	if( doConnect_RTMFP )
	{
		const int waittime = RUNCODE_Get( defCodeIndex_RTMFP_WaitConnTimeout );
		const DWORD dwStart = ::timeGetTime();
		while( stream->GetRTMPSendObj()->getPeerID().empty() && ::timeGetTime()-dwStart < waittime )
		{
			usleep(50000);//Sleep( 100 );
		}

		if( stream->GetRTMPSendObj()->getPeerID().empty() )
		{
			LOGMSG( "CameraBase(%s)::SendToRTMPServer wait rtmfp connect timeout! %dms\r\n", this->m_name.c_str(), ::timeGetTime()-dwStart );

			doConnect_RTMP = true; // 连接RTMFP失败后自动参试连接RTMP
		}
		else
		{
			stream->GetRTMPSendObj()->Connect( useUrl );
			LOGMSG( "CameraBase(%s)::SendToRTMPServer wait rtmfp connect success. %dms\r\n", this->m_name.c_str(), ::timeGetTime()-dwStart );
		}
	}

	if( doConnect_RTMP )
	{
		std::vector<std::string> vecurl;
		if( !New_isRTMFP ) vecurl.push_back( useUrl );
		for( int i=0; i<url_backup.size(); ++i )
		{
			vecurl.push_back( url_backup[i] );
		}

		char cntbuf[64] ={0};
		if( IsRUNCODEEnable( defCodeIndex_SYS_UseUrlDifCnt ) )
		{
			static uint32_t s_url_difcnt = timeGetTime();

			for( uint32_t iurlbak=0; iurlbak<vecurl.size(); ++iurlbak )
			{
				vecurl[iurlbak] += std::string( "GS" );
				//vecurl[iurlbak] += itoa( timeGetTime() + s_url_difcnt++, cntbuf, 16 );
				//vecurl[iurlbak] += sprintf(cntbuf, "%d", timeGetTime() + s_url_difcnt++);
				sprintf(cntbuf, "%d", timeGetTime() + s_url_difcnt++);
				vecurl[iurlbak] += cntbuf;
			}
		}

		defRTMPConnectHandle RTMPhandle = RTMPSend::CreateRTMPInstance( vecurl, useUrl, this->GetName().c_str() );
		if( !RTMPhandle )
		{
			LOGMSG( "CameraBase(%s)::CreateRTMPInstance Connect rtmp failed!\r\n", this->m_name.c_str() );

			return defGSReturn_ConnectSvrErr;
		}

		stream->GetRTMPSendObj()->pushRTMPHandle( RTMPhandle, useUrl );
		stream->GetRTMPSendObj()->Connect( useUrl );
	}

	if( this->getStatus() != "playing" )
	{
		this->setStatus("playing");
	}
	return defGSReturn_Success;
}

void IPCameraBase::StopRTMPSend(const JID& jid)
{
	int size = 0;

	m_mutex_sessionList.lock();

	std::list<RTMPSession *>::iterator it = sessionList.begin();
	while(it!=sessionList.end()){
		if((*it)->GetJID() == jid){		    
			RTMPSession *s = *it;
			sessionList.erase(it);
			delete(s);
			it = sessionList.begin();
		}else{
		    it++;
		}
	}

	size = sessionList.size();

	m_mutex_sessionList.unlock();

	if(size==0){
	    stream->StopRTMPSend();
		this->setStatus("stoped");
		this->setUrl("");
	}
}

void IPCameraBase::StopRTMPSendAll()
{
	m_mutex_sessionList.lock();

	if( !sessionList.empty() )
	{
		std::list<RTMPSession*>::iterator it = sessionList.begin();
		std::list<RTMPSession*>::iterator itEnd = sessionList.end();
		for( ; it!=itEnd; ++it )
		{
			RTMPSession *s = *it;
			delete(s);
		}
		sessionList.clear();
	}

	m_mutex_sessionList.unlock();

	stream->StopRTMPSend();
	this->setStatus("stoped");
	this->setUrl("");

}

bool IPCameraBase::CheckSession(const uint32_t enable, const bool isPlayBack,
		const bool CheckNow) {
	if (CheckNow) {
		//LOGMSG( "Camera(%s) CheckSession Now!", m_name.c_str() );
	} else if ((timeGetTime() - prev_checktime) < (30 * 1000)) {
		return false;
	}

	bool isStateChanged = false;
	int size = 0;
	m_mutex_sessionList.lock();

	if (!sessionList.empty()) {
		std::list<RTMPSession *>::iterator it = sessionList.begin();
		while (it != sessionList.end()) {
			if (!(*it)->Check()) {
				isStateChanged = true;
				RTMPSession *s = *it;
				sessionList.erase(it);
				delete (s);
				it = sessionList.begin();
			} else {
				it++;
			}
		}
		size = sessionList.size();
	}

	m_mutex_sessionList.unlock();
	const bool isAutoConnectFlag = this->isAutoConnect();
	const bool isFrameState = this->GetFrameState();
	const bool isState_Publish = IsPublish()
			&& this->stream->GetRTMPSendObj()->IsRtmpLive();
	const bool isState_Url = !this->getUrl().empty();

	if ( IsRUNCODEEnable(defCodeIndex_SYS_AutoPublishEnable)) {
		if (this->GetAdvAttr().get_AdvAttr(defCamAdvAttr_AutoPublish)) {
			if (isState_Publish) {
				this->UpdateSession(JID(defAutoPublish_strjid));
				size += 1;
			}
		}
	}

	const bool isState_Session = (size > 0);
	if (this->IsConnect()) {
		this->check_NetUseable();
	}
	bool doDisconnect = false;
	bool doConnect = false;

	if (this->IsConnect() && !isState_Session && !isAutoConnectFlag) {
		doDisconnect = true;
	}

	// ֻҪ���ڲ�һ�¾�ֹͣ
	if (isState_Publish != isState_Url || isState_Url != isState_Session
			|| isState_Session != isState_Publish) {
		//LOGMSGEX( defLOGNAME, defLOG_INFO, "Camera(%s) CheckSession Failed! isState_Publish=%d, isState_Url=%d, SessionSize=%d\r\n", 
		//	m_name.c_str(), isState_Publish, isState_Url, size );

		this->stream->StopRTMPSend();
		this->setStatus("stoped");
		this->setUrl("");
	}

	if (!isFrameState && (this->IsConnect() || isState_Session || isAutoConnectFlag)) {
		if (isPlayBack) {
			if (!isState_Session) {
				doDisconnect = true;
			}
		} else {
			doDisconnect = true;
		}

		if (isState_Session) {
			if (!isPlayBack) {
				//LOGMSGEX( defLOGNAME, defLOG_ERROR, "Camera(%s) Long Time No Stream\r\n", m_name.c_str() );
			}
			doConnect = true;
		}
	}

	if (doDisconnect) {
		this->OnDisconnct();
		if (enable && (isAutoConnectFlag || isState_Session)) {
			doConnect = true;
		}
	}

	if (enable && doConnect && !isPlayBack) {
		this->Connect(false);
	}

	if (isState_Publish != prev_isState_Publish
			|| isState_Url != prev_isState_Url
			|| isState_Session != prev_isState_Session
			|| isFrameState != prev_FrameState
			|| this->IsConnect() != prev_IsConnect) {
		isStateChanged = true;
	}

	prev_isState_Publish = isState_Publish;
	prev_isState_Url = isState_Url;
	prev_isState_Session = isState_Session;
	prev_FrameState = isFrameState;
	prev_IsConnect = this->IsConnect();
	prev_checktime = timeGetTime();

	return isStateChanged;
}

uint32_t IPCameraBase::UpdateSession( const JID& jid, bool *isAdded )
{
	uint32_t count = 0;

	m_mutex_sessionList.lock();

	std::list<RTMPSession *>::iterator it = sessionList.begin();
	for(;it!=sessionList.end();it++){
		if((*it)->GetJID() == jid){
			(*it)->Update();
			count = sessionList.size();
			m_mutex_sessionList.unlock();

			if( isAdded )
			{
				*isAdded = false;
			}

			return count;
		}
	}

	m_mutex_sessionList.unlock();

	RTMPSession *s = new RTMPSession(jid);

	m_mutex_sessionList.lock();
	sessionList.push_back(s);

	count = sessionList.size();
	m_mutex_sessionList.unlock();

	if( isAdded )
	{
		*isAdded = true;
	}

	return count;
}

// return starttime
uint32_t IPCameraBase::GetSessionStartTime( const std::string& strjid )
{
	m_mutex_sessionList.lock();

	uint32_t StartTime = 0;
	for( std::list<RTMPSession*>::const_iterator it = sessionList.begin(); it!=sessionList.end(); ++it )
	{
		if( (*it)->GetJID() == strjid )
		{
			StartTime = (*it)->get_start_time();
			if( 0==StartTime )
			{
				StartTime+=1;
			}

			break;
		}
	}

	m_mutex_sessionList.unlock();

	return StartTime;
}

// return LastUpdateTime ts
uint32_t IPCameraBase::GetSessionLastUpdateTime( const std::string& strjid )
{
	m_mutex_sessionList.lock();

	uint32_t LastUpdateTime = 0;
	for( std::list<RTMPSession*>::const_iterator it = sessionList.begin(); it!=sessionList.end(); ++it )
	{
		if( strjid.empty() || (*it)->GetJID() == strjid )
		{
			LastUpdateTime = (*it)->get_timestamp();
			break;
		}
	}

	m_mutex_sessionList.unlock();

	return LastUpdateTime;
}

uint32_t IPCameraBase::GetSessionCount()
{
	uint32_t count = 0;

	m_mutex_sessionList.lock();
	count = sessionList.size();
	m_mutex_sessionList.unlock();

	return count;
}

uint32_t IPCameraBase::GetSessionInfoList( deflstRTMPSessionInfo &outSessionInfoList )
{
	uint32_t count = 0;
	if( !outSessionInfoList.empty() )
	{
		outSessionInfoList.clear();
	}

	m_mutex_sessionList.lock();

	count = sessionList.size();

	std::list<RTMPSession*>::iterator it = sessionList.begin();
	for( ; it!=sessionList.end(); ++it )
	{
		outSessionInfoList.push_back( RTMPSessionInfo( (*it) ) );
	}

	m_mutex_sessionList.unlock();

	return count;
}

void IPCameraBase::OnPublishStart()
{

}

void IPCameraBase::OnPublishStop(defGSReturn code)
{
	this->setStatus("stoped");
	this->setUrl("");

	if( defGSReturn_NoData == code )
	{
		this->set_NetUseable( defUseable_ErrNoData );
	}
}

uint32_t IPCameraBase::OnPublishUpdateSession( const std::string &strjid, bool *isAdded )
{
	return this->UpdateSession( JID(strjid), isAdded );
}

void IPCameraBase::PushVideo( const bool keyframe, const uint32_t rate, char *data, int size, uint32_t timestamp, const bool use_input_ts, const bool isAudio )
{
	if( isAudio )
	{
		if( defAudioSource_Null == this->GetAudioCfg().get_Audio_Source()
			|| defAudioSource_File == this->GetAudioCfg().get_Audio_Source()
			)
		{
			return;
		}

		if( defAudioFmtType_Unknown == this->GetAudioCfg().get_Audio_FmtType() )
		{
			return;
		}
	}
	else
	{
		this->SetFrameState( true );
		this->set_NetUseable( defUseable_OK );
	}

	//-----
	// fix PlayBack time
	if( this->GetStreamObj()->GetRTMPSendObj()->GetIPlayBack() )
	{
		if( IsRUNCODEEnable(defCodeIndex_TEST_PlayBack_timefix_pnt) )
		{
			LOGMSG( "CameraBase::PushVideo fixedpre(%s) ts=%u, prev_ts=%u, fv=%u, fa=%u, audio=%d, size=%d%s", this->m_name.c_str(), (uint32_t)timestamp, m_src_prev_fix, m_src_first_ts_video, m_src_first_ts_audio, isAudio, size, m_src_prev_fix>timestamp?"*****":"" );
		}
		if( isAudio ){
			if( !m_src_first_ts_audio ){
				const int c_PlayBack_timefix_a = RUNCODE_Get(defCodeIndex_SYS_PlayBack_timefix_a);
				LOGMSG( "CameraBase::PushVideo PlayBack_timefix_a=%d", c_PlayBack_timefix_a );
				m_src_first_ts_audio = timestamp-c_PlayBack_timefix_a;
			}
			timestamp -= m_src_first_ts_audio;
		}
		else{
			if( !m_src_first_ts_video ){
				m_src_first_ts_video = timestamp;
			}
			timestamp -= m_src_first_ts_video;
		}

		if( IsRUNCODEEnable(defCodeIndex_TEST_PlayBack_timefix_pnt) )
		{
			LOGMSG( "CameraBase::PushVideo fixed(%s) ts=%u, prev_ts=%u, fv=%u, fa=%u, audio=%d, size=%d%s\r\n", this->m_name.c_str(), (uint32_t)timestamp, m_src_prev_fix, m_src_first_ts_video, m_src_first_ts_audio, isAudio, size, m_src_prev_fix>timestamp?"*****":"" );
		}

		if( m_src_prev_fix>timestamp )
		{
			timestamp = m_src_prev_fix;
		}

		m_src_prev_fix = timestamp;
	}
	//--

	this->m_state.sno++;

	if( use_input_ts ){
		bool PrintQueInfo = false;
		if( isAudio ){
			if( RUNCODE_Get(defCodeIndex_Cam_PrintFrame,defRunCodeValIndex_4) ){
				if( ::timeGetTime()-m_prev_tick_PrintFrame_Audio >= (DWORD)RUNCODE_Get(defCodeIndex_Cam_PrintFrame,defRunCodeValIndex_2) )
				{
					PrintQueInfo = true;
					m_prev_tick_PrintFrame_Audio = ::timeGetTime();
				}
			}
		}
		else{
			if( IsRUNCODEEnable(defCodeIndex_Cam_PrintFrame) ){
				if( 0 == RUNCODE_Get(defCodeIndex_Cam_PrintFrame,defRunCodeValIndex_2) ) // ���Ǵ�ӡ
				{
					PrintQueInfo = true;
				}
				else if( keyframe && RUNCODE_Get(defCodeIndex_Cam_PrintFrame,defRunCodeValIndex_3) )
				{
					PrintQueInfo = true;
				}
				else if( ::timeGetTime()-m_prev_tick_PrintFrame >= (DWORD)RUNCODE_Get(defCodeIndex_Cam_PrintFrame,defRunCodeValIndex_2) )
				{
					PrintQueInfo = true;
					m_prev_tick_PrintFrame = ::timeGetTime();
				}
			}

			if( !PrintQueInfo && IsRUNCODEEnable(defCodeIndex_Cam_PrintFrameSize) )
			{
				if( size >= (int)RUNCODE_Get(defCodeIndex_Cam_PrintFrameSize,defRunCodeValIndex_2) )
				{
					PrintQueInfo = true;
				}
			}
		}

		if( PrintQueInfo ){
			LOGMSG( "MediaStream::PushVideo(%s) ts=%u, key=%d, audio=%d, rate=%u, size=%u\r\n", m_name.c_str(), (uint32_t)timestamp, keyframe, isAudio, rate, size );
		}
		
		this->stream->PushVideo1( keyframe, data, size, timestamp, use_input_ts, isAudio );
		return ;
	}

	uint32_t cur_ts = 0;
	cur_ts = timeGetTime();
	uint32_t cur_ts_src = cur_ts;
	uint32_t span = 0;
	span = this->m_state.isSetFirst ? (cur_ts - this->m_state.prev_ts) : 0;

//	this->m_fixparm.Recal( rate, 5*60*1000, this->m_name.c_str() );
	this->m_fixparm.Recal( rate, 5*60*1000, this->m_name.c_str(), true); //jyc20170612 modify

	if( defFixTSMod_NoFix!=this->m_fixparm.FixTSMod 
		&& this->m_state.sno>100
		)
	{
		if( this->m_state.isSetFirst && span > this->m_fixparm.span_Max )
		{
			cur_ts = this->m_state.prev_ts+this->m_fixparm.fix_Max;
		}

		if( this->m_state.isSetFirst && (cur_ts < this->m_state.prev_ts) )
		{
			cur_ts = this->m_state.prev_ts+this->m_fixparm.fix_Min;
		}

		// ʱ������ʱ���ã���ǰֱ֡�Ӳ��õ�ǰʱ���
		//--temptest playbackparam-20000
		const uint32_t s_spanmax = 2000;
		if( span > s_spanmax )
		{
			cur_ts = cur_ts_src;
			LOGMSG( "Camera(%s) span > %d, reset cur_ts=current!!!\r\n", m_name.c_str(), s_spanmax );
		}

		// FPS�仯ʱ���ã���ǰֱ֡�Ӳ��õ�ǰʱ���
		if( this->m_state.prev_FPS != rate )
		{
			cur_ts = cur_ts_src;
			LOGMSG( "Camera(%s) prev_FPS != rate, reset cur_ts=current!!!\r\n", m_name.c_str() );
		}

		if( cur_ts > cur_ts_src )
		{
			cur_ts = cur_ts_src;
		}
	}

	bool PrintQueInfo = false;

	uint32_t span_time = this->m_state.isSetFirst ? (timeGetTime() - this->m_state.prev_time) : 0;
	if(  span>this->m_fixparm.spanWarn || span_time>this->m_fixparm.spanWarn )
	{
		PrintQueInfo = true;
	}

	if( IsRUNCODEEnable(defCodeIndex_Cam_PrintFrame) )
	{
		if( keyframe && RUNCODE_Get(defCodeIndex_Cam_PrintFrame,defRunCodeValIndex_3) )
		{
			PrintQueInfo = true;
		}
		else if( ::timeGetTime()-m_prev_tick_PrintFrame >= (DWORD)RUNCODE_Get(defCodeIndex_Cam_PrintFrame,defRunCodeValIndex_2) )
		{
			PrintQueInfo = true;
		}
	}

	if( !PrintQueInfo && IsRUNCODEEnable(defCodeIndex_Cam_PrintFrameSize) )
	{
		if( size >= (int)RUNCODE_Get(defCodeIndex_Cam_PrintFrameSize,defRunCodeValIndex_2) )
		{
			PrintQueInfo = true;
		}
	}

	if( PrintQueInfo )
	{
		LOGMSG( "Camera(%s)(%d): fps=%d, key=%d, ts=%u, span=%d, fixed=%d, keyspan=%d, size=%d, span_time=%d %s\r\n", 
			m_name.c_str(), this->m_state.sno, rate, keyframe, cur_ts, span, cur_ts-cur_ts_src, cur_ts-this->m_state.prev_tskey, size, span_time, span>this->m_fixparm.spanWarn?"!!!":"" );

		m_prev_tick_PrintFrame = ::timeGetTime();
	}

	stream->PushVideo( keyframe, data, size, cur_ts ); //h264 no head

	if( keyframe )
	{
		this->m_state.prev_tskey = cur_ts;
	}
	this->m_state.prev_ts = cur_ts;
	this->m_state.prev_FPS = rate;
	this->m_state.prev_time = timeGetTime();
	this->m_state.isSetFirst = true;
}

bool IPCameraBase::ReCreateLocalCapAudio()
{
	StopLocalCapAudio();

	if( m_pLocalCapAudio )
	{
		delete m_pLocalCapAudio;
		m_pLocalCapAudio = NULL;
	}
	
	if( defAudioSource_LocalCap == this->GetAudioCfg().get_Audio_Source() )
	{
		switch( this->GetAudioCfg().get_Audio_FmtType() )
		{
		case defAudioFmtType_PCM_ALAW:
		case defAudioFmtType_PCM_MULAW:
			{
				AudioCap_G711 *pLocalCapAudio = new AudioCap_G711();
				pLocalCapAudio->setG711Type( this->GetAudioCfg().get_Audio_FmtType() );

				m_pLocalCapAudio = pLocalCapAudio;
			}
			break;

		case defAudioFmtType_AAC:
			{
				m_pLocalCapAudio = new AudioCap_AAC();
			}
			break;
		}

		if( m_pLocalCapAudio )
			m_pLocalCapAudio->setAudioCB( this );
	}

	return true;
}

bool IPCameraBase::StartLocalCapAudio()
{
	bool doReCreate = false;
	if( defAudioSource_LocalCap == this->GetAudioCfg().get_Audio_Source() )
	{
		if( this->m_pLocalCapAudio )
		{
			if( this->m_pLocalCapAudio->GetAudioCapType() 
				!= CAudioCfg::trans_FmtType2AudioCapType( this->GetAudioCfg().get_Audio_FmtType() )
				)
			{
				doReCreate = true;
			}

			if( defAudioCapType_G711 == this->m_pLocalCapAudio->GetAudioCapType() )
			{
				((AudioCap_G711*)this->m_pLocalCapAudio)->setG711Type( this->GetAudioCfg().get_Audio_FmtType() );
			}
		}
		else
		{
			doReCreate = true;
		}
	}
	else
	{
		doReCreate = true;
	}

	if( doReCreate )
	{
		//LOGMSGEX( defLOGNAME, defLOG_INFO, "StartLocalCapAudio(%s) do ReCreateLocalCapAudio...\r\n", m_name.c_str() );

		ReCreateLocalCapAudio();
	}

	if( this->m_pLocalCapAudio 
		&& !this->GetStreamObj()->GetRTMPSendObj()->GetIPlayBack()
		&& defAudioSource_LocalCap == this->GetAudioCfg().get_Audio_Source()
		)
	{
		if( !this->m_pLocalCapAudio->IsOpen() )
		{
			bool isSuccess = this->m_pLocalCapAudio->OpenAudio( this->GetAudioCfg() );

			//LOGMSGEX( defLOGNAME, isSuccess?defLOG_INFO:defLOG_ERROR, "StartLocalCapAudio(%s) ret=%s\r\n", 
			//	m_name.c_str(), isSuccess?"success":"failed!!!" );

			return isSuccess;
		}
	}

	return false;
}

bool IPCameraBase::StopLocalCapAudio()
{
	if( this->m_pLocalCapAudio )
	{
		if( this->m_pLocalCapAudio->IsOpen() )
		{
			//LOGMSGEX( defLOGNAME, defLOG_INFO, "StopLocalCapAudio(%s)", m_name.c_str() );
		}

		this->m_pLocalCapAudio->CloseAudio();
	}

	return true;
}

void IPCameraBase::OnAudioBufferCB( double SampleTime, unsigned char *pBuffer, long BufferLen )
{
	if( BufferLen > 0 )
	{
		if( defAudioSource_LocalCap != this->GetAudioCfg().get_Audio_Source() )
		{
			return;
		}

		uint32_t cur_ts = timeGetTime();

		//LOGMSG( "IPCameraBase::OnAudioBufferCB(%s) size=%u, ts=%u\r\n", m_name.c_str(), BufferLen, cur_ts );

		this->PushVideo( false, 25, (char *)pBuffer, BufferLen, cur_ts, true, true );
	}
}

bool IPCameraBase::isChanged( 
	const std::string &old_ipaddress, 
	const uint32_t old_port, 
	const std::string &old_username, 
	const std::string &old_password, 
	const uint32_t old_channel, 
	const uint32_t old_streamfmt,
	const CAudioCfg &old_AudioCfg
) const
{
	// �Ƿ������øı䵼����Ҫ����
	if( old_AudioCfg.isChanged( this->m_AudioCfg )
		|| old_port != this->GetPort()
		|| old_channel != this->GetChannel()
		|| old_streamfmt != this->GetStreamfmt()
		|| old_ipaddress != this->GetIPAddress()
		|| old_username != this->GetUsername()
		|| old_password != this->GetPassword()
		)
	{
		return true;
	}

	return false;
}

bool IPCameraBase::isRight_manual_trace()
{
	if( !m_AdvAttr.get_AdvAttr( defCamAdvAttr_motion_track )
		|| !m_AdvAttr.get_AdvAttr( defCamAdvAttr_manual_trace )
		|| !GetPTZState( GSPTZ_MOTION_TRACK_Enable, false )
		)
	{
		return false;
	}

	return true;
}

void IPCameraBase::ConnectBefore()
{
	CAudioCfg::struAudioParam ap;
	memset( &ap, 0, sizeof(ap) );
	m_AudioCfg.set_AudioParam_Analyse( ap );
	this->UpdateAudioCfg();

	//if( defUseable_ErrNoData != this->get_NetUseable() )
	{
		this->SetFrameState( true );
	}
}

void IPCameraBase::ConnectAfter()
{
	if( !this->GetStreamObj()->GetRTMPSendObj()->GetIPlayBack() )
	{
		if( IsConnect() )
		{
			//if( defUseable_ErrNoData != this->get_NetUseable() )
			{
				this->check_NetUseable();//this->set_NetUseable( defUseable_OK ); // ���ܼ򵥸�ֵ����ͬ��������������һ���ж�
			}

			this->GetPTZState( GSPTZ_MOTION_TRACK_Enable, true );
			this->GetPTZState( GSPTZ_PTZ_ParkAction_Enable, true );
		}
		else
		{
			if( m_camera_hwnd>=0 )
			{
				if( defUseable_ErrNoData != this->get_NetUseable() )
				{
					this->set_NetUseable( defUseable_Err );
				}
			}
			else
			{
				this->set_NetUseable( defUseable_ErrNet );
			}
		}
	}
}

bool IPCameraBase::isTalkUseCam() const
{
	return ( m_AdvAttr.get_AdvAttr( defCamAdvAttr_CamTalk ) && IsRUNCODEEnable(defCodeIndex_SYS_CamTalk) );
}

bool IPCameraBase::isTalkSupport() const
{
	switch( GetCameraType() )
	{
	case CameraType_hik:
		break;

	default:
		return false;
	}

	return true;
}

bool IPCameraBase::isAutoConnect() const
{
	switch( RUNCODE_Get(defCodeIndex_SYS_CamAutoConnect) )
	{
	case 1:
		{
			return true; // �����Զ�����
		}
		break;

	case 2:
		{
			return false; // ���ǰ�������
		}
		break;
	}

	return m_AdvAttr.get_AdvAttr( defCamAdvAttr_AutoConnect );
}

defUseable IPCameraBase::set_NetUseable( defUseable NetUseable, bool *isChanged  )
{
	gloox::util::MutexGuard mutexguard( m_mutex_sessionList );

	if( m_NetUseable != NetUseable )
	{
		if( isChanged )
		{
			*isChanged = true;
		}

		LOGMSG( "cam(%s) set_NetUseable old=%d, new=%d\r\n", m_name.c_str(), m_NetUseable, NetUseable );
	}
	else
	{
		if( isChanged )
		{
			*isChanged = false;
		}
	}

	m_NetUseable = NetUseable;

	return m_NetUseable;
}

defUseable IPCameraBase::get_NetUseable()
{
	gloox::util::MutexGuard mutexguard( m_mutex_sessionList );

	return m_NetUseable;
}

bool IPCameraBase::GetFrameState()
{
	const uint32_t curtime = timeGetTime();

	gloox::util::MutexGuard mutexguard( m_mutex_sessionList );

	return ( curtime-m_hasFrameTime < 15000 );
}

void IPCameraBase::SetFrameState( bool state )
{
	const uint32_t curtime = timeGetTime();

	gloox::util::MutexGuard mutexguard( m_mutex_sessionList );

	if( state )
	{
		m_hasFrameTime = curtime;
	}
	else
	{
		m_hasFrameTime = curtime-999000;
	}
}

defUseable IPCameraBase::get_all_useable_state()
{
	if( InvalidVideoBitrate() )
	{
		return defUseable_ErrConfig;
	}

	defUseable useable = defUseable_Err;

	const bool isConnect = this->IsConnect();
	const defUseable NetUseable = this->get_NetUseable();

	// �����Ӳ��������
	if( isConnect && defUseable_ErrNoData != NetUseable )
	{
		const bool isFrameState = this->GetFrameState();
		if( isFrameState )
		{
			useable = defUseable_OK;
		}
	}
	
	if( defUseable_OK != useable )
	{
		if( defUseable_OK == NetUseable )
		{
			const bool isAutoConnectFlag = this->isAutoConnect();
			const uint32_t SessionCount = this->GetSessionCount();

			if( isAutoConnectFlag || SessionCount>0 )
			{
				useable = isConnect ? defUseable_ErrNet:defUseable_ErrNoData;
			}
			else
			{
				useable = defUseable_OK;
			}
		}
		else
		{
			useable = NetUseable;
		}

		if( defUseable_ErrNoData == useable )
		{
			if( defUseable_ErrNoData != NetUseable )
			{
				this->set_NetUseable( useable );
			}
		}
	}

	return useable;
}

bool IPCameraBase::InvalidVideoBitrate() const
{
	const uint32_t Cam_VideoBitrateLimit = RUNCODE_Get( defCodeIndex_Cam_VideoBitrateLimit );

	return ( m_curVideoBitrate > Cam_VideoBitrateLimit );
}

// ���õ�ǰ�澯״̬
bool IPCameraBase::SetCurAlarmState( const defAlarmState CurAlarmState )
{
	bool isChanged = false;
	if( CurAlarmState != m_CurAlarmState )
	{
		LOGMSG( "SetCurAlarmState(%s) (old=%d,new=%d)\r\n", this->m_name.c_str(), m_CurAlarmState, CurAlarmState );

		isChanged = true;
		m_CurAlarmState = CurAlarmState;
	}

	m_lastupdate_CurAlarmState = timeGetTime();
	return isChanged;
}

// �澯���³�ʱ��ָ��澯״̬
bool IPCameraBase::CheckResumeCurAlarmState( uint32_t enable )
{
	if( macAlarmState_IsAlarm(m_CurAlarmState) )//if( !macAlarmState_IsOK(m_CurAlarmState) )
	{
		const uint32_t SYS_CamAlarmResumeTime = RUNCODE_Get(defCodeIndex_SYS_CamAlarmResumeTime);

		if( !enable
			|| !this->GetAdvAttr().get_AdvAttr(defCamAdvAttr_SupportAlarm)
			|| timeGetTime()-m_lastupdate_CurAlarmState > SYS_CamAlarmResumeTime * 1000
			)
		{
			LOGMSG( "CheckResumeCurAlarmState(%s) AlarmResume(old=%d,new=%d)(%ds)\r\n", this->m_name.c_str(), m_CurAlarmState, defAlarmState_OK, SYS_CamAlarmResumeTime );

			this->SetCurAlarmState( defAlarmState_OK );//�ָ���
			return true;
		}
	}

	return false;
}

void IPCameraBase::SetPrePicChangeCode( const std::string &PrePicChangeCode )
{
	gloox::util::MutexGuard mutexguard( m_mutex_sessionList );

	m_PrePicChangeCode = PrePicChangeCode;
}

std::string IPCameraBase::GetPrePicChangeCode()
{
	gloox::util::MutexGuard mutexguard( m_mutex_sessionList );

	return m_PrePicChangeCode;
}

bool IPCameraBase::SavePrePicChangeInfo( const int camid, const std::string &PrePicChangeCode )
{
	if( !PrePicChangeCode.empty() )
	{
		const std::string PrePicChangeInfo = std::string( "chg=" ) + PrePicChangeCode;
		return g_SaveStringToFile( g_createPicPre_InfoFileFullPathName( IOT_DEVICE_Camera, camid ), PrePicChangeInfo );
	}

	return false;
}


bool IPCameraBase::LoadPrePicChangeInfo( const int camid, std::string &PrePicChangeCode )
{
	//FILE *fd = NULL; //jyc20170331 modify
	//fopen( &fd, g_createPicPre_InfoFileFullPathName( IOT_DEVICE_Camera, camid ).c_str(), "rb+" );
	FILE *fd;
	fd=fopen( g_createPicPre_InfoFileFullPathName( IOT_DEVICE_Camera, camid ).c_str(), "rb+" );
	if( !fd )
	{
		return false;
	}

	fseek( fd, 0, SEEK_END );
	const int nFileLen = ftell( fd );
	fseek( fd, 0, SEEK_SET );

	if( nFileLen < 1 )
	{
		PrePicChangeCode = "";
		fclose( fd );
		return true;
	}

	//����
	if( nFileLen > 1024 )
	{
		fclose( fd );
		return false;
	}

	const int buflen = nFileLen + 3;

	char *bufsrc = new char[buflen];
	memset( bufsrc, 0, buflen );
	size_t ret=0;

	ret = fread( bufsrc, 1, nFileLen, fd );

	if( ferror( fd ) )
	{
		fclose( fd );
		delete[]bufsrc;
		return false;
	}

	fclose( fd );
	bufsrc[nFileLen] = 0;

	std::map<std::string, std::string> mapval;
	split_getmapval( bufsrc, mapval );

	PrePicChangeCode = "";
	for( std::map<std::string, std::string>::const_iterator it=mapval.begin(); it!=mapval.end(); ++it )
	{
		if( it->first == "chg" )
		{
			PrePicChangeCode = it->second;
		}
	}
	return true;
}
