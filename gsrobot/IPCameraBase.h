#ifndef IPCAMERABASE_H_
#define IPCAMERABASE_H_

#include "gloox/jid.h"
#include "RTMPSession.h"
#include "MediaStream.h"
#include "CameraControl.h"
#include "IPublishHandler.h"
#include "gloox/mutexguard.h"
#include "audio/AudioCapBase.h"
#include <bitset>
#include "PresetManager.h"

//��Чͨ����
#define defInvalidCamCh		0
#define isInvalidCamCh(ch)	(defInvalidCamCh==ch)

// ��Ƶ�������������׼���õȼ�
enum defCamCompress
{
	defCamCompress_720PLow = 1,		// 720P ��
	defCamCompress_720PNorm,		// 720P ����
	defCamCompress_720PRelaHigh,	// 720P �Ϻ�
	defCamCompress_720PHigh,		// 720P ��
	defCamCompress_1080PLow,		// 1080P ��
	defCamCompress_1080PNorm,		// 1080P ��
};

// ��������ͨ�û��棬���ڽ�һ�����Ƹ���һ���м�ת��ʱʹ�ã��ȵ�
struct struCamParamBuf
{
	IPCameraType CamType;	// ���������
	uint32_t ParamType;		// ��ݲ�ͬ��������Ͳ��������ڲ����
	uint16_t ParamLen;
	uint8_t *ParamBuf;

	struCamParamBuf()
	{
		CamType= CameraType_Unkown;
		ParamType = 0;
		ParamLen = 0;
		ParamBuf = NULL;
	}

	~struCamParamBuf()
	{
		ReleaseBuf();
	}

	void ReleaseBuf()
	{
		if( ParamLen>0 && ParamBuf )
		{
			delete[]ParamBuf;
		}

		ParamLen = 0;
		ParamBuf = NULL;
	}

	bool IsValidBuf( const IPCameraType in_CamType, const uint32_t in_ParamType ) const
	{
		return ( in_CamType==CamType && in_ParamType==ParamType && ParamLen>0 && ParamBuf );
	}

	void SetBuf( const IPCameraType in_CamType, const uint32_t in_ParamType, const uint16_t in_ParamLen, const uint8_t *in_ParamBuf )
	{
		ReleaseBuf();

		CamType = in_CamType;
		ParamType = in_ParamType;
		ParamLen = in_ParamLen;
		ParamBuf = new uint8_t[ParamLen];
		memcpy( ParamBuf, in_ParamBuf, ParamLen );
	}

	bool GetBuf( const IPCameraType in_CamType, const uint32_t in_ParamType, const uint16_t in_ParamLen, uint8_t *in_ParamBuf ) const
	{
		if( !IsValidBuf( in_CamType, in_ParamType ) )
			return false;

		if( in_ParamLen < ParamLen )
			return false;

		//in_ParamLen = ParamLen;
		memcpy( in_ParamBuf, ParamBuf, in_ParamLen );
	}
};

struct struChInfo
{
	uint32_t channel;
	uint32_t streamfmt;
	std::string strCh;
	std::string name;
	std::string desc;

	struChInfo()
	{
		channel = -1;
		streamfmt = 0;
	}
};
typedef std::vector<struChInfo> defvecChInfo;

// ʱ�������ģʽ
enum defFixTSMod
{
	defFixTSMod_NoFix	= 0, // ������
	defFixTSMod_Manual	= 1, // �ֶ��̶������������
	defFixTSMod_Auto	= 2  // �Զ���������������
};

static const char* s_name_defFixTSMod[] =
{
	"NoFix",
	"Manual",
	"Auto"
};


#define defDefault_CameraFixParm_UPPER	1000
#define defDefault_CameraFixParm_LOWER	10

// �������Ĭ��ֵ����10֡100Ϊ����
#define defDefault_CameraFixParm_span_Min	50
#define defDefault_CameraFixParm_span_Max	120
#define defDefault_CameraFixParm_fix_Min	40
#define defDefault_CameraFixParm_fix_Max	110
#define defDefault_CameraFixParm_spanWarn	350

