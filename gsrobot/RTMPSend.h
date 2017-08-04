#ifndef RTMPSEND_H
#define RTMPSEND_H

//#define NO_CRYPTO 1
//#include "rtmp.h"
#include <queue>
//#include <process.h>
//#include <pthread.h>
#include "IPublishHandler.h"
#include "gloox/mutexguard.h"
#include "common.h"
#include "audio/AudioCfg.h"
#include "audio/AudioCap_File.h"

typedef void *defRTMPConnectHandle;

#define STR2AVAL(av,str)av.av_val = str; av.av_len = strlen(av.av_val)


struct H264VideoPackge
{
	uint32_t index; // �����кţ�ѭ������
	int bufLimit;	// ���������ÿռ��С
    char* buf;
	bool keyframe;
    uint32_t timeStamp;
    int size;		// ��ǰ����������ݴ�С
	bool isAudio;
};

class IPlayBackControl
{
public:
	virtual int PlayBackControl( GSPlayBackCode_ ControlCode, void *pInBuffer = NULL, uint32_t InLen = 0, void *pOutBuffer = NULL, uint32_t *pOutLen = NULL )  { return 0; };
};


class RTMPSend
{
private:	
	static std::string s_RTMPSendglobalID;

	uint32_t m_packet_index;
	CAudioCfg m_AudioCfg;
	IPlayBackControl *m_playback;
	int m_playback_sound;
	IPublishHandler *m_handler;
	bool isRunning;
	uint32_t m_runningkey;
	uint32_t m_lastGetVideoPacketTime;
	bool m_isThreadExit;
	x264_nal_s *videoNal;
	int nalSize;
	int videoWidth;
	int videoHeight;
	int videoFps;
	uint32_t startTime;
	uint32_t m_startTime_tick;
	uint32_t m_lasttsVideoPackageList;
	int reConnectCount;
	std::deque<H264VideoPackge*> videoPackage;
	std::deque<H264VideoPackge*> videoPackreuse;
	std::string rtmpUrl;
	std::string m_StreamID;
	std::string m_PeerID;
	std::string m_P2PUrl;
	uint32_t m_RTMFPSessionCount;
	gloox::util::Mutex m_queue_mutex;
	std::string m_name;
	bool wait_frist_key_frame;
	bool rtmp_run_flag;

	uint32_t m_prev_tick_PrintFrame;

	uint32_t m_pushindex;

	char *m_fullbuf_video_cache_buffer;

	AudioCap_File m_AFS;

	GSPlayBackCode_ m_PlayBackCtrl_Code;
	uint32_t m_PlayBackCtrl_ts;
	uint32_t m_doSkipTime_timems; // GSPlayBackCode_SkipTimeʱ��Ч
	int m_PlayBackCtrl_speedlevel; // GSPlayBackCode_PLAYFAST/GSPlayBackCode_PLAYSLOWʱ��Ч
	uint32_t m_PlayBackCtrl_ThrowFrame; // GSPlayBackCode_PLAYFASTʱ��Ч
	GSPlayBackCode_ m_PlayBackCtrl_pause_oldcode; // GSPlayBackCode_PLAYPAUSEʱ��Ч
	uint32_t m_lastThrowIndex;
	
	defRTMPConnectHandle m_RTMPHandle;
	std::string m_RTMPHandle_useUrl;
	

public:
	RTMPSend(IPublishHandler *handler, const std::string& name);
	~RTMPSend(void);
	
	static void Init( const std::string &globalID );
	static defRTMPConnectHandle CreateRTMPInstance( const std::vector<std::string> &vecurl, std::string &useUrl, const char *pname=NULL );
	static void deleteRTMPHandle( defRTMPConnectHandle handle );
	
	CAudioCfg& GetAudioCfg()
	{
		return m_AudioCfg;
	};

	void UpdateAudioCfg( const CAudioCfg& AudioCfg )
	{
		m_AudioCfg = AudioCfg;
	};
	void free_fullbuf_video_cache_buffer();
	char* get_fullbuf_video_cache_buffer();

	uint32_t GetStartTime() const
	{
		return startTime;
	}

