#include "RTMPSend.h"
#include <stdio.h>
#include <time.h>
#include "librtmp/rtmp_sys.h" //jyc20170511 modify
#include "common.h"
#include "RunCode.h"
#include "faac.h" //jyc20170511 modify
#include "faaccfg.h"
#include "include/flv.h" //jyc20170511 resume add form libavformat
//#include "MemoryContainer.h"
//#include <objbase.h> //win
#include <uuid/uuid.h> //linux
#include <pthread.h>

//#define VIDEOH264_FLAG 1

#ifdef VIDEOH264_FLAG
#include "h264/videoh264.h" //jyc20170726
#else
#include "Video.h" //jyc20170519 add
#endif

#include "recorder.h"  //jyc20170525 add
#include "player.h"
//#include <faac.h>

typedef struct _GUID
{
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
} GUID, UUID;

//#define defUseRTMFP  //jyc20170511 remove

#define defUse_TSStartTime // ʱ�����ȡ���ڿ�ʼʱ��

#define MAX_VIDEO_PACKET_SIZE (2*1024*1024)//(512*1024)
#define MINNEW_VIDEO_PACKET_SIZE (32*1024)
#define KEYNEWOVER_VIDEO_PACKET_SIZE (32*1024)


// ���ж����ʱ
char *g_flv_code_morepak( char *enc, char *pend, const char *pBuf_src, const uint32_t bufLen_src )
{
	const char *pBuf = pBuf_src;
	uint32_t bufLen = bufLen_src;
	int prefix = 0;
	uint32_t offset = 0;
	while( bufLen>0 
		&& pBuf + 3 < pBuf_src+bufLen_src )
	{
		uint32_t cplen = bufLen;
		offset = g_GetH264PreFix( (unsigned char*)pBuf, bufLen, prefix, 256 );
		if( offset > 0 )
		{
			cplen = offset-prefix;
		}
		else
		{
			cplen = bufLen;
		}

		enc = AMF_EncodeInt32( enc, pend, cplen );
		memcpy( enc, pBuf, cplen );
		enc += cplen;

		pBuf += (cplen + prefix);
		bufLen -= (cplen + prefix);
	}

	return enc; // return new size
}

bool g_rtmp_sendnal( RTMPSend *rtmp_send, RTMPPacket &rtmppkt, RTMPPacket &rtmppkt_sendnal, char *szBodyBuffer, char *enc, char *pend, uint32_t nTimeStamp )
{
	int nal_size = rtmp_send->getNalSize();
	if(nal_size>1){	
		enc = szBodyBuffer;	
		rtmppkt.m_headerType = RTMP_PACKET_SIZE_LARGE;
		rtmppkt.m_nTimeStamp = nTimeStamp;//0;//(uint32_t)packet->timeStamp;
		rtmppkt.m_packetType = RTMP_PACKET_TYPE_VIDEO;

		x264_nal_s *nal_ptr = rtmp_send->getNal();

		int sps_size = nal_ptr[0].i_payload;
		uint8_t *sps = nal_ptr[0].p_payload;		
		int pps_size = nal_ptr[1].i_payload;
		uint8_t *pps = nal_ptr[1].p_payload;

		*enc++= 7 | FLV_FRAME_KEY;
		*enc++= 0; // AVC sequence header
		enc = AMF_EncodeInt24(enc,pend,0); // composition time

		*enc++= 0x01;  // version 
		*enc++= sps[1]; // profile 
		*enc++= sps[2]; // profile compat
		*enc++= sps[3];
		*enc++= (uint8_t)0xFF;
		*enc++= (uint8_t)0xE1;

		enc = AMF_EncodeInt16(enc,pend, sps_size);
		memcpy(enc, sps, sps_size);
		enc+= sps_size;
		*enc++= 1;  //pps number
		enc = AMF_EncodeInt16(enc,pend, pps_size);
		memcpy(enc, pps, pps_size);
		enc+= pps_size;
		rtmppkt.m_nBodySize=enc - szBodyBuffer;

		char buf[256] = {0};
		snprintf( buf, sizeof(buf), "name=%s, RTMP_SendPacket NAL size=%d", rtmp_send->getName().c_str(), rtmppkt.m_nBodySize );
		g_PrintfByte( (unsigned char*)rtmppkt.m_body, rtmppkt.m_nBodySize>64?64:rtmppkt.m_nBodySize, buf );

		RTMPPacket_Copy(&rtmppkt_sendnal,&rtmppkt ); //jyc20170517 resume
	}
	else
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "name=%s, getNalSize failed Nal!\r\n", rtmp_send->getName().c_str() );
		return false;
	}

	return true;
}

// audio sequence header������д���Ϣʱ����
bool g_rtmp_sendAudioInfo( RTMPSend *rtmp_send, RTMPPacket &rtmppkt, RTMPPacket &rtmppkt_sendAudioInfo, char *szBodyBuffer, char *enc, char *pend, uint32_t nTimeStamp, uint8_t &AudioHead )
{
	if( defAudioSource_Null == rtmp_send->GetAudioCfg().get_Audio_Source() )
	{
		return false;
	}

	AudioHead = rtmp_send->GetAudioCfg().get_FlvAudioHead();

	if( defAudioFmtType_AAC != rtmp_send->GetAudioCfg().get_Audio_FmtType() )
	{
		return false;
	}

	if( defAudioSource_File == rtmp_send->GetAudioCfg().get_Audio_Source()
		&& rtmp_send->GetIPlayBack() )
	{
		return false;
	}

	const unsigned int aacObjectType = defAACObjectType_Default;

	rtmppkt.m_packetType = RTMP_PACKET_TYPE_AUDIO;
	szBodyBuffer[0] = AudioHead;
	szBodyBuffer[1] = 0x00; // 0: AAC sequence header

	unsigned char aac_cfg1 = 0;
	unsigned char aac_cfg2 = 0;
	rtmp_send->GetAudioCfg().get_AAC_AudioSpecificConfig( aac_cfg1, aac_cfg2, aacObjectType );
	szBodyBuffer[2] = aac_cfg1;
	szBodyBuffer[3] = aac_cfg2;

	rtmppkt.m_nBodySize = 4;

	//LOGMSGEX( defLOGNAME, defLOG_INFO, "name=%s, g_rtmp_sendAudioInfo AAC cfg %02X %02X. BodySize=%d\r\n", rtmp_send->getName().c_str(), aac_cfg1, aac_cfg2, rtmppkt.m_nBodySize );

	RTMPPacket_Copy( &rtmppkt_sendAudioInfo, &rtmppkt ); //jyc20170517 resume

	return true;
}

unsigned __stdcall RTMPDataExService(LPVOID lpPara)
{
	RTMPSend *rtmp_send = (RTMPSend *)lpPara;
	const uint32_t runningkey = rtmp_send->get_runningkey();

	while( rtmp_send->IsRunning() && runningkey == rtmp_send->get_runningkey() )
	{
		if( defAudioSource_File == rtmp_send->GetAudioCfg().get_Audio_Source()
			&& !rtmp_send->GetIPlayBack()
			)
		{
			AudioCap_File &AFS = rtmp_send->GetAFS();
			int audio_size = 0;
			unsigned char *paudio_buf = AFS.GetFrame(audio_size);
			if( paudio_buf && audio_size>0 )
			{
				//LOGMSG( "aacfile ts=%d, ensize=%d", timeGetTime(), audio_size );
				rtmp_send->PushVideo1( false, (char*)paudio_buf, audio_size, timeGetTime(), true );
			}
			else
			{
//				timeBeginPeriod(1); //jyc20170511 remove
//				DWORD start = timeGetTime();
//				usleep(1000);
//				DWORD end = timeGetTime();
//				timeEndPeriod(1);
				usleep(1000);//jyc20170511 add
			}
		}
		else
		{
			break;
		}
	}

	return 0;
}

