#include "GsCamera.h"

defGSReturn hikErr2GSErr(const DWORD nErr) {
	return defGSReturn_Err; // 无明确匹配的错误码，具体错误需要再根据其它判断
}

uint32_t hikTransVideoBitrate(const DWORD dwVideoBitrate) {
	/*
	 视频码率:0-保留，1-16K(保留)，2-32K，3-48k，4-64K，5-80K，6-96K，7-128K，8-160k，9-192K，10-224K，
	 11-256K，12-320K，13-384K，14-448K，15-512K，16-640K，17-768K，18-896K，19-1024K，20-1280K，21-1536K，22-1792K，23-2048K，
	 24-3072K，25-4096K，26-8192K，27-16384K。最高位(31位)置成1表示是自定义码流，0～30位表示码流值，最小值16k,0xfffffffe，自动，和源一致
	 */

	switch (dwVideoBitrate) {
	case 1:
		return 16000;
	case 2:
		return 32000;
	case 3:
		return 48000;
	case 4:
		return 64000;
	case 5:
		return 80000;
	case 6:
		return 96000;
	case 7:
		return 128000;
	case 8:
		return 160000;
	case 9:
		return 192000;
	case 10:
		return 224000;
	case 11:
		return 256000;
	case 12:
		return 320000;
	case 13:
		return 384000;
	case 14:
		return 448000;
	case 15:
		return 512000;
	case 16:
		return 640000;
	case 17:
		return 760000;
	case 18:
		return 896000;
	case 19:
		return 1024000;
	case 20:
		return 1280000;
	case 21:
		return 1536000;
	case 22:
		return 1792000;
	case 23:
		return 2048000;
	case 24:
		return 3072000;
	case 25:
		return 4096000;
	case 26:
		return 8192000;
	case 27:
		return 16384000;
	default: {
		if (dwVideoBitrate & 0x80000000) {
			return (dwVideoBitrate & 0x7FFFFFFF);
		}
	}
		break;
	}

	return 0;
}

//static bool getAlarmStr_MessageCallback(
//		std::string &alarmstr,
//		const LONG lCommand,
//		const defNS_HIK::NET_DVR_ALARMER *pAlarmer,
//		const char *pAlarmInfo,
//		const DWORD dwBufLen )
//{
//	return false;
//}

//void CALLBACK MessageCallback(LONG lCommand, defNS_HIK::NET_DVR_ALARMER *pAlarmer, char *pAlarmInfo, DWORD dwBufLen, void* pUser)
//{
//
//}

//void CALLBACK g_ExceptionCallBack(DWORD dwType, LONG lUserID, LONG lHandle, void *pUser)
//{
//	switch(dwType)
//	{
//	case EXCEPTION_RECONNECT:    //预览时重连
//		LOGMSGEX( defLOGNAME, defLOG_WORN, "----------HIK reconnect--------\r\n");
//		break;
//	default:
//		break;
//	}
//}

//void CALLBACK DataCallBack(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer,DWORD dwBufSize,DWORD lUser)
void DataCallBack(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer,
		DWORD dwBufSize, void* lUser) {
	if (0 == lUser) {
		return;
	}

	GsCamera *Camera = (GsCamera*) lUser;
	Camera->OnData(pBuffer, dwBufSize);
}

//void CALLBACK PlayDataCallBack_V40( LONG lPlayHandle, DWORD dwDataType, BYTE *pBuffer,DWORD dwBufSize,void *pUser )
//{
//	if( 0 == pUser )
//	{
//		return;
//	}
//
//	HikCamera *Camera = (HikCamera*)pUser;
//	Camera->OnData( lPlayHandle, dwDataType, pBuffer, dwBufSize );
//}
/*静态函数*/
void GsCamera::InitSDK(ICameraHandler *handler) {

}

void GsCamera::Cleanup() {
}

bool GsCamera::GetChannelNum(char *sDVRIP, uint16_t wDVRPort, char *sUserName,
		char *sPassword, uint8_t &byChanNum, uint8_t &byStartChan,
		std::string &strErr) {
	char chbuf[256] = { 0 };

	byChanNum = 0;
	byStartChan = 0;
	strErr = "";

	return true;
}

bool GsCamera::GetCameraDevInfo(char *sDVRIP, uint16_t wDVRPort,
		char *sUserName, char *sPassword, const std::string &devname,
		std::string &strCamSerialNo, defvecChInfo &vecChInfo,
		std::string &strErr, const uint32_t streamfmt) {
	char chbuf[256] = { 0 };

	strErr = "";
	return true;
}