//��̬���ޣ���ֵ�İٷֱȣ�����120��ʾ��ֵ�İٷ�֮120
#define defDefault_CameraFixParm_mfactor_span_Min	50
#define defDefault_CameraFixParm_mfactor_span_Max	120
#define defDefault_CameraFixParm_mfactor_fix_Min	40
#define defDefault_CameraFixParm_mfactor_fix_Max	110
#define defDefault_CameraFixParm_mfactor_spanWarn	500


// ��Ƶ�ɼ��������Ĭ��ֵ��defDefault_CameraFixParm_
struct struCameraFixParm
{
	defFixTSMod	FixTSMod;

	uint32_t	parmFps; // �����Ӧ��fpsֵ��δ֪д0
	uint32_t	lasttime_Recal; // ���һ�μ����ʱ��

	// �ֶ�ģʽʱ�����ù̶�ֵ���Զ�ģʽʱ��ݳ�ϵ��̬����
	uint32_t	span_Min; // ֡����������
	uint32_t	span_Max; // ֡����������
	uint32_t	fix_Min;  // ֡���������Сֵ
	uint32_t	fix_Max;  // ֡����������ֵ
	uint32_t	spanWarn; // ֡����Ƚϴ�ʱ��ӡ������־������ֵ

	// �Զ�ģʽʱ���ڶ�̬����
	uint32_t	mfactor_span_Min; // ֡���������޳�ϵ��ٷֱȣ�����120��ʾ�ٷ�֮120
	uint32_t	mfactor_span_Max; // ֡���������޳�ϵ��ٷֱ�
	uint32_t	mfactor_fix_Min;  // ֡���������Сֵ��ϵ��ٷֱ�
	uint32_t	mfactor_fix_Max;  // ֡����������ֵ��ϵ��ٷֱ�
	uint32_t	mfactor_spanWarn; // ֡����Ƚϴ�ʱ��ӡ������־������ֵ��ϵ��ٷֱ�

	void		*exparam; // ��չ����

	struCameraFixParm(void)
	{
		Reset();
	}

	~struCameraFixParm(void)
	{
	}

	void Reset();
	void Recal( uint32_t cur_fps, uint32_t timespan, const char *loginfo=NULL, bool isForceRecal=false );
	void CheckParam();
	void print( const char *pinfo=NULL );
};

// ��Ƶ�ɼ�״̬
struct struCameraState
{
	bool		isSetFirst;
	uint32_t	startTime;	// ��ʼʱ��
	uint32_t	sno;		// ֡��ż�����յ��ĵ�һ��֡��ʼ���������Ƿ���Ч
	uint32_t	validPktCount;  // ��Ч֡�İ��ܼ���
	uint32_t	prev_FPS;	// ��һ֡FPS
	uint32_t	prev_ts;	// ��һ֡ʱ���
	uint32_t	prev_tskey;	// ��һ�ؼ�֡ʱ���
	uint32_t	prev_time;	// ��һ֡ʱ��
	void		*exparam;	// ��չ����

	struCameraState(void)
	{
		Reset();
	}

	~struCameraState(void)
	{
	}

	void Reset();
};

using namespace gloox;

class CRecCfg
{
public:
	CRecCfg(void) 
		: m_rec_mod(defRecMod_NoRec), m_rec_svrtype(CameraType_Unkown), m_rec_port(0), m_rec_channel(0)
	{};

	~CRecCfg(void){};


	void setrec_mod( defRecMod rec_mod )
	{
		this->m_rec_mod = rec_mod;
	}
	defRecMod getrec_mod() const
	{
		return this->m_rec_mod;
	}
	void setrec_svrtype( IPCameraType rec_svrtype )
	{
		this->m_rec_svrtype = rec_svrtype;
	}
	IPCameraType getrec_svrtype() const
	{
		return this->m_rec_svrtype;
	}
	const std::string& getrec_ip() const
	{
		return this->m_rec_ip;
	}
	void setrec_ip( const std::string &rec_ip )
	{
		m_rec_ip = rec_ip;
	}

	void setrec_port( uint32_t rec_port )
	{
		this->m_rec_port = rec_port;
	}
	uint32_t getrec_port() const
	{
		return this->m_rec_port;
	}