//unsigned __stdcall RTMPPushThread(LPVOID lpPara)
void *RTMPPushThread(LPVOID lpPara)
{
	RTMPSend *rtmp_send = (RTMPSend *)lpPara;
	const uint32_t runningkey = rtmp_send->get_runningkey();

	uint32_t RTMFPSession_ts = timeGetTime()-60*1000;
	const std::string RTMFPSession_strjid = defRTMFPSession_strjid;

	std::string useUrl;

	defGSReturn threadret = defGSReturn_Err;
	RTMP *r = NULL;
	RTMPPacket rtmppkt = {0};

	RTMPPacket rtmppkt_metahead = {0};
	RTMPPacket_Reset( &rtmppkt_metahead );

	RTMPPacket rtmppkt_sendnal = {0};
	RTMPPacket_Reset( &rtmppkt_sendnal );

	RTMPPacket rtmppkt_sendAudioInfo = {0};
	RTMPPacket_Reset( &rtmppkt_sendAudioInfo );

	rtmp_send->popRTMPHandle( &useUrl, false );
	const bool New_isRTMFP = g_IsRTMFP_url( useUrl );

	//jyc20170525 add for audio
	rtmp_send->GetAudioCfg().set_Audio_Source(defAudioSource_LocalCap);
	rtmp_send->GetAudioCfg().set_Audio_FmtType(defAudioFmtType_AAC);

//音频源-->文件
//	if( defAudioSource_File == rtmp_send->GetAudioCfg().get_Audio_Source()
//		&& !rtmp_send->GetIPlayBack()
//		)
//	{
//		AudioCap_File &AFS = rtmp_send->GetAFS();
//		if( AFS.LoadFiles( rtmp_send->GetAudioCfg() ) )
//		{
////			if( AFS.GetPCM() ) //jyc20170511 remove
////			{
////				if( defAudioParamDef_Analyse == rtmp_send->GetAudioCfg().get_Audio_ParamDef() )
////				{
////					CAudioCfg::struAudioParam ap;
////					ap.Audio_FmtType = defAudioFmtType_AAC;
////					ap.Audio_Channels = AFS.GetPCM()->channels;
////					ap.Audio_bitSize = AFS.GetPCM()->samplebytes*8;
////					ap.Audio_SampleRate = AFS.GetPCM()->samplerate;
////					ap.Audio_ByteRate = 0;
////					rtmp_send->GetAudioCfg().set_AudioParam_Analyse( ap );
////				}
////			}
//			//LOGMSGEX( defLOGNAME, defLOG_ERROR, "name=%s, AudioCap_File Start success.\r\n", rtmp_send->getName().c_str() );
//		}
//	}

	const int c_PlayBack_spanfix = RUNCODE_Get(defCodeIndex_SYS_PlayBack_spanfix);
	if( rtmp_send->GetIPlayBack() )
	{
		if( IsRUNCODEEnable(defCodeIndex_SYS_PlayBackSetSpeed) )
		{
			int netspeed = 8192; // �ٶȵ�λ��kbps // 8192,16384
			const int ret = rtmp_send->GetIPlayBack()->PlayBackControl( GSPlayBackCode_SETSPEED, &netspeed, sizeof(int) );
			LOGMSG( "name=%s, set playback netspeed=%d kbps, ret=%d\r\n", rtmp_send->getName().c_str(), netspeed, ret );
		}

		if( rtmp_send->HasVideoPacket() < 70 || !rtmp_send->IsReady() )
		{
			rtmp_send->GetIPlayBack()->PlayBackControl( GSPlayBackCode_PLAYRESTART );
		}

		LOGMSG( "RTMPPushThread PlayBack_spanfix=%d", c_PlayBack_spanfix );
	}

	//jyc20170512 move here
	char *fullbuf_video_cache_buffer = rtmp_send->get_fullbuf_video_cache_buffer();//char fullbuf_video_cache_buffer[MAX_VIDEO_PACKET_SIZE+RTMP_MAX_HEADER_SIZE];
	char *video_cache_buffer = fullbuf_video_cache_buffer+RTMP_MAX_HEADER_SIZE; // ��ݻ���ǰ��Ԥ����RTMPͷ����
	char *pend = video_cache_buffer+MAX_VIDEO_PACKET_SIZE;
	char *szBodyBuffer = video_cache_buffer;
	char *enc = szBodyBuffer;
	uint8_t AudioHead = 0;
	bool sendnal = false;
	DWORD first_tick = 0;
	DWORD prev_tick = 0;
	DWORD curtick = ::timeGetTime();
	DWORD basetick = ::timeGetTime();
	uint32_t prev_TimeStamp = 0;
	const DWORD this_start_send_tick = ::timeGetTime(); // jyc20170512 move because 		goto
	DWORD dwStartWait = ::timeGetTime();

	while( !rtmp_send->IsReady() || !(rtmp_send->getNalSize()>0) ) //jyc20170516 debug
	{
		//jyc20170518 debug
//		bool ready_f=rtmp_send->IsReady();
//		int nalsize_f=rtmp_send->getNalSize();
//		bool run_f=rtmp_send->IsRunning();
//		uint32_t runkey_f=rtmp_send->get_runningkey();
//		printf("pushthread %d %d %d %d %d",ready_f,nalsize_f,run_f,runkey_f,runningkey);
//		printf("......RTMPPushThread......\n");

		if( !rtmp_send->IsRunning() || runningkey != rtmp_send->get_runningkey() )
		{
			goto label_RTMPPushThread_End; //jyc20170517 remove
		}

		if( ::timeGetTime()-dwStartWait>30000 )
		{
			threadret = defGSReturn_NoData;
			LOGMSG( "name=%s, wait ready and nal timeout %dms!!!\r\n", rtmp_send->getName().c_str(), ::timeGetTime()-dwStartWait );
			goto label_RTMPPushThread_End; //jyc20170511 remove
		}

		//usleep(1000); //jyc20170511 modify
		usleep(10000); //jyc20170519 modify ok
	}

	RTMPPacket_Reset( &rtmppkt );
	//jyc20170512 move up
	rtmppkt.m_nChannel = 0x15;
	rtmppkt.m_headerType = RTMP_PACKET_SIZE_LARGE;//RTMP_PACKET_SIZE_MEDIUM;//RTMP_PACKET_SIZE_LARGE;
	rtmppkt.m_nTimeStamp = 0;
	rtmppkt.m_nInfoField2 = -1; // r->m_stream_id;
	if( r ) rtmppkt.m_nInfoField2 = r->m_stream_id;
	rtmppkt.m_hasAbsTimestamp = 0;
    rtmppkt.m_packetType = RTMP_PACKET_TYPE_INFO;
	rtmppkt.m_body = szBodyBuffer;

	AVal av;
	STR2AVAL(av, "@setDataFrame");
	enc = AMF_EncodeString(enc, pend, &av);
	STR2AVAL(av, "onMetaData");
	enc = AMF_EncodeString(enc, pend, &av);
	*enc++ = AMF_OBJECT;
	STR2AVAL(av, "hasMetadata");
	enc = AMF_EncodeNamedBoolean(enc, pend, &av, 1);
	STR2AVAL(av, "hasVideo");
	enc = AMF_EncodeNamedBoolean(enc, pend, &av, 1);
	STR2AVAL(av, "hasKeyframes");
	enc = AMF_EncodeNamedBoolean(enc, pend, &av, 1);
	STR2AVAL(av, "width");
	enc = AMF_EncodeNamedNumber(enc, pend, &av, rtmp_send->getVideoWidth());
	STR2AVAL(av, "height");
	enc = AMF_EncodeNamedNumber(enc, pend, &av, rtmp_send->getVideoHeight());
	STR2AVAL(av, "videocodecid");
	enc = AMF_EncodeNamedNumber(enc, pend, &av, 7);

	if( defAudioSource_Null != rtmp_send->GetAudioCfg().get_Audio_Source()
		&& !rtmp_send->GetIPlayBack()
		)
	{
		int Audio_Channels = 0;
		int Audio_bitSize = 0;
		int Audio_SampleRate = 0;
		int Audio_ByteRate = 0;
		rtmp_send->GetAudioCfg().get_param_use( Audio_Channels, Audio_bitSize, Audio_SampleRate, Audio_ByteRate );

		STR2AVAL(av, "hasAudio");
		enc = AMF_EncodeNamedBoolean( enc, pend, &av, 1);

		STR2AVAL(av, "audiocodecid");
		enc=AMF_EncodeNamedNumber( enc, pend, &av, rtmp_send->GetAudioCfg().get_Audio_FmtType() );

		STR2AVAL(av, "audiosamplerate");
		enc=AMF_EncodeNamedNumber( enc, pend, &av, Audio_SampleRate );

		STR2AVAL(av, "audiochannels");
		enc=AMF_EncodeNamedNumber( enc, pend, &av, Audio_Channels );
	}

    enc = AMF_EncodeInt16(enc, pend, 0); //enc= AMF_EncodeString(enc, pend,&av);
    *enc++ = AMF_OBJECT_END;

    rtmppkt.m_nBodySize = enc - szBodyBuffer;

	RTMPPacket_Copy( &rtmppkt_metahead, &rtmppkt ); //jyc20170516 resume

	if( !g_rtmp_sendnal( rtmp_send, rtmppkt, rtmppkt_sendnal, szBodyBuffer, enc, pend, 0 ) )
	{
		goto label_RTMPPushThread_End; //jyc20170517 resume
	}

	if( g_rtmp_sendAudioInfo( rtmp_send, rtmppkt, rtmppkt_sendAudioInfo, szBodyBuffer, enc, pend, 0, AudioHead ) )
	{

	}

	if( defAudioSource_File == rtmp_send->GetAudioCfg().get_Audio_Source()
		&& !rtmp_send->GetIPlayBack()
		)
	{
		AudioCap_File &AFS = rtmp_send->GetAFS();
		AFS.Start( rtmp_send->GetStartTime(), true );
//		HANDLE   hth1;  //jyc20170511 trouble
//		unsigned  uiThread1ID;
//		hth1 = (HANDLE)_beginthreadex( NULL, 0, RTMPDataExService, rtmp_send, 0, &uiThread1ID );
//		CloseHandle(hth1);
	}

	if( !rtmp_send->GetIPlayBack() )
	{
		rtmp_send->ReKeyListVideoPacket();
	}

	rtmppkt.m_headerType = RTMP_PACKET_SIZE_LARGE;

	while( rtmp_send->IsRunning() && runningkey == rtmp_send->get_runningkey() )
	{
		//curtick = ::timeGetTime();
		//if( (curtick-basetick) > 100 )LOGMSGEX( defLOGNAME, defLOG_WORN, "name=%s, RTMPSend s0 tick=%u\r\n", rtmp_send->getName().c_str(), curtick-basetick );
		basetick = ::timeGetTime();

		if( !r )
		{
			r = (RTMP*)rtmp_send->popRTMPHandle();
			// PreSendRTMP
			if( r )
			{
				rtmppkt.m_nInfoField2 = r->m_stream_id;
				rtmppkt_metahead.m_nInfoField2 = rtmppkt.m_nInfoField2;
				rtmppkt_sendnal.m_nInfoField2 = rtmppkt.m_nInfoField2;
				rtmppkt_sendAudioInfo.m_nInfoField2 = rtmppkt.m_nInfoField2;

				if( !RTMP_SendPacket( r, &rtmppkt_metahead, 0 ) )
				{
					//LOGMSGEX( defLOGNAME, defLOG_ERROR, "name=%s, RTMP_SendPacket failed RTMP_PACKET_TYPE_INFO!\r\n", rtmp_send->getName().c_str() );
					goto label_RTMPPushThread_End;
				}

				int outChunkSize = RUNCODE_Get( defCodeIndex_RTMPSend_SetChunkSize );//32*1024;//4096; //32000
				//LOGMSGEX( defLOGNAME, defLOG_INFO, "name=%s, RTMP_SetChunkSize=%d\r\n", rtmp_send->getName().c_str(), outChunkSize );
				if( !RTMP_SetChunkSize( r, outChunkSize ) ) //jyc20170516 resume
				{
					//LOGMSGEX( defLOGNAME, defLOG_ERROR, "name=%s, RTMP_SetChunkSize=%d error!\r\n", rtmp_send->getName().c_str(), outChunkSize );
					goto label_RTMPPushThread_End;
				}

				if( !RTMP_SendPacket( r, &rtmppkt_sendnal, 0 ) ){
					//LOGMSGEX( defLOGNAME, defLOG_ERROR, "name=%s, RTMP_SendPacket failed Nal!\r\n", rtmp_send->getName().c_str() );
					goto label_RTMPPushThread_End;
				}

				if( rtmppkt_sendAudioInfo.m_body )
				{
					if( !RTMP_SendPacket( r, &rtmppkt_sendAudioInfo, 0 ) ){
						//LOGMSGEX( defLOGNAME, defLOG_ERROR, "name=%s, RTMP_SendPacket failed audio info!\r\n", rtmp_send->getName().c_str() );
					}
				}
			}
		}

		if( rtmp_send->GetIPlayBack() )
		{
			if( rtmp_send->HasVideoPacket() < 70 )
			{
				rtmp_send->GetIPlayBack()->PlayBackControl( GSPlayBackCode_PLAYRESTART );
			}
		}

		H264VideoPackge *packet = rtmp_send->getVideoPacket();
		if( packet )
		{
			//LOGMSG( "name=%s, RTMPSend getVideoPacket ts=%u, prev_ts=%u, audio=%d, packetSize=%u\r\n", rtmp_send->getName().c_str(), packet->timeStamp, prev_tick, packet->isAudio, packet->size );
//			curtick = ::timeGetTime(); //jyc20170512 remove
//			if( (curtick-basetick) > 100 )LOGMSGEX( defLOGNAME, defLOG_WORN, "name=%s, RTMPSend s1 getVideoPacket tick=%u, ts=%u, packetSize=%u\r\n", rtmp_send->getName().c_str(), curtick-basetick, packet->timeStamp, packet->size );
			basetick = ::timeGetTime();
			if( r )
			{
				if(!RTMP_IsConnected(r) || RTMP_IsTimedout(r)){
					//LOGMSGEX( defLOGNAME, defLOG_ERROR,"name=%s, connect error\r\n", rtmp_send->getName().c_str());
					rtmp_send->ReleaseVideoPacket(packet);
					break;
				}
			}

			if(packet->size > MAX_VIDEO_PACKET_SIZE-9){
				//LOGMSGEX( defLOGNAME, defLOG_ERROR, "name=%s, packet too large, packetSize=%u\r\n", rtmp_send->getName().c_str(), packet->size );
				rtmp_send->ReleaseVideoPacket(packet);
				continue;
			}

	        enc = video_cache_buffer;
			if( packet->isAudio )
			{
				rtmppkt.m_packetType = RTMP_PACKET_TYPE_AUDIO;
				*enc++= AudioHead;
				if( defAudioFmtType_AAC == rtmp_send->GetAudioCfg().get_Audio_FmtType() )
				{
					*enc++= 0x01; // 1: AAC raw
				}
				memcpy(enc, packet->buf, packet->size);
				enc+= packet->size;
			}
			else
			{
				rtmppkt.m_packetType = RTMP_PACKET_TYPE_VIDEO;
				*enc++= 7 | (packet->keyframe ?  FLV_FRAME_KEY : FLV_FRAME_INTER);
				*enc++= 1;
				enc = AMF_EncodeInt24(enc,pend,0);
				enc = g_flv_code_morepak( enc, pend, packet->buf, packet->size );
			}
			rtmppkt.m_nTimeStamp = (uint32_t)packet->timeStamp;

//			curtick = ::timeGetTime(); //jyc20170512 remove
//			if( (curtick-basetick) > 10 )
//				LOGMSGEX( defLOGNAME, defLOG_WORN, "name=%s, RTMPSend s2 AMF_Encode tick=%u, ts=%u, packetSize=%u\r\n", rtmp_send->getName().c_str(), curtick-basetick, rtmppkt.m_nTimeStamp, packet->size );

			if( rtmp_send->GetIPlayBack() )
			{
				int hassize = rtmp_send->HasVideoPacket();
				int tick_span = packet->timeStamp-prev_tick;
				if( prev_tick && tick_span>c_PlayBack_spanfix )
				{
					tick_span -= c_PlayBack_spanfix;
					if( tick_span < 1 )
					{
						tick_span = 1;
					}
					else if( tick_span > 120 )
					{
						if( 0!=rtmp_send->GetPlayBackCtrl_speedlevel()
							&& (GSPlayBackCode_PLAYFAST==rtmp_send->GetPlayBackCtrl_Code() || GSPlayBackCode_PLAYSLOW==rtmp_send->GetPlayBackCtrl_Code()) )
						{
							if( tick_span > 3000 )
							{
								tick_span = 3000;
							}
						}
						else
						{
							tick_span = 120;
						}
					}
					usleep(tick_span*1000);
				}
				prev_tick = packet->timeStamp;
				if( !rtmp_send->IsRunning() )
				{
					rtmp_send->ReleaseVideoPacket(packet);
					break;
				}
				if( hassize < 70 )
				{
					rtmp_send->GetIPlayBack()->PlayBackControl( GSPlayBackCode_PLAYRESTART );
				}
			}

			rtmppkt.m_body = video_cache_buffer;
			rtmppkt.m_nBodySize = enc - video_cache_buffer;

			bool doSendPacket = false;
			if( packet->isAudio )
			{
				if( defAudioSource_Null != rtmp_send->GetAudioCfg().get_Audio_Source() )
				{
					if( rtmp_send->GetIPlayBack() )
					{
						if( rtmp_send->get_playback_sound() )
						{
							doSendPacket = true;
						}
					}
					else
					{
						doSendPacket = true;
					}
				}
			}
			else
			{
				doSendPacket = true;
			}

			if( doSendPacket )
			{
				if( rtmppkt.m_nTimeStamp <= prev_TimeStamp )
				{
					rtmppkt.m_nTimeStamp = prev_TimeStamp + 2;
				}
				prev_TimeStamp = rtmppkt.m_nTimeStamp;

				if( r )
				{
					if(RTMP_SendPacket(r,&rtmppkt,0) ==0){
						curtick = ::timeGetTime();
						//LOGMSGEX( defLOGNAME, defLOG_ERROR, "name=%s, RTMP_SendPacket error, tick=%u, ts=%u, packetSize=%u\r\n", rtmp_send->getName().c_str(), curtick-basetick, rtmppkt.m_nTimeStamp, packet->size );
						rtmp_send->ReleaseVideoPacket(packet);
						break;
					}
				}
			}

			curtick = ::timeGetTime();
			if( (curtick-basetick) > (DWORD)RUNCODE_Get(defCodeIndex_RTMPSend_NetSend_WarnTime) )
			{
				if( 0!=rtmp_send->GetPlayBackCtrl_speedlevel()
					&& (GSPlayBackCode_PLAYFAST==rtmp_send->GetPlayBackCtrl_Code() || GSPlayBackCode_PLAYSLOW==rtmp_send->GetPlayBackCtrl_Code()) )
				{
					// playback speed!=normal
				}
				else
				{
					//LOGMSGEX( defLOGNAME, defLOG_WORN, "name=%s, RTMPSend s9 tick=%u, ts=%u, BodySize=%u\r\n", rtmp_send->getName().c_str(), curtick-basetick, rtmppkt.m_nTimeStamp, rtmppkt.m_nBodySize );
				}
			}
			basetick = ::timeGetTime();
			rtmp_send->ReleaseVideoPacket(packet);
			rtmppkt.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
		}else{
//			timeBeginPeriod(1);
//			DWORD start = timeGetTime();
//			Sleep(1);
//			DWORD end = timeGetTime();
//			timeEndPeriod(1);
			usleep(1000);
		}
	}
	threadret = defGSReturn_Success;

label_RTMPPushThread_End:
	rtmp_send->free_fullbuf_video_cache_buffer(); //jyc20170720 add
	rtmp_send->clearRtmpflag(); //jyc20170525 add for rtmpcap thread exit
	RTMPPacket_Free( &rtmppkt_metahead );
	RTMPPacket_Free( &rtmppkt_sendnal );
	RTMPPacket_Free( &rtmppkt_sendAudioInfo );

	if( r )
	{
		RTMP_Close(r);
		RTMP_Free(r);
	}

	rtmp_send->OnSendThreadExit( threadret );
	pthread_detach(pthread_self()); //jyc20170722 add
	//return 0;
}

