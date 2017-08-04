#include "Audio_WAV2AAC.h"
//#include "DirectShow/Include/qedit.h"
//#include "DirectShow/Include/dshow.h"
#include "faaccfg.h"
#include "AudioTestDefine.h"

#if defined(defTest_AudioSaveWAV2AAC)
static FILE *fstreamsrc = NULL;
#endif


Audio_WAV2AAC::Audio_WAV2AAC()
	: m_bitbuf(NULL), m_bitbufSize(0)
{
	m_inputSamples = 0;
	m_maxOutputBytes = 0;
	m_hencode = NULL;
	//m_pcm_t = NULL;
	m_PCM_buf = NULL;
	m_aacObjectType = defAACObjectType_Default;
}

Audio_WAV2AAC::~Audio_WAV2AAC(void)
{
	this->CloseAudio();

	if( m_bitbuf )
	{
		delete []m_bitbuf;
		m_bitbuf = NULL;
		m_bitbufSize = 0;
	}
}

void Audio_WAV2AAC::ReleaseRes_AAC()
{
	faacEncHandle temp_hencode = NULL;
	if( m_hencode )
	{
		temp_hencode = m_hencode;
		m_hencode = NULL;
	}
	/* jyc20170323 remove
	if( m_pcm_t )
	{
		delete m_pcm_t;
		m_pcm_t = NULL;
	}*/

	if( m_PCM_buf )
	{
		delete []m_PCM_buf;
		m_PCM_buf = NULL;
	}

	m_inputSamples = 0;
	m_maxOutputBytes = 0;
	/*jyc20170323 remove
	if( temp_hencode )
		faacEncClose(temp_hencode);

	m_bufMerge.ResetDataBuf();
	*/
}

bool Audio_WAV2AAC::OpenAudio( const CAudioCfg &AudioCfg )
{
	int Audio_Channels = 0;
	int Audio_bitSize = 0;
	int Audio_SampleRate = 0;
	int Audio_ByteRate = 0;
	AudioCfg.get_param_use( Audio_Channels, Audio_bitSize, Audio_SampleRate, Audio_ByteRate );

	if( Audio_Channels <= 0 )
		return false;
	/*
	if( !m_pcm_t )
	{
		m_pcm_t = new pcmfile_t;//(pcmfile_t*)malloc(sizeof(pcmfile_t));
		memset( m_pcm_t, 0, sizeof(pcmfile_t) );
	}

	m_pcm_t->samplebytes = Audio_bitSize/8;
	m_pcm_t->channels = Audio_Channels;
	m_pcm_t->samplerate = Audio_SampleRate;
	m_pcm_t->bigendian = 0;
	
	m_hencode = faacEncOpen( m_pcm_t->samplerate, m_pcm_t->channels, &m_inputSamples, &m_maxOutputBytes );
	if( m_hencode==NULL )
	{
		return false;
	}
		
	if( m_hencode )
	{
		m_PCM_buf = new int32_t[m_inputSamples*m_pcm_t->samplebytes];//(int32_t *)malloc(m_inputSamples*sizeof(int32_t));
		//bitbuf = new unsigned char[m_maxOutputBytes];//(unsigned char*)malloc(m_maxOutputBytes*sizeof(unsigned char));
		memset( m_PCM_buf, 0, sizeof(int32_t)*m_inputSamples*m_pcm_t->samplebytes );
		
		faacEncConfigurationPtr fec = faacEncGetCurrentConfiguration( m_hencode );

		fec->mpegVersion = 4;
		fec->outputFormat = 0; // flash player 只支持Raw AAC播放，采用ADTS时aac文件可以播放，flash player不能播放。201403
		fec->quantqual = 100;
		fec->aacObjectType = defAACObjectType_Default;

		//fec->mpegVersion	=0;
		//fec->aacObjectType	=0;
		//fec->allowMidside	=1;
		//fec->useLfe	=0;
		//fec->useTns	=1;
		//fec->bitRate	=0;
		//fec->bandWidth	=0;
		//fec->quantqual	=100;
		//fec->outputFormat	=1;
		//fec->inputFormat	=3;

		m_aacObjectType = fec->aacObjectType;

		if( !faacEncSetConfiguration( m_hencode, fec ) )
		{
			//LOGMSGEX( defLOGNAME, defLOG_INFO, "faacEncSetConfiguration failed!" );

			this->ReleaseRes_AAC();
			return false;
		}
	}
	else
	{
		this->ReleaseRes_AAC();
		return false;
	}
	*/
	
	//unsigned char *pBuffer = NULL;
	//unsigned long SizeOfDecoderSpecificInfo = 0;
	//faacEncGetDecoderSpecificInfo( m_hencode, &pBuffer, &SizeOfDecoderSpecificInfo );
	//if( pBuffer ) free(pBuffer);

#if defined(defTest_AudioSaveWAV2AAC)
	WAVEFORMATEX wavepak;
	memset( &wavepak, 0, sizeof(wavepak) );

	WAVEFORMATEX *wave = &wavepak;

	wave->wFormatTag = WAVE_FORMAT_PCM;
	wave->nSamplesPerSec = m_pcm_t->samplerate;
	wave->nChannels = m_pcm_t->channels;
	wave->wBitsPerSample = Audio_bitSize;

	wave->nBlockAlign = wave->nChannels * (wave->wBitsPerSample/8);
	wave->nAvgBytesPerSec = wave->nSamplesPerSec * wave->nBlockAlign;
	wave->cbSize=0;


	char out_buffer[1024];
	int out_len = 0;
	char *pout_buffer = out_buffer;

	write_wav_head( wave, NULL, 0, pout_buffer, &out_len );
	if( out_len > 0 )
	{
		fstreamsrc = fopen("d:\\file_wav2aac_src.wav","wb+");
		if( fstreamsrc )
			fwrite( pout_buffer, 1, out_len, fstreamsrc );
	}
#endif

	return true;
}

