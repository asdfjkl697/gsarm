#include "MediaStream.h"

MediaStream::MediaStream(IPublishHandler *handler, const std::string& name)
	:m_width(0), m_height(0), m_frameRate(25), m_publish(false)
	,m_savefile(false),m_h264file(NULL),m_rtmp(NULL)
	,videoNal(NULL),nalSize(0)
	,m_handler(handler), m_name(name)
{
	videoNal = new x264_nal_s[2];
	m_rtmp = new RTMPSend(this, m_name);
}

MediaStream::~MediaStream(void)
{
	this->StopRTMPSend();

	if(m_h264file){
	    delete(m_h264file);
	}
	if(m_rtmp){
	    delete(m_rtmp);
	}

	nalSize = 2;
	Delete_x264_nal_t( &videoNal, nalSize );
	nalSize = 0;
}

void MediaStream::SetVideoNal(x264_nal_s *nal,int i_nal)
{
	char chbuf[256] = {0};

	for( int i=0; i<i_nal; ++i )
	{
		snprintf( chbuf, sizeof(chbuf), "MediaStream(%s)::SetVideoNal(%d/%d)", this->m_name.c_str(), i+1, i_nal );

		g_PrintfByte( nal[i].p_payload, nal[i].i_payload>64?64:nal[i].i_payload, chbuf );
	}

	if( this->nalSize<=0 )
	{
		for( int i=0; i<i_nal; ++i )
		{
			Copy_x264_nal_t( videoNal[i], nal[i] );
		}
		this->nalSize = i_nal;
	}
	else if( this->nalSize<=1 )
	{
		for( int i=0; i<i_nal; ++i )
		{
			if( videoNal[0].p_payload[0] == nal[i].p_payload[0] )
				continue;

			Copy_x264_nal_t( videoNal[1], nal[i] );
			this->nalSize = 2;
			break;
		}
	}

	if( this->HasNAL() && m_rtmp )
	{
		m_rtmp->SetVideoNal( this->videoNal, this->nalSize );
	}
}

void MediaStream::PushVideo( const bool keyframe, char *data, int size, const bool isAudio )
{
	PushVideo1(keyframe, data,size, 0, false, isAudio);
}

void MediaStream::PushVideo1( const bool keyframe, char *data, int size, uint32_t timestamp, const bool use_input_ts, const bool isAudio )
{
	if(m_publish || this->GetRTMPSendObj()->GetIPlayBack()){
		if( use_input_ts )
			m_rtmp->PushVideo1(keyframe, data,size, timestamp, isAudio);
		else
			m_rtmp->PushVideo(keyframe, data,size, isAudio);
	}

	if( !isAudio )
	{
		if(m_savefile){
			m_h264file->WriteData(data,size, keyframe?defH264MediaFile_Data_Key:defH264MediaFile_Data_Normal);
		}
	}
}

bool MediaStream::SendToRTMPServer( const std::string& url, const bool forcenew )
{
	if(!m_publish || forcenew ){

		//if( m_width<=0 || m_height<=0 || !HasNAL() )
		//{
		//	LOGMSGEX( defLOGNAME, defLOG_ERROR, "MediaStream::SendToRTMPServer Failed! m_width=%d, m_width=%d, nalSize=%d", m_width, m_height, nalSize );
		//	return false;
		//}

		if(!m_rtmp)
		    m_rtmp = new RTMPSend(this, m_name);

		//m_rtmp->Connect(url);
		//m_rtmp->SetVideoMetaData(this->m_width,this->m_height,this->m_frameRate);
		//m_rtmp->SetVideoNal(this->videoNal,this->nalSize);
		m_rtmp->Run();
	}

	return true;
}

void MediaStream::StopRTMPSend()
{
	m_publish = false;
	if(m_rtmp)
	   m_rtmp->Close();
}

void MediaStream::SaveToFile(const char* filename)
{
	m_h264file = new H264MediaFile(filename);
	/*for(int i=0;i<this->nalSize;i++){
		m_h264file->WriteData((char *)this->videoNal[i].p_payload,this->videoNal[i].i_payload,defH264MediaFile_Data_Nal);
	}*/
	m_savefile = true;
}

void MediaStream::OnPublishStart(){
	this->m_publish = true;
	if(m_handler){
		m_handler->OnPublishStart();
	}
}

void MediaStream::OnPublishStop(defGSReturn code){
	this->m_publish = false;
	if(m_handler){
		m_handler->OnPublishStop(code);
	}
}

GSPlayBackCode_ MediaStream::CheckPlayBackDoState() const
{
	if( m_rtmp->GetIPlayBack() )
	{
		if( m_rtmp->IsRunning() )
		{
			if( m_rtmp->HasVideoPacket()>270 )
			{
				return GSPlayBackCode_PLAYPAUSE;
			}
		}
		else
		{
			if( this->HasNAL() && m_rtmp->IsReady() && m_rtmp->HasVideoPacket()>100 )
			{
				return GSPlayBackCode_PLAYPAUSE;
			}
		}
	}

	return GSPlayBackCode_NULL;
}