std::string RTMPSend::s_RTMPSendglobalID = "";

void RTMPSend::Init( const std::string &globalID )
{
	RTMPSend::s_RTMPSendglobalID = globalID;
}

RTMPSend::RTMPSend(IPublishHandler *handler, const std::string& name)
	:m_handler(handler), m_playback(NULL), m_playback_sound(1), m_name(name), m_prev_tick_PrintFrame(0), m_pushindex(0)//,m_queue_lock(true)
{
	m_packet_index = 0;
	m_runningkey = 0;
	m_lastGetVideoPacketTime = timeGetTime();
	startTime = 0;
	m_lasttsVideoPackageList = 0;
	m_startTime_tick = 0;
	isRunning = false;
	m_isThreadExit = true;
	nalSize = 0;
	videoNal = NULL;//new x264_nal_t[2];
	videoWidth = 0;
	videoHeight = 0;
	videoFps = 0;
	reConnectCount = 0;

	unsigned char thisid[32] ={0};
	uint16_t thisid_num = 0;

	GUID guid; //jyc20170511 remove
	uuid_generate(reinterpret_cast<unsigned char *>(&guid));
	thisid_num = sizeof( guid );
	memcpy( &thisid, &guid, thisid_num );

//	if( S_OK == ::CoCreateGuid( &guid ) )
//	{
//		thisid_num = sizeof( guid );
//		memcpy( &thisid, &guid, thisid_num );
//	}
//	else
//	{
//		const uint32_t nt = (uint32_t)this;
//		thisid_num = sizeof( nt );
//		memcpy( thisid, &nt, thisid_num );
//	}

	m_StreamID =
		IsRUNCODEEnable( defCodeIndex_RTMFP_UseSpecStreamID )
		?
		m_StreamID = RUNCODE_GetStr( defCodeIndex_RTMFP_UseSpecStreamID )
		:
		m_StreamID = g_BufferToString( thisid, thisid_num, false, false );

	m_RTMFPSessionCount = 0;
	
	m_fullbuf_video_cache_buffer = NULL;

	ResetPlayBackCtrlFlag();
	m_lastThrowIndex = 0;

	m_RTMPHandle = NULL;
}