void Audio_WAV2AAC::CloseAudio()
{
	this->ReleaseRes_AAC();
}

// return out pBuf
unsigned char* Audio_WAV2AAC::OnAudioBufferCB( unsigned char *pBuffer, long BufferLen, long &OutBufferLen )
{
#if defined(defTest_AudioSaveWAV2AAC)
	if( fstreamsrc && BufferLen>0 )
		fwrite( pBuffer, 1, BufferLen, fstreamsrc );
#endif

	if( m_hencode && BufferLen > 0 )
	{
		/*jyc20170323 remove
		m_bufMerge.ResetDataBuf();
		unsigned char *bitbuf = this->new_bitbuf( m_maxOutputBytes );//m_maxOutputBytes

		int last_read_buf_size = 0;

		unsigned char* tmpbuf = pBuffer;
		const unsigned char* tmpbufEnd = pBuffer + BufferLen;
		while( m_hencode && ( tmpbuf < tmpbufEnd ) )
		{
			int read_buf_size = wav_read_int24_buf( m_pcm_t, m_PCM_buf, m_inputSamples, tmpbuf, tmpbufEnd-tmpbuf ) * m_pcm_t->samplebytes;
			int bitoutputsize = faacEncEncode( m_hencode, (int *)m_PCM_buf, read_buf_size/m_pcm_t->samplebytes, bitbuf, m_maxOutputBytes );
			
			if( read_buf_size>0 )
			{
				tmpbuf += read_buf_size;
				last_read_buf_size = read_buf_size;
			}

			if( bitoutputsize > 0 )
			{
				m_bufMerge.PushBuf( bitbuf, bitoutputsize, 0 );
			}
		}
		
		unsigned char *pbitbuf_merge = m_bufMerge.GetDataP();
		uint32_t bitbuf_merge_size = m_bufMerge.GetDataSize();

#if 0
		LOGMSG( "Audio_WAV2AAC::OnAudioBufferCB(), ts=%u, OnBufLen=%d, WavReadSize=%d, EncodeBitOutSize=%d%s\r\n",
			timeGetTime(), BufferLen, last_read_buf_size, bitbuf_merge_size, bitbuf_merge_size>0?"":" *****" );
#endif

		if( bitbuf_merge_size > 0 )
		{
#if defined(defTest_AudioSaveWAV2AAC)
			static FILE *fstream_dest_aac = NULL;
			if( !fstream_dest_aac )
			{
				fstream_dest_aac = fopen("d:\\file_wav2aac_aac.aac","wb+");
			}
			if( fstream_dest_aac )
				fwrite( pbitbuf_merge, 1, bitbuf_merge_size, fstream_dest_aac );
#endif

			//memcpy( pOutBuffer, pbitbuf_merge, bitbuf_merge_size );
		}

		OutBufferLen = bitbuf_merge_size;
		return pbitbuf_merge;
		*/
	}

	return 0;
}
