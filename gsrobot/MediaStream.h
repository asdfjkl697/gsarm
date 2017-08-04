#ifndef MEDIASTREAM_H_
#define MEDIASTREAM_H_

#include "typedef.h"
#include "H264MediaFile.h"
#include "RTMPSend.h"
#include "IPublishHandler.h"

class MediaStream:public IPublishHandler
{
private:
	int m_width,m_height,m_frameRate;
	bool m_publish,m_savefile;
	
	std::string m_name;

	x264_nal_s *videoNal;
	int nalSize;

	H264MediaFile *m_h264file;
	RTMPSend *m_rtmp;
	IPublishHandler *m_handler;

public:
	MediaStream(IPublishHandler *handler, const std::string& name);
	~MediaStream(void);

	void SetVideoMetaData(int width,int height,int frameRate)
	{
		this->m_width = width;
		this->m_height= height;
		this->m_frameRate = frameRate;

		if( m_rtmp )
		{
			m_rtmp->SetVideoMetaData( this->m_width, this->m_height, this->m_frameRate );
		}

		LOGMSG( "SetVideoMetaData  width=%d, height=%d, frameRate=%d\r\n", this->m_width, this->m_height, this->m_frameRate );
	};
	void SetVideoNal(x264_nal_s *nal,int i_nal);
	void PushVideo( const bool keyframe, char *data, int size, const bool isAudio=false );
	void PushVideo1( const bool keyframe, char *data, int size, uint32_t timestamp, const bool use_input_ts=true, const bool isAudio=false );
	bool SendToRTMPServer( const std::string& url, const bool forcenew=false );
	void StopRTMPSend();
	void SaveToFile(const char* filename);

	GSPlayBackCode_ CheckPlayBackDoState() const;

	RTMPSend* GetRTMPSendObj()
	{
		return this->m_rtmp;
	}

    void OnPublishStart();
	void OnPublishStop(defGSReturn code);
	uint32_t OnPublishUpdateSession( const std::string &strjid, bool *isAdded=NULL )
	{
		return m_handler->OnPublishUpdateSession( strjid, isAdded );
	}

	bool HasNAL() const
	{
		return this->nalSize >1;
	}

	bool IsPublish(){
		return this->m_publish;
	}
};

#endif