defGSReturn GsCamera::SetCamTime_Spec(const struGSTime &newtime,
		const std::string &name, const std::string &sIP, uint16_t wPort,
		const std::string &sUserName, const std::string &sPassword) {
	printf("......GsCamera::SetCamTime_Spec.....\n");
	defGSReturn gsret = defGSReturn_Err;
	return gsret;
}

defGSReturn GsCamera::SetCamTime_ForLoginID(const struGSTime &newtime,
		const std::string &name, const long LoginID, const std::string &sIP,
		const uint16_t wPort) {
	printf("......GsCamera::SetCamTime_ForLoginID.....\n");
	defGSReturn gsret = defGSReturn_Err;
	return defGSReturn_ConnectObjErr;
}

GsCamera::GsCamera(const std::string& deviceID, const std::string& name,
		const std::string& ipaddress, uint32_t port,
		const std::string& username, const std::string& password,
		const std::string& ver, GSPtzFlag PTZFlag, GSFocalFlag FocalFlag,
		uint32_t channel, uint32_t streamfmt) :
		IPCameraBase(deviceID, name, ipaddress, port, username, password, ver,
				PTZFlag, FocalFlag, channel, streamfmt) {
	printf("......GsCamera::GsCamera.....\n");
	this->m_camera_hwnd = -1;
	this->m_playhandle = -1;
	this->m_AlarmChan = -1;
	this->m_isFirstAudio = true;
	this->m_AudioEncType = -1;
	this->m_pDecHandle = (void*) -1;

	this->m_lVoiceComHandle = -1;

	//memset( &m_sptr, 0, sizeof(m_sptr) ); //jyc20170512 remove
	//memset( &m_frame, 0, sizeof(m_frame) );

	m_timeMakeKeyFrame = timeGetTime() - 60000;
}

GsCamera::~GsCamera(void) {
	printf("......GsCamera::~GsCamera.....\n");
}

void GsCamera::PreinitBufCheck() {
	printf("......GsCamera::PreinitBufCheck.....\n");
}

bool GsCamera::TestConnect(std::string &strErr) {
	printf("......GsCamera::TestConnect.....\n");
	return true;
}

defGSReturn GsCamera::SetCamTime(const struGSTime &newtime) {
	printf("......GsCamera::SetCamTime.....\n");
	this->Login(false);
	return SetCamTime_ForLoginID(newtime, m_name, m_camera_hwnd,
			this->ConnUse_ip(), this->ConnUse_port());
}

defGSReturn GsCamera::Login(bool playback) {
	printf("......GsCamera::Login.....\n");
	return defGSReturn_Success;
}

// 连接建立告警通道
void GsCamera::ConnectAlarmGuard() {
	printf("......GsCamera::ConnectAlarmGuard.....\n");
}

// 连接建立告警通道
void GsCamera::DisconnectAlarmGuard() {
	printf("......GsCamera::DisconnectAlarmGuard.....\n");
}

void GsCamera::Logout() {
	printf("......GsCamera::Logout.....\n");
}

defGSReturn GsCamera::Connect(bool reconnectAudio, void* RealPlayWnd,
		bool playback, const struGSTime *pdtBegin, const struGSTime *pdtEnd) {
	printf("......GsCamera::Connect.....\n");
	ConnectBefore();
	if (this->IsConnect()) {
		this->OnDisconnct();
	}
	this->m_state.Reset();
	this->m_state.startTime = timeGetTime();

	this->m_camera_hwnd = 1;
	this->m_playhandle = 1;	//jyc20170512 add to debug

	defGSReturn ret = this->Login(playback);
	if (macGSFailed(ret)) {
		ConnectAfter();
		return ret;
	}

	LOGMSG("Cam:Connect(%s,%s:%d) Success. LoginID=%d\r\n",
			m_name.c_str() ? m_name.c_str() : "",
			(char *) this->ConnUse_ip(playback).c_str(),
			this->ConnUse_port(playback), this->m_camera_hwnd);

	m_isPlayBack = playback;

	if (m_isPlayBack) {
		return PlayBack(RealPlayWnd, pdtBegin, pdtEnd);
	}
	InitAudioEncType();
	ret = this->SetChannelStream(m_channel, reconnectAudio, RealPlayWnd);

	ConnectAfter();
	return ret;
}

bool GsCamera::isSupportAudioEncType(int AudioEncType) {
	return false;
}