RTMPSend::~RTMPSend(void)
{
	this->Close();

	if( m_fullbuf_video_cache_buffer )
	{
#if defuseMemoryContainer
		g_GetMemoryContainer()->ReleaseMemory( m_fullbuf_video_cache_buffer,
				MAX_VIDEO_PACKET_SIZE+RTMP_MAX_HEADER_SIZE );
#else
		delete []m_fullbuf_video_cache_buffer;
#endif
		m_fullbuf_video_cache_buffer = NULL;
	}

	delNal();

	m_queue_mutex.lock();

	if( m_RTMPHandle )
	{
		deleteRTMPHandle( m_RTMPHandle );
	}

	m_RTMPHandle = NULL;
	m_RTMPHandle_useUrl = "";

	m_queue_mutex.unlock();

}

void RTMPSend::free_fullbuf_video_cache_buffer()
{
	if( m_fullbuf_video_cache_buffer )
	{
#if defuseMemoryContainer
		g_GetMemoryContainer()->ReleaseMemory( m_fullbuf_video_cache_buffer,
				MAX_VIDEO_PACKET_SIZE+RTMP_MAX_HEADER_SIZE );
#else
		delete []m_fullbuf_video_cache_buffer;
#endif
		m_fullbuf_video_cache_buffer = NULL;
	}
}

char* RTMPSend::get_fullbuf_video_cache_buffer()
{
	if( !m_fullbuf_video_cache_buffer )
	{	//jyc20170512 modify because momorycomtainer.h error
#if defuseMemoryContainer
		m_fullbuf_video_cache_buffer = g_GetMemoryContainer()->GetMemory( MAX_VIDEO_PACKET_SIZE+RTMP_MAX_HEADER_SIZE );
#else
		m_fullbuf_video_cache_buffer = new char[MAX_VIDEO_PACKET_SIZE+RTMP_MAX_HEADER_SIZE];
#endif
	}
	return m_fullbuf_video_cache_buffer;
}


void RTMPSend::deleteRTMPHandle( defRTMPConnectHandle handle )
{
	if( !handle )
		return;

	RTMP *r = (RTMP*)handle;
	if( r )
	{
		RTMP_Close(r);
		RTMP_Free(r);
	}
}

void string_replace(string &s1,const string &s2,const string &s3)
{
	string::size_type pos=0;
	string::size_type a=s2.size();
	string::size_type b=s3.size();
	while((pos=s1.find(s2,pos))!=string::npos)
	{
		s1.replace(pos,a,s3);
		pos+=b;
	}
}

defRTMPConnectHandle RTMPSend::CreateRTMPInstance( const std::vector<std::string> &vecurl, std::string &useUrl, const char *pname )
{
	if( vecurl.empty() )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "CreateRTMPInstance(%s) failed! url is null!\r\n", pname?pname:"" );
		return NULL;
	}

	// 数量上限
	const uint32_t SYS_RTMPUrlNumMax = RUNCODE_Get(defCodeIndex_SYS_RTMPUrlNumMax);

	LOGMSG( "CreateRTMPInstance SYS_RTMPUrlNumMax=%u, curNum=%u", SYS_RTMPUrlNumMax, vecurl.size() );

	std::vector<std::string> vecurltemp = vecurl;

	if( IsRUNCODEEnable(defCodeIndex_TEST_UseSpecRTMPUrlList) )
	{
		uint32_t urlsno = RUNCODE_Get(defCodeIndex_TEST_UseSpecRTMPUrlList,defRunCodeValIndex_2);
		
		if( urlsno > 0 && urlsno < vecurl.size() ) // 0时无需调整
		{
			vecurltemp.insert( vecurltemp.begin(), vecurl[urlsno] );

			LOGMSG( "CreateRTMPInstance UseSpecRTMPUrlList vecurl[%d] first. temp size=%d, 0url=%s", urlsno, vecurltemp.size(), vecurltemp[0].c_str() );
		}
	}

	//RTMP *rtmp = NULL;
	//rtmp = RTMP_Alloc();
	RTMP *rtmp = RTMP_Alloc();  //jyc20170626 modify
	RTMP_Init(rtmp);

	bool issuccess = false;
	for( int i=0; i<vecurltemp.size() && i<SYS_RTMPUrlNumMax; ++i )
	{
		string_replace(vecurltemp[i],"www.gsss.cn","101.227.242.220"); //jyc20170720 add
		RTMP_SetupURL(rtmp, (char*)vecurltemp[i].c_str() );
		RTMP_EnableWrite(rtmp);
		
		if(!RTMP_Connect(rtmp,NULL))
		{
			//LOGMSGEX( defLOGNAME, defLOG_ERROR, "CreateRTMPInstance(%s), RTMP_Connect failed! url=%s", pname?pname:"", vecurltemp[i].c_str() );
			continue;
		}
		
		if(!RTMP_ConnectStream(rtmp,0))
		{
			//LOGMSGEX( defLOGNAME, defLOG_ERROR, "CreateRTMPInstance(%s), RTMP_ConnectStream failed! url=%s", pname?pname:"", vecurltemp[i].c_str() );
			LOGMSG( "CreateRTMPInstance(%s), RTMP_ConnectStream failed! url=%s", pname?pname:"", vecurltemp[i].c_str() );
			continue;
		}
		
		issuccess = true;
		useUrl = vecurltemp[i].c_str();
		break;
	}
	if( !issuccess )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "CreateRTMPInstance(%s) all url failed!\r\n", pname?pname:"" );
		goto label_CreateRTMPConnectInstance_End;
	}
	LOGMSG( "CreateRTMPInstance(%s) success. url=%s\r\n", pname?pname:"", useUrl.c_str() );
	return rtmp;

label_CreateRTMPConnectInstance_End:

	if( rtmp )
	{
		RTMP_Close(rtmp);
		RTMP_Free(rtmp);
	}

	return NULL;
}


void RTMPSend::pushRTMPHandle( defRTMPConnectHandle handle, const std::string &useUrl )
{
	gloox::util::MutexGuard mutexguard( m_queue_mutex );

	if( m_RTMPHandle )
	{
		deleteRTMPHandle( m_RTMPHandle );
	}

	m_RTMPHandle = handle;
	m_RTMPHandle_useUrl = useUrl;
}

defRTMPConnectHandle RTMPSend::popRTMPHandle( std::string *useUrl, const bool dopop )
{
	gloox::util::MutexGuard mutexguard( m_queue_mutex );

	defRTMPConnectHandle handle = this->m_RTMPHandle;
	if( useUrl ) *useUrl = this->m_RTMPHandle_useUrl;

	if( dopop )
	{
		this->m_RTMPHandle = NULL;
		this->m_RTMPHandle_useUrl = "";
	}

	return handle;
}

void RTMPSend::Close()
{
	isRunning = false;

	if( !m_isThreadExit )
	{
		DWORD dwStart = ::timeGetTime();
		while( !m_isThreadExit && ::timeGetTime()-dwStart < 30*1000 )
		{
			usleep(1000);
		}
		LOGMSG( "RTMPSend ThreadExit wait usetime=%dms\r\n", ::timeGetTime()-dwStart );
	}

	m_queue_mutex.lock();
	
	m_RTMFPSessionCount = 0;

	if( !videoPackage.empty() )
	{
		while(videoPackage.size()>0){
			H264VideoPackge *packet =  videoPackage.front();
			FinalDeleteVideoPacket( packet );
			videoPackage.pop_front();
		}
		videoPackage.clear();
	}

	if( !videoPackreuse.empty() )
	{
		while(videoPackreuse.size()>0){
			H264VideoPackge *packet =  videoPackreuse.front();
			FinalDeleteVideoPacket( packet );
			videoPackreuse.pop_front();
		}
		videoPackreuse.clear();
	}
	m_queue_mutex.unlock();

	m_pushindex = 0;
}

int RTMPSend::Connect(const std::string& url)
{
	this->setUrl( url );
	return 1;
}

int RTMPSend::SetVideoMetaData(int width,int height,int fps)
{
	this->videoWidth = width;
	this->videoHeight = height;
	this->videoFps = fps;
	return 1;
}