	const std::string& getrec_username() const
	{
		return this->m_rec_username;
	}
	void setrec_username( const std::string &rec_username )
	{
		m_rec_username = rec_username;
	}

	const std::string& getrec_password() const
	{
		return this->m_rec_password;
	}
	void setrec_password( const std::string &rec_password )
	{
		m_rec_password = rec_password;
	}

	void setrec_channel( uint32_t rec_channel )
	{
		this->m_rec_channel = rec_channel;
	}
	uint32_t getrec_channel() const
	{
		return this->m_rec_channel;
	}

protected:
	// ¼��¼���ͨ��ӳ����Ϣ
	defRecMod m_rec_mod;
	IPCameraType m_rec_svrtype; // ¼�������
	std::string m_rec_ip;
	uint32_t m_rec_port;
	std::string m_rec_username;
	std::string m_rec_password;
	uint32_t m_rec_channel;

private:
};


// ������Чλλ��
// ��m_aAttr�е�λ��
// ע��˳�������ô洢�йأ��汾ȷ����ֻ��������ӣ�����������ԭ��˳��
enum defCamAdvAttr
{
	defCamAdvAttr_Min_ = 0,
	defCamAdvAttr_motion_track = 0,		// �Ƿ�֧�����ܸ���
	defCamAdvAttr_manual_trace,			// �Ƿ�֧���ֶ�����
	defCamAdvAttr_manual_ptzsel,		// �Ƿ�֧���ֶ���λ
	defCamAdvAttr_CamTalk,				// �Ƿ�֧�������Խ�
	defCamAdvAttr_AutoConnect,			// �Ƿ񱣳��Զ�����
	defCamAdvAttr_PTZ_ParkAction,		// �Ƿ�֧��������
	defCamAdvAttr_PTZ_ParkActionKeep,	// �Ƿ��ڿ���ʱ��������
	defCamAdvAttr_PTZ_Preset,			// �Ƿ�֧��Ԥ�õ�
	defCamAdvAttr_manual_zoomrng,		// �Ƿ�֧������ѡ������
	defCamAdvAttr_SupportAlarm,			// �Ƿ���и澯����
	defCamAdvAttr_PTZ_Ang,				// �Ƿ������̨б�Ƿ�����ƹ���
	defCamAdvAttr_AutoPublish,			// �Ƿ񱣳ַ���/�Զ�����
	defCamAdvAttr_Max_
};

class CCamAdvAttr
{
public:
	CCamAdvAttr(void)
		: m_AdvAttribute(0)
	{};

	~CCamAdvAttr(void){};

	// �߼����Բ��� m_AdvAttribute
	static bool isShow_AdvAttr( defCamAdvAttr attr );
	static const char* get_AdvAttr_Name( defCamAdvAttr attr );

	bool get_AdvAttr( defCamAdvAttr attr ) const;
	bool set_AdvAttr( defCamAdvAttr attr, bool val );
	uint32_t get_AdvAttr_uintfull() const;
	bool set_AdvAttr_uintfull( uint32_t fullval );

private:
	std::bitset<defCamAdvAttr_Max_> m_AdvAttribute;
};

class IPCameraBase:public IPublishHandler,public CameraControl, public AudioCapCallBack, public CPresetManager, public AGRunStateBase
{
protected:
	CRecCfg m_RecCfg;
	CAudioCfg m_AudioCfg;
	CCamAdvAttr m_AdvAttr;

	AudioCapBase* m_pLocalCapAudio;

	std::string m_name;
	std::string m_deviceID;

	uint32_t m_port;
	std::string m_ipaddress;
	std::string m_username;
	std::string m_password;

	uint32_t m_channel;
	uint32_t m_streamfmt;

	long m_camera_hwnd;
	long m_playhandle;
	bool m_isPlayBack;
	MediaStream *stream;

	struCameraFixParm m_fixparm;
	struCameraState m_state;
	
	uint32_t m_prev_tick_PrintFrame;
	uint32_t m_prev_tick_PrintFrame_Audio;

	// PrePic
	std::string m_PrePicChangeCode;