bool GsCamera::InitAudioEncType() {
	printf("......GsCamera::InitAudioEncType.....\n");
	return true;
}


defGSReturn GsCamera::SetChannelStream(int channel, bool reconnectAudio,
		void* RealPlayWnd) {
	printf("......GsCamera::SetChannelStream.....\n");
	return defGSReturn_Success;
}

defGSReturn GsCamera::PlayBack(void* RealPlayWnd, const struGSTime *pdtBegin,
		const struGSTime *pdtEnd) {
	printf("......GsCamera::PlayBack.....\n");
	return defGSReturn_Success;
}

defGSReturn GsCamera::QuickSearchPlayback(const struGSTime *pdtBegin,
		const struGSTime *pdtEnd) {
	if (!pdtBegin || !pdtEnd)
		return defGSReturn_Err;
	return defGSReturn_Err;
}

int GsCamera::PlayBackControl(GSPlayBackCode_ ControlCode, void *pInBuffer,
		uint32_t InLen, void *pOutBuffer, uint32_t *pOutLen) {
	printf("......GsCamera::PlayBackControl.....\n");
	return -1;
}

void GsCamera::OnDisconnct() {
	//printf("......GsCamera::OnDisconnct.....\n");
}

bool GsCamera::IsConnect() {
	//printf("......GsCamera::IsConnect.....\n");
	return true; //jyc20170515 debug
	if (m_camera_hwnd >= 0 && m_playhandle >= 0) {
		return true;
	}
	return false;
}

void GsCamera::ResetBuf() {
}

void GsCamera::SearchInit(ICameraHandler *handler) {
}

void GsCamera::SearchStart() {
}

void GsCamera::SearchStop() {
}

bool GsCamera::DealwithAlarm(char* pBuf, unsigned long dwLen) {
	return true;
}

// speed : 1-7
bool GsCamera::SendPTZ(GSPTZ_CtrlCmd command, int param1, int param2, int speed,
		const char *callinfo) {
	return true;
}

int GsCamera::GetPTZState(GSPTZ_CtrlCmd command, bool refresh_device) {
	return 0;
}

bool GsCamera::Set_motion_track(const bool enable, const bool srccall) {
	return true;
}

bool GsCamera::Set_PTZ_ParkAction(const bool enable, const bool srccall) {
	printf("......GsCamera::Set_PTZ_ParkAction.....\n");
	return true;
}

bool GsCamera::PresetControl(DWORD dwPTZPresetCmd, DWORD dwPresetIndex,
		const std::string &callinfo) {
	return true;
}

void GsCamera::OnPublishStart() {
	printf("......GsCamera::OnPublishStart.....\n"); //notice
	//MakeKeyFrame();
}

void GsCamera::MakeKeyFrame(bool forcenow) {

}

void GsCamera::OnData(BYTE *pBuffer, DWORD dwBufSize) {

}

void GsCamera::doAudioSource_CameraReEnc(unsigned char *buffer, int size,
		const uint32_t sendFrameTS) {
}

bool GsCamera::isTalkSupport() const {
	return false;
}

bool GsCamera::OnTalkSend(defTalkSendCmd_ TalkSendCmd, unsigned char *pdata,
		uint32_t data_size) {
	return true;
}

void GsCamera::check_NetUseable(bool *isChanged) {
}

defGSReturn GsCamera::GetCamShowInfo(int &showtime, int &showname,
		std::string &name) {
	printf("......GsCamera::GetCamShowInfo.....\n");
	return defGSReturn_Err;
}

defGSReturn GsCamera::SetCamShowInfo(const int showtime, const int showname,
		const char *pname) {
	printf("......GsCamera::SetCamShowInfo.....\n");
	return defGSReturn_Err;
}

defGSReturn GsCamera::GetCamCompressCfg(std::string *cfginfo,
		struCamParamBuf *getCamParamBuf) {
	printf("......GsCamera::GetCamCompressCfg.....\n");
	return defGSReturn_Success;
}

// setuseCamParamBuf有值时，使用此覆盖整个结构进行设置
defGSReturn GsCamera::SetCamCompressCfg(const defCamCompress CamCompress,
		const struCamParamBuf *setuseCamParamBuf) {
	printf("......GsCamera::SetCamCompressCfg.....\n");
	return defGSReturn_Success;
}

bool GsCamera::CapturePic(char *buf, DWORD bufsize, DWORD &outsize,
		DWORD PicSize) {
	printf("......GsCamera::CapturePic.....\n");
	return false;
}