void *RTMPAudioThread(LPVOID lpPara) {
	RTMPSend *rtmp_send = (RTMPSend *) lpPara;

	DWORD nSampleRate = SampleRate;  // 采样率
	UINT nChannels = 1;         // 声道数
	UINT nPCMBitSize = 16;      // 单样本位数
	DWORD nInputSamples = 0;
	DWORD nMaxOutputBytes = 0;

	int nRet;
	faacEncHandle hEncoder;
	faacEncConfigurationPtr pConfiguration;

	int nBytesRead;
	int nPCMBufferSize;
	BYTE* pbPCMBuffer;
	BYTE* pbAACBuffer;

	// (1) Open FAAC engine
	hEncoder = faacEncOpen(nSampleRate, nChannels, &nInputSamples,
			&nMaxOutputBytes);
	if (hEncoder == NULL) {
		printf("[ERROR] Failed to call faacEncOpen()\n");
		//jyc20170525 remove return;
	}

	nPCMBufferSize = nInputSamples * nPCMBitSize / 8;
	pbPCMBuffer = new BYTE[nPCMBufferSize];
	pbAACBuffer = new BYTE[nMaxOutputBytes];

	// (2.1) Get current encoding configuration
	pConfiguration = faacEncGetCurrentConfiguration(hEncoder);
	pConfiguration->inputFormat = FAAC_INPUT_16BIT;

	// (2.2) Set encoding configuration
	nRet = faacEncSetConfiguration(hEncoder, pConfiguration);

	Recorder recorder;
	recorder.initRecoder();

	char *buffer = (char*) malloc(2048 * 2 * 2 * 2);

	int audio_samples_per_frame = 32;
	int audio_sample_rate = nSampleRate;
	int audio_pts_increment = (90000 * audio_samples_per_frame)
			/ audio_sample_rate; //一秒钟采集多少次
	audio_pts_increment /= 2; //单声道除以2

	while (1) {
		unsigned char *buffer2;
		buffer2 = (unsigned char *) malloc(100000 * sizeof(char));
		int size2 = 0;
		//for (int i = 0; i < audio_pts_increment/10; i++) //采集0.1s
		for (int i = 0; i < 1; i++) //采集0.1s
				{
			//fprintf(stderr, "%d\n", audio_pts_increment);
			int size1;
//			for (size1 = 0;;) {
//				char *buf = (char*) (buffer + size1);
//				int readSize = recorder.recode(buf, 32);
//				size1 += readSize * 2;
//				if (size1 >= 2048)
//					break;
//			}
			for (size1 = 0;;) {
				char *buf = (char*) (buffer + size1);
				int readSize = recorder.recode(buf, 64);
				if (readSize==0){ //jyc20170526 add
					usleep(1000);
				}
				size1 += readSize * 2;
				if (size1 >= 2048)
					break;
			}
			nBytesRead = size1;
			// 输入样本数，用实际读入字节数计算，一般只有读到文件尾时才不是nPCMBufferSize/(nPCMBitSize/8);
			nInputSamples = nBytesRead / (nPCMBitSize / 8);
			// (3) Encode
			nRet = faacEncEncode(hEncoder,(int32_t*) buffer, //jyc20170612 int -> int32_t
					nInputSamples,pbAACBuffer,nMaxOutputBytes);
			memcpy(&buffer2[size2], pbAACBuffer, nRet);
			size2 += nRet;
		}
		rtmp_send->PushVideo(false, (char *) buffer2, size2, true); //isaudio
		usleep(10000);
		free(buffer2);
		if(!rtmp_send->isRtmprun()){ //jyc20170525 add
			break; //jyc20170525 add for exit rtmpcap thread
		}
	}
#ifdef OS_ARMLINUX
    recorder.closeRecoder();
#endif
    nRet = faacEncClose(hEncoder);
    delete[] pbAACBuffer;
    free(buffer);
	pthread_detach(pthread_self()); //jyc20170722 add
    //return 0;
}

#ifdef VIDEOH264_FLAG
#define V4L2_PIX_FMT_H264 v4l2_fourcc('H','2','6','4') /* H264 */
#define V4L2_PIX_FMT_MP2T v4l2_fourcc('M','P','2','T') /* MPEG-2 TS */