	// state
protected:
	BYTE m_EnableTrack;   //���ø��٣�0-��1-��
	BYTE m_PTZ_ParkAction; // ��������0-��1-��
	
public:
	IPCameraBase(const std::string& deviceID, const std::string& name,
		const std::string& ipaddress,uint32_t port,const std::string& username,
		const std::string& password, const std::string& ver, GSPtzFlag PTZFlag, GSFocalFlag FocalFlag,
		uint32_t channel, uint32_t streamfmt );
	~IPCameraBase(void);

	// enable�ֶ�ֵ���
	static void uncode_enable_col_save( const int enable_col_save, int &enable_val, int &AGRunState_val )
	{
		enable_val = enable_col_save & 0x01;					// ���ñ�־
		AGRunState_val = (enable_col_save & 0x02) >> 1;	//��������־
	}

	// �ϲ����enable�ֶ�ֵ
	static int encode_enable_col_save( const int enable_val, const int AGRunState_val )
	{
		const int AGRunState_comb = (AGRunState_val & 0x01) << 1;
		const int enable_comb = enable_val & 0x01;

		return (AGRunState_comb+enable_comb);
	}

	bool doEditAttrFromAttrMgr( const EditAttrMgr &attrMgr, defCfgOprt_ oprt = defCfgOprt_Modify );

	virtual void OnAudioBufferCB( double SampleTime, unsigned char *pBuffer, long BufferLen );

	CRecCfg& GetRecCfg()
	{
		return m_RecCfg;
	};

	void UpdateReccfg( const CRecCfg& RecCfg )
	{
		m_RecCfg = RecCfg;
	};

	CAudioCfg& GetAudioCfg()
	{
		return m_AudioCfg;
	};
	
	void UpdateAudioCfg()
	{
		//this->GetStreamObj()->GetRTMPSendObj()->UpdateAudioCfg( m_AudioCfg );
	};

	void UpdateAudioCfg( const CAudioCfg& AudioCfg )
	{
		m_AudioCfg = AudioCfg;
		this->UpdateAudioCfg();
	};

	CCamAdvAttr& GetAdvAttr()
	{
		return m_AdvAttr;
	};

	const CCamAdvAttr& GetAdvAttrSafe() const
	{
		return m_AdvAttr;
	};

	void UpdateAdvAttr( const CCamAdvAttr& AdvAttr )
	{
		m_AdvAttr = AdvAttr;
	};

	bool isChanged( 
		const std::string &old_ipaddress, 
		const uint32_t old_port, 
		const std::string &old_username, 
		const std::string &old_password, 
		const uint32_t old_channel, 
		const uint32_t old_streamfmt,
		const CAudioCfg &old_AudioCfg ) const;

	bool isRight_manual_trace();

	bool ReCreateLocalCapAudio();
	bool StartLocalCapAudio();
	bool StopLocalCapAudio();

    virtual void OnPublishStart();
	virtual void OnPublishStop(defGSReturn code);
	virtual uint32_t OnPublishUpdateSession( const std::string &strjid, bool *isAdded=NULL );
	
	void SetDeviceId( const std::string &deviceID )
	{
		this->m_deviceID = deviceID;
	}

	const std::string& GetDeviceId()
	{
		return this->m_deviceID;
	}

	const std::string& GetName() const
	{
		return this->m_name;
	}

	void SetName( const std::string &name )
	{
		m_name = name;
	}

	void SetIPAddress( std::string& ipaddress )
	{
		this->m_ipaddress = ipaddress;
	}
	const std::string& GetIPAddress() const
	{
		return this->m_ipaddress;
	}

	void setPort( uint32_t port )
	{
		this->m_port = port;
	}
	uint32_t GetPort() const
	{
		return this->m_port;
	}

	void SetUsername( std::string &username )
	{
		this->m_username = username;
	}
	const std::string& GetUsername() const
	{
		return this->m_username;
	}

	void SetPassword( std::string &password )
	{
		this->m_password = password;
	}
	const std::string& GetPassword() const
	{
		return this->m_password;
	}


	long GetCameraHWND()
	{
		return this->m_camera_hwnd;
	}

	MediaStream* GetStreamObj( )
	{
		return  this->stream;
	}