	AudioCap_File& GetAFS()
	{
		return m_AFS;
	}

	uint32_t GetQueMaxSize() const;
	uint32_t GetReuseQueMaxSize() const;
	IPlayBackControl* GetIPlayBack() const
	{
		return m_playback;
	}
	void SetIPlayBack( IPlayBackControl *playback )
	{
		m_playback = playback;
	}

	int get_playback_sound() const
	{
		return m_playback_sound;
	}
	void set_playback_sound( int playback_sound )
	{
		m_playback_sound = playback_sound;
	}

	uint32_t get_startTime_tick() const
	{
		return m_startTime_tick;
	}

	bool IsReady();

	bool IsRtmpLive();

	bool IsRunning()
	{
		return this->isRunning;
	}

	uint32_t get_runningkey() const
	{
		return this->m_runningkey;
	}

	std::string getUrl();
	void setUrl( const std::string& url );

	std::string getStreamID();
	void setStreamID( const std::string& StreamID );
	
	std::string getP2PUrl();
	std::string getPeerID();
	void setPeerID( const std::string& PeerID );

	uint32_t getRTMFPSessionCount();
	void setRTMFPSessionCount( const uint32_t RTMFPSessionCount );

	int getVideoWidth()
	{
		return this->videoWidth;
	}
	int getVideoHeight()
	{
		return this->videoHeight;
	}
	int getVideoFps()
	{
		return this->videoFps;
	}

	x264_nal_s* getNal();

	int getNalSize();
	void setNalSize(int size); //jyc20170518 add
	void delNal();

	std::string getName()
	{
		return this->m_name;
	}
	int HasVideoPacket();
	H264VideoPackge* getVideoPacket();

	void set_wait_frist_key_frame()
	{
		wait_frist_key_frame = true;
	}

	bool isRtmprun()
	{
		return this->rtmp_run_flag;
	}
	void clearRtmpflag()
	{
		rtmp_run_flag = false;
	}
	void setRtmpflag()
	{
		rtmp_run_flag = true;
	}

	void pushRTMPHandle( defRTMPConnectHandle handle, const std::string &useUrl );
	defRTMPConnectHandle popRTMPHandle( std::string *useUrl=NULL, const bool dopop=true );

	void SetVideoNal(x264_nal_s *nal,int i_nal);
	int Connect(const std::string& url);
	int SetVideoMetaData(int width,int height,int fps);
	void PushVideo( const bool keyframe, char *buffer, int size, const bool isAudio=false );
	void PushVideo1( const bool keyframe, char *buffer, int size, uint32_t timestamp, const bool isAudio=false );
	int Run();
	void Close();
	void OnSendThreadExit(defGSReturn code);
	void CapThreadExit(); //jyc20170517 add

	H264VideoPackge* newVideoPacket( int needsize, bool keyframe );
	void ReleaseVideoPacket( H264VideoPackge *p );
	void ClearListVideoPacket();
	void RePreFirstKeyListVideoPacket();
	void ReKeyListVideoPacket();
	uint32_t ReListOfOneKey();
	int PlayBackControl( GSPlayBackCode_ ControlCode, uint32_t InValue=0 );

	GSPlayBackCode_ GetPlayBackCtrl_Code() const
	{
		return m_PlayBackCtrl_Code;
	}

	int GetPlayBackCtrl_speedlevel() const
	{
		return m_PlayBackCtrl_speedlevel;
	}

	int GetPlayBackCtrl_ThrowFrame() const
	{
		return m_PlayBackCtrl_ThrowFrame;
	}

	void OnPublishStart()
	{
		m_handler->OnPublishStart();
	}

	uint32_t OnPublishUpdateSession( const std::string &strjid, bool *isAdded=NULL )
	{
		return m_handler->OnPublishUpdateSession( strjid, isAdded );
	}

private:
	void RePreFirstKeyListVideoPacket_nolock();
	void ReKeyListVideoPacket_nolock();
	void ResetPlayBackCtrlFlag( bool resetall=true );
	void ReleaseVideoPacket_nolock( H264VideoPackge *p );
	void FinalDeleteVideoPacket( H264VideoPackge *p );
};

#endif