#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,32)
#define V4L_BUFFERS_DEFAULT	6//16
#define V4L_BUFFERS_MAX		16//32
#else
#define V4L_BUFFERS_DEFAULT	3
#define V4L_BUFFERS_MAX		3
#endif
void *RTMPVideoh264Thread(LPVOID lpPara) {

	RTMPSend *rtmp_send = (RTMPSend *) lpPara;

	unsigned int i;
	int framerate = 30;
	/* Video buffers */
	void *mem0[V4L_BUFFERS_MAX];
//	void *mem1[V4L_BUFFERS_MAX];
//	unsigned int pixelformat = V4L2_PIX_FMT_MJPEG;
	unsigned int width = 1280;
	unsigned int height = 720;
	unsigned int nbufs = V4L_BUFFERS_DEFAULT;
	unsigned int input = 0;
	unsigned int skip = 0;

	struct v4l2_buffer buf0;
	struct v4l2_capability cap;
	struct v4l2_format fmt;
	struct v4l2_requestbuffers rb;
	int dev, ret;

	dev = open("/dev/video1", O_RDWR);
	memset(&cap, 0, sizeof cap);
	ioctl(dev, VIDIOC_QUERYCAP, &cap);

	memset(&fmt, 0, sizeof fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = width;
	fmt.fmt.pix.height = height;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_H264;
	fmt.fmt.pix.field = V4L2_FIELD_ANY;

	ioctl(dev, VIDIOC_S_FMT, &fmt);

	struct v4l2_streamparm parm;

	memset(&parm, 0, sizeof parm);
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	ioctl(dev, VIDIOC_G_PARM, &parm);
	parm.parm.capture.timeperframe.numerator = 1;
	parm.parm.capture.timeperframe.denominator = framerate;

	ioctl(dev, VIDIOC_S_PARM, &parm);

	memset(&rb, 0, sizeof rb);
	rb.count = nbufs;
	rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	rb.memory = V4L2_MEMORY_MMAP;

	ioctl(dev, VIDIOC_REQBUFS, &rb);
	nbufs = rb.count;

	for (i = 0; i < nbufs; ++i) {
		memset(&buf0, 0, sizeof buf0);
		buf0.index = i;
		buf0.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf0.memory = V4L2_MEMORY_MMAP;
		ioctl(dev, VIDIOC_QUERYBUF, &buf0);

		mem0[i] = mmap(0, buf0.length, PROT_READ, MAP_SHARED, dev,
				buf0.m.offset);
	}
	for (i = 0; i < nbufs; ++i) {
		memset(&buf0, 0, sizeof buf0);
		buf0.index = i;
		buf0.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf0.memory = V4L2_MEMORY_MMAP;
		ret = ioctl(dev, VIDIOC_QBUF, &buf0);
	}

	int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ioctl(dev, VIDIOC_STREAMON, &type);

	int n_nal = 0;

	rtmp_send->setRtmpflag(); //jyc20170525 add
	bool keyframe = false;
	unsigned char *buffer = (unsigned char *)malloc(100000*sizeof(char)); //jyc20170720 modify

	while (1){
		int size=0;
		memset(&buf0, 0, sizeof buf0);
		buf0.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf0.memory = V4L2_MEMORY_MMAP;

		ioctl(dev, VIDIOC_DQBUF, &buf0);
		memcpy(buffer,mem0[buf0.index],buf0.bytesused); //jyc20170726 modify
		size=buf0.bytesused;
//		fwrite(mem0[buf0.index], buf0.bytesused, 1, rec_fp);
		ioctl(dev, VIDIOC_QBUF, &buf0);

		int prefixNum = 0;
		int preindex = g_GetH264PreFix(buffer,size>32?32:size,prefixNum ,32);

		// h264 no head err
		if( 0==prefixNum || 0 == preindex  ){
			printf("h264 no head err\n");
		}
		if(preindex>0){
			if(0x65==buffer[preindex+22]){ //jyc20170727 debug
				keyframe=true;
			}else keyframe=false;
		}
//		if (!rtmp_send->HasNAL()
//				&& preindex > 0	&& (0x67 == buffer[preindex] || 0x68 == buffer[preindex]))
		if(!(rtmp_send->getNalSize()>1) && preindex > 0 && (0x67 == buffer[preindex] || 0x68 == buffer[preindex]))
		{
			x264_nal_s nal[2];
			unsigned char *bufnext = g_Get_x264_nal_t(buffer, size, nal[0]);
			if (bufnext) {
				rtmp_send->SetVideoNal(&nal[0], 1);
				if (g_Get_x264_nal_t(bufnext, size - (nal[0].i_payload + 3),nal[1])) {
					rtmp_send->SetVideoNal(&nal[0], 2); //jyc20170518 modigy
					keyframe=true; //just test
				}
			}
			Delete_x264_nal_t(nal[0]);
			Delete_x264_nal_t(nal[1]);
		}

		rtmp_send->PushVideo(keyframe,(char *)buffer,size,false); //
		usleep(100000);
		if(!rtmp_send->isRtmprun()){ //jyc20170525 add
			rtmp_send->delNal();
			rtmp_send->setNalSize(0);
			break; //jyc20170525 add for exit rtmpcap thread
		}
	}
	
	free(buffer);
	close(dev); //jyc20170726 add
//	VideoRelease(encoder); //jyc20170525 add
	pthread_detach(pthread_self()); //jyc20170722 add
	//return 0;
}
#else
void *RTMPVideoThread(LPVOID lpPara) {

	RTMPSend *rtmp_send = (RTMPSend *) lpPara;
	my_x264_encoder * encoder  = VideoInit(); //jyc20170519 modify
	int n_nal = 0;
	x264_picture_t pic_out;
	x264_nal_t *my_nal;
	rtmp_send->setRtmpflag(); //jyc20170525 add
	bool keyframe = false;
	unsigned char *buffer = (unsigned char *)malloc(100000*sizeof(char)); //jyc20170720 modify
	while (1){

		int size=0;
		//for (int i = 0; i < 1; i++) { //jyc20170524 remove
			getyuv(encoder->yuv);
			encoder->yuv420p_picture->i_pts++;
			int ret; //jyc20170519 add
			if ((ret = x264_encoder_encode(encoder->x264_encoder, &encoder->nal,
					&n_nal, encoder->yuv420p_picture, &pic_out)) < 0) {
				printf("x264_encoder_encode error!\n");
				exit(EXIT_FAILURE);
			}
			for (my_nal = encoder->nal; my_nal < encoder->nal + n_nal; ++my_nal) {
				//write(fd_write, my_nal->p_payload, my_nal->i_payload);
				memcpy(&buffer[size], my_nal->p_payload, my_nal->i_payload);
				size += my_nal->i_payload;
			}
		//}
		int prefixNum = 0;
		int preindex = g_GetH264PreFix(buffer,size>32?32:size,prefixNum ,32);
//		printf("\n");
//		for(int i=0;i<32;i++)printf(" %x",buffer[i]);
//		printf("  size=%d preindex=%x buffer[preindex]=%x\n",size,preindex,buffer[preindex]);
		// h264 no head err
		if( 0==prefixNum || 0 == preindex  ){
			printf("h264 no head err\n");
		}
		if(preindex>0){
			if(0x65==buffer[preindex]){
				keyframe=true;
			}
		}
//		if (!rtmp_send->HasNAL()
//				&& preindex > 0	&& (0x67 == buffer[preindex] || 0x68 == buffer[preindex]))
		if(!(rtmp_send->getNalSize()>1) && preindex > 0 && (0x67 == buffer[preindex] || 0x68 == buffer[preindex]))
		{
			x264_nal_s nal[2];
			unsigned char *bufnext = g_Get_x264_nal_t(buffer, size, nal[0]);
			if (bufnext) {
				rtmp_send->SetVideoNal(&nal[0], 1);
				if (g_Get_x264_nal_t(bufnext, size - (nal[0].i_payload + 3),nal[1])) {
					rtmp_send->SetVideoNal(&nal[0], 2); //jyc20170518 modigy
					keyframe=true; //just test
				}
			}
			Delete_x264_nal_t(nal[0]);
			Delete_x264_nal_t(nal[1]);
		}
		rtmp_send->PushVideo(keyframe,(char *)buffer,size,false); //
		usleep(200000);
		
		if(!rtmp_send->isRtmprun()){ //jyc20170525 add
			rtmp_send->delNal();
			rtmp_send->setNalSize(0);
			break; //jyc20170525 add for exit rtmpcap thread
		}
	}
	free(buffer); //jyc20170721 move here
	free(encoder->yuv);
	//x264_picture_clean(encoder->yuv420p_picture); //jyc20170722 add but error
	free(encoder->yuv420p_picture);
	free(encoder->x264_parameter);
	x264_encoder_close(encoder->x264_encoder);
	free(encoder);
	VideoRelease(); //jyc20170525 add
	pthread_detach(pthread_self()); //jyc20170722 add
}
#endif

int RTMPSend::Run()
{
	pthread_t id_1,id_2,id_3;
    int ret;
    if(!this->isRunning){
    	   this->Close();
    	   this->isRunning = true;
    	   this->m_runningkey++;
    	   this->m_lastGetVideoPacketTime = timeGetTime();
    	   m_isThreadExit = false;

#ifdef VIDEOH264_FLAG
    	   ret=pthread_create(&id_2,NULL,RTMPVideoh264Thread,this);
    	   if(ret!=0){
    		   	  printf("Create RTMPVideoh264Thread error!\n");
    	   }
#else
    	   ret=pthread_create(&id_2,NULL,RTMPVideoThread,this);
    	   if(ret!=0){
    		   	  printf("Create RTMPVideoThread error!\n");
    	   }
#endif
    	   ret=pthread_create(&id_3,NULL,RTMPAudioThread,this);
    	   if(ret!=0){
    		   	  printf("Create RTMPAudioThread error!\n");
    	   }
    	   ret=pthread_create(&id_1, NULL, RTMPPushThread, this );
    	   if(ret!=0) {
    	          printf("Create RTMPPushThread error!\n");
    	   }
    	   this->m_handler->OnPublishStart();
    }

	return this->isRunning;
}

void RTMPSend::SetVideoNal(x264_nal_s *nal,int i_nal)
{
	gloox::util::MutexGuard mutexguard( m_queue_mutex );
//	Delete_x264_nal_t( &videoNal, nalSize ); //jyc20170518 remove
//	Copy_x264_nal_t( &this->videoNal, this->nalSize, nal, i_nal ); //jyc20170518 modify
//	if (this->nalSize <= 0){
//		Copy_x264_nal_t(&this->videoNal,this->nalSize,nal,i_nal);
//		this->nalSize = 1;
//	} else if(this->nalSize <=1){
//		Copy_x264_nal_t(&this->videoNal,this->nalSize,nal,i_nal);
//		this->nalSize =2;
//	}
	Copy_x264_nal_t(&this->videoNal,this->nalSize,nal,i_nal);
}

x264_nal_s* RTMPSend::getNal()
{
	gloox::util::MutexGuard mutexguard( m_queue_mutex );

	char chbuf[256] = {0};
	for( int i=0; i<nalSize; ++i )
	{//jyc20170518 remove
//		snprintf( chbuf, sizeof(chbuf), "RTMPSend(%s)::getNal(%d/%d)", this->m_name.c_str(), i+1, nalSize );
//		g_PrintfByte( videoNal[i].p_payload, videoNal[i].i_payload>64?64:videoNal[i].i_payload, chbuf );
	}
	return this->videoNal;
}

int RTMPSend::getNalSize()
{
	gloox::util::MutexGuard mutexguard( m_queue_mutex );

	return this->nalSize;
}

void RTMPSend::setNalSize(int size) //jyc20170518 add
{
	gloox::util::MutexGuard mutexguard( m_queue_mutex );

	this->nalSize = size;
}

void RTMPSend::delNal()
{
	gloox::util::MutexGuard mutexguard( m_queue_mutex );

	Delete_x264_nal_t( &videoNal, nalSize );
}

void RTMPSend::PushVideo1( const bool keyframe, char *buffer, int size, uint32_t timestamp, const bool isAudio )
{
	if( isAudio )
	{
		if( !this->get_playback_sound() )
		{
			return;
		}

		if( this->GetIPlayBack() )
		{
			if( !IsRUNCODEEnable(defCodeIndex_SYS_PlayBackSound) ) // Not PlayBack Sound
			{
				return;
			}
		}
	}

	m_pushindex++;
	//LOGMSG( "RTMPSend src(%s)::PushVideo(index=%d) ts=%u, key=%d, audio=%d, size=%u\r\n", m_name.c_str(), m_pushindex, (uint32_t)timestamp, keyframe, isAudio, size );

	if(wait_frist_key_frame)
	{
		if(keyframe)
		{
			m_pushindex = 0;
			LOGMSG( "RTMPSend(%s)::PushVideo(index=%u) set key=true, set wait=false, ts=%u, size=%d", this->m_name.c_str(), m_pushindex, timestamp, size );
			startTime = timestamp;
			m_lasttsVideoPackageList = 0;
			m_startTime_tick = ::timeGetTime();
			wait_frist_key_frame = false;
			m_prev_tick_PrintFrame = 0;
		}
		else
		{
			return;
		}
	}
	if( !isAudio )
	{
		// h264 no head err
		int prefixNum = 0;
		int preindex = g_GetH264PreFix( (unsigned char*)buffer, size>16?16:size, prefixNum );

		if( 0==prefixNum || 0 == preindex  )
		{
			char chbuf[256] = {0};
			snprintf( chbuf, sizeof(chbuf), "RTMPSend(%s)::PushVideo(index=%u) prefixNum=0 err! size=%d", this->m_name.c_str(), m_pushindex, size );

			g_PrintfByte( (unsigned char*)buffer, size>32?32:size, chbuf );
		}

		buffer += prefixNum;
		size -= prefixNum;
	}


	if( size > MAX_VIDEO_PACKET_SIZE )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "RTMPSend(%s)::PushVideo packet too large, packetSize=%u\r\n", m_name.c_str(), size );
		return;
	}

	uint32_t quesize = 0;
	H264VideoPackge *packet = newVideoPacket( size, keyframe );

	packet->isAudio = isAudio;
	packet->keyframe = keyframe;
	packet->size = size;
	memcpy(packet->buf,buffer,size);
	packet->timeStamp = timestamp;
	//packet->size = g_h264_remove_all_start_code( (uint8_t*)packet->buf, packet->size );

	m_queue_mutex.lock();


	packet->index = ++m_packet_index;
	this->videoPackage.push_back(packet);
	quesize = this->videoPackage.size();

	// ������ڴ�ֵʱ��ʼ�����ͷţ��ͷŵ�һ����
	if( videoPackage.size()>GetQueMaxSize()  )
	{
		//LOGMSGEX( defLOGNAME, defLOG_WORN, "RTMPSend(%s)::PushVideo videoPackage full, release packet, beforeCount=%u\r\n", m_name.c_str(), videoPackage.size() );

		while( videoPackage.size()>100 )
		{
			H264VideoPackge *packet =  videoPackage.front();
			ReleaseVideoPacket_nolock( packet );
			videoPackage.pop_front();
		}

		//LOGMSGEX( defLOGNAME, defLOG_WORN, "RTMPSend(%s)::PushVideo videoPackage full, release packet, AfterCount=%u\r\n", m_name.c_str(), videoPackage.size() );
	}

	m_queue_mutex.unlock();

	bool PrintQueInfo = false;

	if( PrintQueInfo )
	{
		if( quesize<5 ) // �Ѿ��ڴ�ӡʱ�����ٵ�С�ڴ�ֵ�Ų���ӡ
		PrintQueInfo = false;
	}

	if( quesize>9 && !this->GetIPlayBack() && 0==(quesize%10) ) // ���ڴ�ֵ�ſ�ʼ��ӡ
		PrintQueInfo = true;

	if( quesize>200 && 0==(quesize%200) ) // ���ڴ�ֵ�ſ�ʼ��ӡ
		PrintQueInfo = true;

	if( IsRUNCODEEnable(defCodeIndex_RTMPSend_PrintFrame) ) // if( !isAudio && IsRUNCODEEnable(defCodeIndex_RTMPSend_PrintFrame) )
	{
		if( 0 == RUNCODE_Get(defCodeIndex_RTMPSend_PrintFrame,defRunCodeValIndex_2) ) // ���Ǵ�ӡ
		{
			PrintQueInfo = true;
		}
		else if( keyframe && RUNCODE_Get(defCodeIndex_RTMPSend_PrintFrame,defRunCodeValIndex_3) )
		{
			PrintQueInfo = true;
		}
		else if( ::timeGetTime()-m_prev_tick_PrintFrame >= (DWORD)RUNCODE_Get(defCodeIndex_RTMPSend_PrintFrame,defRunCodeValIndex_2) )
		{
			PrintQueInfo = true;
		}
	}

	if( PrintQueInfo )
	{
		LOGMSG( "RTMPSend(%s)::PushVideo(index=%u) ts=%u, key=%d, audio=%d, quesize=%u, size=%u\r\n", m_name.c_str(), m_pushindex, (uint32_t)timestamp, keyframe, isAudio, quesize, size );
		m_prev_tick_PrintFrame = ::timeGetTime();
	}
}

void RTMPSend::PushVideo( const bool keyframe, char *buffer, int size, const bool isAudio )
{
	uint32_t timestamp = timeGetTime();

	PushVideo1(keyframe, buffer, size, timestamp, isAudio );
}

void RTMPSend::OnSendThreadExit(defGSReturn code)
{
	LOGMSG( "name=%s, OnSendThreadExit(%d:%s)%s\r\n", this->getName().c_str(), code, g_Trans_GSReturn(code).c_str(), this->GetIPlayBack()?" Playback":"" );

	m_isThreadExit = true;
	if(isRunning){
	   isRunning = false;
	   this->m_handler->OnPublishStop(code);
	}
}