	struCameraFixParm& GetFixParmObj()
	{
		return  this->m_fixparm;
	}

	struCameraState& GetStateObj()
	{
		return  this->m_state;
	}
	
	void SetChannel( uint32_t channel )
	{
		m_channel = channel;
	}

	uint32_t GetChannel() const
	{
		return this->m_channel;
	}

	void SetStreamfmt( uint32_t streamfmt )
	{
		m_streamfmt = streamfmt;
	}

	uint32_t GetStreamfmt() const
	{
		return this->m_streamfmt;
	}


	bool isGetSelf( bool playback ) const
	{
		if( playback )
		{
			if( defRecMod_OnReordSvr==this->m_RecCfg.getrec_mod() )
			{
				return false;
			}

			return true;
		}

		return true;
	}
	const std::string& ConnUse_ip( bool playback=false ) const
	{
		if( isGetSelf(playback) )
			return m_ipaddress;

		return this->m_RecCfg.getrec_ip();
	}
	uint32_t ConnUse_port( bool playback=false ) const
	{
		if( isGetSelf(playback) )
			return m_port;

		return this->m_RecCfg.getrec_port();
	}
	const std::string& ConnUse_username( bool playback=false ) const
	{
		if( isGetSelf(playback) )
			return m_username;

		return this->m_RecCfg.getrec_username();
	}
	const std::string& ConnUse_password( bool playback=false ) const
	{
		if( isGetSelf(playback) )
			return m_password;

		return this->m_RecCfg.getrec_password();
	}
	uint32_t ConnUse_channel( bool playback=false ) const
	{
		if( isGetSelf(playback) )
			return m_channel;

		return this->m_RecCfg.getrec_channel();
	}

	bool GetFrameState();
	void SetFrameState( bool state );

	void setUrl( const std::string& url )
	{
		this->stream->GetRTMPSendObj()->setUrl( url );
	}

	const std::string getUrl()
	{
		return this->stream->GetRTMPSendObj()->getUrl();
	}

	// �����״̬
	defUseable set_NetUseable( defUseable NetUseable, bool *isChanged=NULL );
	defUseable get_NetUseable();
	virtual void check_NetUseable( bool *isChanged=NULL ){};
	bool InvalidVideoBitrate() const;

	// �ۺϷ�������״̬
	defUseable get_all_useable_state();

	bool IsPublish();
	defGSReturn SendToRTMPServer( const JID& jid, const std::string& url, const std::vector<std::string> &url_backup );
	void StopRTMPSend(const JID& jid);
	void StopRTMPSendAll();
	bool CheckSession( const uint32_t enable, const bool isPlayBack, const bool CheckNow );
	uint32_t UpdateSession( const JID& jid, bool *isAdded=NULL );
	uint32_t GetSessionStartTime( const std::string& strjid );
	uint32_t GetSessionLastUpdateTime( const std::string& strjid );
	uint32_t GetSessionCount();
	uint32_t GetSessionInfoList( deflstRTMPSessionInfo &outSessionInfoList );
	void PushVideo( const bool keyframe, const uint32_t rate, char *data, int size, uint32_t timestamp, const bool use_input_ts=true, const bool isAudio=false );

	bool isAutoConnect() const;