H264VideoPackge* RTMPSend::getVideoPacket()
{
	H264VideoPackge* packet = NULL;

	m_queue_mutex.lock();

	this->m_lastGetVideoPacketTime = timeGetTime();

	if( this->GetIPlayBack() )
	{
		if( GSPlayBackCode_PLAYPAUSE == m_PlayBackCtrl_Code )
		{
			m_queue_mutex.unlock();
			return packet;
		}
	}

	if( !this->videoPackage.empty() )
	{
		packet = this->videoPackage.front();
		this->videoPackage.pop_front();
	}

	// ts
	if( packet )
	{
		if( this->GetIPlayBack() )
		{
			switch( m_PlayBackCtrl_Code )
			{
			case GSPlayBackCode_PLAYFAST:
			case GSPlayBackCode_PLAYSLOW:
				{
					if( m_PlayBackCtrl_speedlevel >=2 )
					{
						packet->timeStamp -= ( packet->timeStamp - m_PlayBackCtrl_ts )*4/5;
					}
					else if( m_PlayBackCtrl_speedlevel >=1 )
					{
						packet->timeStamp -= ( packet->timeStamp - m_PlayBackCtrl_ts )/2;
					}
					else if( m_PlayBackCtrl_speedlevel <= -2 )
					{
						packet->timeStamp += ( packet->timeStamp - m_PlayBackCtrl_ts );
					}
					else if( m_PlayBackCtrl_speedlevel <= -1 )
					{
						packet->timeStamp += ( packet->timeStamp - m_PlayBackCtrl_ts )/2;
					}
					else
					{
						ResetPlayBackCtrlFlag();
					}
				}
				break;

			case GSPlayBackCode_PLAYNORMAL:
				{
					ResetPlayBackCtrlFlag();
				}
				break;

			case GSPlayBackCode_SkipTime: // ��Ƶ��ת
				{
					const uint32_t SkipTime_time = m_doSkipTime_timems>=10000 ? (m_doSkipTime_timems-3000) : m_doSkipTime_timems;
					const uint32_t curFrameTSSpan = packet->timeStamp - m_PlayBackCtrl_ts;
					if( curFrameTSSpan < SkipTime_time )
					{
						ReleaseVideoPacket_nolock( packet );
						packet = NULL;
						m_queue_mutex.unlock();
						return NULL;
					}
					else
					{
						ResetPlayBackCtrlFlag();

						if( !packet->keyframe )
						{
							RePreFirstKeyListVideoPacket_nolock();
						}
					}
				}
				break;
			}
		}


#if defined(defUse_TSStartTime)
		packet->timeStamp -= startTime;
#else
#endif

		if( !this->GetIPlayBack() )
		{

		}
		else
		{
			if( packet->timeStamp - m_lasttsVideoPackageList > 2000
				//&& !( m_PlayBackCtrl_ThrowFrame && GSPlayBackCode_PLAYFAST==m_PlayBackCtrl_Code && 1==m_PlayBackCtrl_speedlevel ) // �����֡ģʽ��ʱ�����������
				)
			{
				LOGMSG( "RTMPSend(%s)::getVideoPacket reset startTime, oldstartTime=%u, curts=%u!!!", this->m_name.c_str(), startTime, packet->timeStamp );
				startTime += (packet->timeStamp - m_lasttsVideoPackageList);
				packet->timeStamp = m_lasttsVideoPackageList;
			}
		}

		if( packet->timeStamp < m_lasttsVideoPackageList )
		{
			packet->timeStamp = m_lasttsVideoPackageList;
		}


		if( this->GetIPlayBack() )
		{
			switch( m_PlayBackCtrl_Code )
			{
			case GSPlayBackCode_PLAYNORMAL:
			case GSPlayBackCode_PLAYFAST:
				{
					bool doThrowFrame = packet->isAudio;

					if( !doThrowFrame && !packet->keyframe && m_PlayBackCtrl_speedlevel >=1 ) // is fastplay
					{
						if( m_PlayBackCtrl_ThrowFrame )
						{
							doThrowFrame = true;
						}
					}

					if( doThrowFrame )
					{
						//m_lastThrowIndex = packet->index;
						//LOGMSG( "throwpacket %d", packet->index );
						ReleaseVideoPacket_nolock( packet );
						packet = NULL;
						m_queue_mutex.unlock();
						return NULL;
					}
				}
				break;
			}
		}

		m_lasttsVideoPackageList = packet->timeStamp;
	}
	m_queue_mutex.unlock();
	return packet;
}

void RTMPSend::ResetPlayBackCtrlFlag( bool resetall )
{
	if( resetall )
	{
		m_PlayBackCtrl_Code = GSPlayBackCode_NULL;
	}

	m_PlayBackCtrl_ts = 0;
	m_doSkipTime_timems = 0;
	m_PlayBackCtrl_speedlevel = 0;
	m_PlayBackCtrl_ThrowFrame = 0;
	m_PlayBackCtrl_pause_oldcode = GSPlayBackCode_NULL;
}

int RTMPSend::PlayBackControl( GSPlayBackCode_ ControlCode, uint32_t InValue )
{
	if( !this->GetIPlayBack() )
		return false;

	switch( ControlCode )
	{
	case GSPlayBackCode_SkipTime:
		{
			if( 0==InValue )
				return false;
		}
		break;

	default:
		break;
	}

	m_queue_mutex.lock();

	const GSPlayBackCode_ oldcode = m_PlayBackCtrl_Code;
	m_PlayBackCtrl_Code = ControlCode;
	m_PlayBackCtrl_ts = startTime + m_lasttsVideoPackageList;

	switch( m_PlayBackCtrl_Code )
	{
	case GSPlayBackCode_SkipTime:
		{
			m_doSkipTime_timems = InValue;
			m_PlayBackCtrl_pause_oldcode = oldcode;
		}
		break;

	case GSPlayBackCode_PLAYFAST:
	case GSPlayBackCode_PLAYSLOW:
		{
			m_PlayBackCtrl_speedlevel += (GSPlayBackCode_PLAYFAST==m_PlayBackCtrl_Code) ? 1 : -1;
			
			if( 0==m_PlayBackCtrl_speedlevel )
			{
				ReKeyListVideoPacket_nolock();
				ResetPlayBackCtrlFlag();
			}
			else
			{
				if( m_PlayBackCtrl_speedlevel > 2 )
				{
					m_PlayBackCtrl_speedlevel = 2;
				}
				else if( m_PlayBackCtrl_speedlevel < -2 )
				{
					m_PlayBackCtrl_speedlevel = -2;
				}

				if( m_PlayBackCtrl_speedlevel > 0 )
				{
					m_PlayBackCtrl_Code = GSPlayBackCode_PLAYFAST;
				}
				else if( m_PlayBackCtrl_speedlevel < 0 )
				{
					m_PlayBackCtrl_Code = GSPlayBackCode_PLAYSLOW;
				}

				if( m_PlayBackCtrl_ThrowFrame && !InValue )
				{
					RePreFirstKeyListVideoPacket_nolock();
				}

				if( GSPlayBackCode_PLAYFAST == m_PlayBackCtrl_Code )
				{
					m_PlayBackCtrl_ThrowFrame = InValue;
				}
			}
			LOGMSG( "RTMPSend(%s):PlayBackControl speedlevel=%d, ThrowFrame=%d\r\n", this->getName().c_str(), m_PlayBackCtrl_speedlevel, m_PlayBackCtrl_ThrowFrame );
		}
		break;

	case GSPlayBackCode_PLAYNORMAL:
		{
			if( m_PlayBackCtrl_ThrowFrame )
			{
				RePreFirstKeyListVideoPacket_nolock();
			}
			ResetPlayBackCtrlFlag();
		}
		break;

	case GSPlayBackCode_PLAYPAUSE:
		{
			if( GSPlayBackCode_PLAYPAUSE != oldcode )
			{
				m_PlayBackCtrl_pause_oldcode = oldcode;
			}
		}
		break;

	case GSPlayBackCode_PLAYRESTART:
		{
			if( GSPlayBackCode_PLAYPAUSE == oldcode )
			{
				m_PlayBackCtrl_Code = m_PlayBackCtrl_pause_oldcode;
				m_PlayBackCtrl_pause_oldcode = GSPlayBackCode_NULL;
			}
		}
		break;

	default:
		break;
	}

	m_queue_mutex.unlock();
	return true;
}

int RTMPSend::HasVideoPacket()
{
	uint32_t quesize = 0;
	m_queue_mutex.lock();
	quesize = this->videoPackage.size();
	m_queue_mutex.unlock();
	return quesize;
}

H264VideoPackge* RTMPSend::newVideoPacket( int needsize, bool keyframe )
{
	H264VideoPackge *packet = NULL;
	m_queue_mutex.lock();
	if( !videoPackreuse.empty() )
	{
		if( videoPackreuse.size()>GetReuseQueMaxSize() )
		{
			//LOGMSGEX( defLOGNAME, defLOG_WORN, "newVideoPacket(%s) videoPackreuse full, release packet, beforeCount=%u\r\n", m_name.c_str(), videoPackreuse.size() );
			const int minQueSize = GetReuseQueMaxSize()/2;
			while( videoPackreuse.size()>minQueSize )
			{
				H264VideoPackge *packet =  videoPackreuse.front();
				FinalDeleteVideoPacket( packet );
			}
			//LOGMSGEX( defLOGNAME, defLOG_WORN, "newVideoPacket(%s) videoPackreuse full, release packet, AfterCount=%u\r\n", m_name.c_str(), videoPackreuse.size() );
		}

		std::deque<H264VideoPackge*>::iterator it = videoPackreuse.begin();
		for( ; it!=videoPackreuse.end(); ++it )
		{
			if( (*it)->bufLimit > needsize )
			{
				packet = (*it);
				videoPackreuse.erase(it);
				break;
			}
		}
	}
	m_queue_mutex.unlock();

	if( !packet )
	{//jyc20170517 resume modify no memorycontainer
#if defuseMemoryContainer
		int c_bufLimit = needsize>CRM_MEMORY_MIN_LEN ? needsize:CRM_MEMORY_MIN_LEN;
		
		char *c_pbuf = g_GetMemoryContainer()->GetMemory( c_bufLimit + sizeof(H264VideoPackge) ); // 将结构和缓存分配在同一连续内存区域

		packet = (H264VideoPackge*)c_pbuf;
		packet->bufLimit = c_bufLimit;
		packet->buf = c_pbuf + sizeof(H264VideoPackge);
#else
		packet = new H264VideoPackge();
		packet->bufLimit = needsize>MINNEW_VIDEO_PACKET_SIZE ? needsize:MINNEW_VIDEO_PACKET_SIZE;
		if( keyframe ) packet->bufLimit += KEYNEWOVER_VIDEO_PACKET_SIZE;
		packet->buf = new char[packet->bufLimit];
#endif
	}
	return packet;
}