	/*�鷽��������̳�*/
	virtual defGSReturn GetCamCompressCfg( std::string *cfginfo, struCamParamBuf *getCamParamBuf=NULL ){ return defGSReturn_UnSupport; };
	virtual defGSReturn SetCamCompressCfg( const defCamCompress CamCompress, const struCamParamBuf *setuseCamParamBuf=NULL ){ return defGSReturn_UnSupport; };
	virtual defGSReturn GetCamShowInfo( int &showtime, int &showname, std::string &name ){ return defGSReturn_UnSupport; };
	virtual defGSReturn SetCamShowInfo( const int showtime, const int showname, const char *pname ){ return defGSReturn_UnSupport; };
	virtual void PreinitBufCheck(){};
	virtual bool TestConnect( std::string &strErr ) = 0;
	virtual bool IsConnect() = 0;
	virtual defGSReturn SetCamTime( const struGSTime &newtime ){ return defGSReturn_UnSupport; };
	void ConnectBefore();
	void ConnectAfter();
	virtual defGSReturn Connect( bool reconnectAudio, void* RealPlayWnd=NULL, bool playback=false, const struGSTime *pdtBegin=NULL, const struGSTime *pdtEnd=NULL ) = 0;
	virtual int PlayBackControl( GSPlayBackCode_ ControlCode, void *pInBuffer = NULL, uint32_t InLen = 0, void *pOutBuffer = NULL, uint32_t *pOutLen = NULL ) = 0;
	virtual bool SendPTZ( GSPTZ_CtrlCmd command, int param1=0, int param2=0, int speed=0, const char *callinfo=NULL ) = 0;
	virtual int GetPTZState(GSPTZ_CtrlCmd command, bool refresh_device) = 0;
	virtual void OnDisconnct(){ this->SetFrameState(false); };
	virtual IPCameraType GetCameraType() const = 0;
	virtual void MakeKeyFrame( bool forcenow=true ) = 0;
	bool isTalkUseCam() const;
	virtual bool isTalkSupport() const = 0;
	virtual bool OnTalkSend( defTalkSendCmd_ TalkSendCmd, unsigned char *pdata, uint32_t data_size ) = 0;
	virtual defGSReturn QuickSearchPlayback( const struGSTime *pdtBegin=NULL, const struGSTime *pdtEnd=NULL ){ return defGSReturn_Success; };

	virtual bool SetAGRunState( bool AGRunState, bool onlyvalue=false ) // �����������ø澯����״̬
	{
		if( onlyvalue )
		{
			m_AGRunState = AGRunState;	// �޸Ĳ�����־
			return true;
		}

		// ���������ñ�Ϊ����ʱ���澯����
		if( m_AGRunState != AGRunState && !AGRunState  )
		{
			this->SetCurAlarmState( defAlarmState_UnInit );
		}

		m_AGRunState = AGRunState;
		return true;
	}
	virtual defAGRunStateFinalST GetAGRunStateFinalST() const
	{
		if( !IsSupportAGRunState() )
		{
			return defAGRunStateFinalST_UnAg;
		}

		if( GetAGRunState() )
		{
			if( IsAGRunStateFinished() )
				return defAGRunStateFinalST_AgFinish;
			else
				return defAGRunStateFinalST_AgFailed;
		}

		return defAGRunStateFinalST_UnAg;
	}
	virtual bool IsSupportAGRunState() const
	{
		return this->GetAdvAttrSafe().get_AdvAttr(defCamAdvAttr_SupportAlarm);
	}

	bool SetCurAlarmState( const defAlarmState CurAlarmState );
	bool CheckResumeCurAlarmState( uint32_t enable );
	defAlarmState GetCurAlarmState() const // ��ȡ��ǰ�澯״̬
	{
		return m_CurAlarmState;
	}

	// PrePic
	static bool SavePrePicChangeInfo( const int camid, const std::string &PrePicChangeCode );
	static bool LoadPrePicChangeInfo( const int camid, std::string &PrePicChangeCode );
	void SetPrePicChangeCode( const std::string &PrePicChangeCode );
	std::string GetPrePicChangeCode();

private:
	defAlarmState m_CurAlarmState;	// �����ǰ�澯״̬��0��1�澯
	uint32_t m_lastupdate_CurAlarmState;// �����ǰ�澯״̬������ʱ��
	std::list<RTMPSession *> sessionList;
	gloox::util::Mutex m_mutex_sessionList;
	
protected:
	// prev state
	bool prev_isState_Publish;
	bool prev_isState_Url;
	bool prev_isState_Session;
	bool prev_FrameState;
	bool prev_IsConnect;
	uint32_t prev_checktime;

	uint32_t m_src_first_ts_video;
	uint32_t m_src_first_ts_audio;
	uint32_t m_src_prev_fix;

	uint32_t m_hasFrameTime;
	defUseable m_NetUseable;
	uint32_t  m_curVideoBitrate; // �豸��ǰ��Ƶ����ֵ��ͨ��ӿڻ�ȡ��
};

#endif