void RTMPSend::ClearListVideoPacket()
{
	m_queue_mutex.lock();

	LOGMSG( "RTMPSend::ClearListVideoPacket(%s) size=%d\r\n", m_name.c_str(), videoPackage.size() );

	while( !videoPackage.empty() )
	{
		H264VideoPackge *packet =  videoPackage.front();
		ReleaseVideoPacket_nolock( packet );
		videoPackage.pop_front();
	}

	m_queue_mutex.unlock();
}

void RTMPSend::RePreFirstKeyListVideoPacket()
{
	m_queue_mutex.lock();

	RePreFirstKeyListVideoPacket_nolock();

	m_queue_mutex.unlock();
}

void RTMPSend::RePreFirstKeyListVideoPacket_nolock()
{
	if( videoPackage.empty() )
	{
		LOGMSG( "RTMPSend::RePreFirstKeyList(%s) list is empty\r\n", m_name.c_str() );
	}
	else
	{
		const uint32_t sizebefore = videoPackage.size();
		// ��һ��keyframe֮ǰ��ȫ�ӵ���keyframe���?֮�������֡ȫ����
		while( !videoPackage.empty() )
		{
			H264VideoPackge *packet =  videoPackage.front();
			if( packet->keyframe )
			{
				break;
			}

			ReleaseVideoPacket_nolock( packet );
			videoPackage.pop_front();
		}

		const uint32_t sizeafter = videoPackage.size();

		LOGMSG( "RTMPSend::RePreFirstKeyList(%s) after do, sizebefore=%d, sizeafter=%d.\r\n", m_name.c_str(), sizebefore, sizeafter );
	}
}

void RTMPSend::ReKeyListVideoPacket()
{
	m_queue_mutex.lock();

	ReKeyListVideoPacket_nolock();

	m_queue_mutex.unlock();
}

void RTMPSend::ReKeyListVideoPacket_nolock()
{
	if( videoPackage.empty() )
	{
		LOGMSG( "RTMPSend::ReKeyList(%s) list is empty\r\n", m_name.c_str() );
	}
	else
	{
		std::deque<H264VideoPackge*>::iterator it = videoPackage.begin();
		std::deque<H264VideoPackge*>::iterator itEnd = videoPackage.end();

		const bool first_is_key = (*it)->keyframe;
		const uint32_t sizebefore = videoPackage.size();

		uint32_t keycount = 0;
		for( ; it != itEnd; ++it )
		{
			//LOGMSG( "RTMPSend::ReKeyList i=%d \r\n", (*it)->keyframe, (*it)->keyframe?"!!!!!!!!!!!!!!!!!!!!!":"" );
			if( (*it)->keyframe )
			{
				keycount++;
			}
		}

		if( !first_is_key || keycount > 1 )
		{
			LOGMSG( "RTMPSend::ReKeyList(%s) keycount=%d, first_is_key=%d, do relist!\r\n", m_name.c_str(), keycount, first_is_key );
			// ֻ����һ��keyframe��ͷ
			while( !videoPackage.empty() )
			{
				H264VideoPackge *packet =  videoPackage.front();
				if( packet->keyframe )
				{
					if( keycount<=1 )
					{
						break;
					}
					keycount--;
				}

				ReleaseVideoPacket_nolock( packet );
				videoPackage.pop_front();
			}

			const uint32_t sizeafter = videoPackage.size();
			LOGMSG( "RTMPSend::ReKeyList(%s) after do, sizebefore=%d, sizeafter=%d.\r\n", m_name.c_str(), sizebefore, sizeafter );
		}
		else
		{
			LOGMSG( "RTMPSend::ReKeyList(%s) keycount=%d, sizebefore=%d, not relist.\r\n", m_name.c_str(), keycount, sizebefore );
		}
	}
}

// ֻ�������һ��key�����key֮ǰ��֮��������ȫ�����
uint32_t RTMPSend::ReListOfOneKey()
{
	uint32_t keyts = 0;

	m_queue_mutex.lock();

	if( videoPackage.empty() )
	{
		LOGMSG( "RTMPSend::ReListOfOneKey(%s) list is empty\r\n", m_name.c_str() );
	}
	else
	{
		const uint32_t sizebefore = videoPackage.size();
		std::deque<H264VideoPackge*>::iterator it = videoPackage.begin();
		std::deque<H264VideoPackge*>::iterator itEnd = videoPackage.end();

		// �޳��key��
		for( ; it != itEnd; ++it )
		{
			H264VideoPackge *packet = (*it);

			if( packet->keyframe )
				continue;

			ReleaseVideoPacket_nolock( packet );
			videoPackage.erase(it);
			it = videoPackage.begin();
			itEnd = videoPackage.end();
		}

		// ֻ����һ��key
		while( videoPackage.size()>1 )
		{
			H264VideoPackge *packet =  videoPackage.front();
			ReleaseVideoPacket_nolock( packet );
			videoPackage.pop_front();
		}

		if( !videoPackage.empty() )
		{
			H264VideoPackge *packet =  videoPackage.front();
			keyts = packet->timeStamp;
		}

		const uint32_t sizeafter = videoPackage.size();
		LOGMSG( "RTMPSend::ReListOfOneKey(%s) after do, sizebefore=%d, sizeafter=%d, keyts=%u\r\n", m_name.c_str(), sizebefore, sizeafter, keyts );
	}

	m_queue_mutex.unlock();

	return keyts;
}

void RTMPSend::ReleaseVideoPacket( H264VideoPackge *p )
{
	m_queue_mutex.lock();
	ReleaseVideoPacket_nolock( p );
	m_queue_mutex.unlock();
}

void RTMPSend::ReleaseVideoPacket_nolock( H264VideoPackge *p )
{
#if defuseMemoryContainer // defuseMemoryContainer
	FinalDeleteVideoPacket(p);
#else
	this->videoPackreuse.push_back(p);
#endif
	//LOGMSG( "videoPackreuse %d\r\n", videoPackreuse.size() );
}

void RTMPSend::FinalDeleteVideoPacket( H264VideoPackge *p )
{//jyc20170720 modify
#if defuseMemoryContainer // defuseMemoryContainer
	g_GetMemoryContainer()->ReleaseMemory( (char*)p, p->bufLimit+sizeof(H264VideoPackge) );
#else
	delete [](p->buf);
	delete p;
#endif
}

bool RTMPSend::IsReady()
{
	if( wait_frist_key_frame )
		return false;
	if( HasVideoPacket()<1 )
		return false;
	return true;
}

bool RTMPSend::IsRtmpLive()
{
	m_queue_mutex.lock();

	const bool iscurlive = ( (timeGetTime()-m_lastGetVideoPacketTime) < 60000 );

	m_queue_mutex.unlock();

	return iscurlive;
}

uint32_t RTMPSend::GetQueMaxSize() const
{
	if( GetIPlayBack() )
	{
		return 2000;
	}

	return 200;
}

uint32_t RTMPSend::GetReuseQueMaxSize() const
{
#if defuseMemoryContainer
	if( GetIPlayBack() )
	{
		return 100;
	}

	return 100;
#else
	if( GetIPlayBack() )
	{
		return 900;
	}

	return 500;
#endif
}

std::string RTMPSend::getUrl()
{
	gloox::util::MutexGuard mutexguard( m_queue_mutex );

	return this->rtmpUrl;
}

void RTMPSend::setUrl( const std::string& url )
{
	gloox::util::MutexGuard mutexguard( m_queue_mutex );

	this->rtmpUrl = url;

	if( this->rtmpUrl.empty() )
	{
		m_PeerID = "";
	}

	if( this->m_PeerID.empty() )
	{
		this->m_P2PUrl = "";
	}
	else
	{
		if( g_IsRTMFP_url( this->rtmpUrl ) )
		{
			this->m_P2PUrl = this->rtmpUrl;
		}
	}
}

std::string RTMPSend::getStreamID()
{
	gloox::util::MutexGuard mutexguard( m_queue_mutex );

	return this->m_StreamID;
}

void RTMPSend::setStreamID( const std::string& StreamID )
{
	gloox::util::MutexGuard mutexguard( m_queue_mutex );

	this->m_StreamID = StreamID;
}

std::string RTMPSend::getP2PUrl()
{
	gloox::util::MutexGuard mutexguard( m_queue_mutex );

	return this->m_P2PUrl;
}

std::string RTMPSend::getPeerID()
{
	gloox::util::MutexGuard mutexguard( m_queue_mutex );

	return this->m_PeerID;
}

void RTMPSend::setPeerID( const std::string& PeerID )
{
	gloox::util::MutexGuard mutexguard( m_queue_mutex );

	this->m_PeerID = PeerID;
	
	if( this->m_PeerID.empty() )
	{
		this->m_P2PUrl = "";
	}
	else
	{
		if( g_IsRTMFP_url(this->rtmpUrl) )
		{
			this->m_P2PUrl = this->rtmpUrl;
		}
	}
}

uint32_t RTMPSend::getRTMFPSessionCount()
{
	gloox::util::MutexGuard mutexguard( m_queue_mutex );

	return this->m_RTMFPSessionCount;
}

void RTMPSend::setRTMFPSessionCount( const uint32_t RTMFPSessionCount )
{
	gloox::util::MutexGuard mutexguard( m_queue_mutex );

	this->m_RTMFPSessionCount = RTMFPSessionCount;
}

