#include "AudioCap_AAC.h"
#include "faaccfg.h"
#include "AudioTestDefine.h"


#if defined(defTest_AudioAAC_EncodeFile)
static char AAC_inputFilename[_MAX_PATH], AAC_outputFilename[_MAX_PATH];
static BOOL AAC_Encoding = FALSE;
static DWORD WINAPI AAC_EncodeFile(LPVOID pParam=NULL)
{
	HWND hWnd = (HWND) pParam;
	pcmfile_t *infile = NULL;

	AAC_Encoding = TRUE;
	strcpy( AAC_inputFilename, "d:\\file_cap_src_in.wav" ); 
	strcpy( AAC_outputFilename, "d:\\file_cap_src_out.aac" ); 

	/* open the input file */
	if ((infile = wav_open_read(AAC_inputFilename, 0)) != NULL)
	{
		/* determine input file parameters */
		unsigned int sampleRate = infile->samplerate;
		unsigned int numChannels = infile->channels;

		unsigned long inputSamples;
		unsigned long maxOutputBytes;

		/* open and setup the encoder */
		faacEncHandle hEncoder = faacEncOpen(sampleRate, numChannels,
			&inputSamples, &maxOutputBytes);

		if (hEncoder)
		{
			HANDLE hOutfile;

			/* set encoder configuration */
			faacEncConfigurationPtr fec = faacEncGetCurrentConfiguration(hEncoder);
			
			fec->mpegVersion = 4;
			fec->outputFormat = 0;
			fec->quantqual = 100;
			fec->aacObjectType = defAACObjectType_Default;

			fec->mpegVersion	=0x00000000;
			fec->aacObjectType	=0x00000000;
			fec->allowMidside	=0x00000001;
			fec->useLfe	=0x00000000;
			fec->useTns	=0x00000001;
			fec->bitRate	=0x00000000;
			fec->bandWidth	=0x00000000;
			fec->quantqual	=0x00000064;
			fec->outputFormat	=0x00000001;
			fec->inputFormat	=0x00000003;

			if (!faacEncSetConfiguration(hEncoder, fec))
			{
				faacEncClose(hEncoder);
				wav_close(infile);

				AAC_Encoding = FALSE;
				return 0;
			}

			/* open the output file */
			hOutfile = CreateFileA(AAC_outputFilename, GENERIC_WRITE, 0, NULL,
				CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

			if (hOutfile != INVALID_HANDLE_VALUE)
			{
				int last_binput=0;
				unsigned int bytesInput = 0;
				DWORD numberOfBytesWritten = 0;
				int *pcmbuf;
				unsigned char *bitbuf;

				pcmbuf = (int*)LocalAlloc(0, inputSamples*sizeof(int));
				bitbuf = (unsigned char*)LocalAlloc(0, maxOutputBytes*sizeof(unsigned char));

				for ( ;; )
				{
					int bytesWritten;

					bytesInput = wav_read_int24(infile, pcmbuf, inputSamples, NULL) * sizeof(int);

					/* call the actual encoding routine */
					bytesWritten = faacEncEncode(hEncoder,
						pcmbuf,
						bytesInput/sizeof(int),
						bitbuf,
						maxOutputBytes);

					/* Stop Pressed */
					if ( !AAC_Encoding )
						break;

					/* all done, bail out */
					if (!bytesInput && !bytesWritten)
						break;

					if (bytesWritten < 0)
					{
						break;
					}

					WriteFile(hOutfile, bitbuf, bytesWritten, &numberOfBytesWritten, NULL);
					if( bytesInput ) last_binput=bytesInput;
				}

				CloseHandle(hOutfile);
				if (pcmbuf) LocalFree(pcmbuf);
				if (bitbuf) LocalFree(bitbuf);
			}

			faacEncClose(hEncoder);
		}

		wav_close(infile);
	}

	AAC_Encoding = FALSE;
	return 0;
}
#endif

AudioCap_AAC::AudioCap_AAC()
{
	m_AudioCap.setAudioCB( this );
	
	m_inputSamples = 0;
	m_maxOutputBytes = 0;
	m_hencode = NULL;
	//m_pcm_t = NULL;  //jyc20170323 remove
	m_PCM_buf = NULL;
	m_aacObjectType = defAACObjectType_Default;
}

AudioCap_AAC::~AudioCap_AAC(void)
{
	this->CloseAudio();
}

void AudioCap_AAC::ReleaseRes_AAC()
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
	/*
	if( temp_hencode )
		faacEncClose(temp_hencode);

	//m_bufMerge.ResetDataBuf();  //jyc20170323 remove
	*/
}

bool AudioCap_AAC::OpenAudio( const CAudioCfg &AudioCfg )
{
#if defined(defTest_AudioAAC_EncodeFile)
	AAC_EncodeFile();
	//return false;
#endif

#if 1
	// 为空时表示自动
#else
	if( AudioCfg.get_Audio_cap_src().empty() )
	{
		LOGMSGEX( defLOGNAME, defLOG_INFO, "AudioCap_AAC::OpenAudio is null!" );
		return false;
	}
#endif

	/*
	if( !m_pcm_t )
	{
		m_pcm_t = new pcmfile_t;//(pcmfile_t*)malloc(sizeof(pcmfile_t));
		memset( m_pcm_t, 0, sizeof(pcmfile_t) );
		
	}
	
	int Audio_Channels = 0;
	int Audio_bitSize = 0;
	int Audio_SampleRate = 0;
	int Audio_ByteRate = 0;
	AudioCfg.get_param_use( Audio_Channels, Audio_bitSize, Audio_SampleRate, Audio_ByteRate );

	m_pcm_t->samplebytes = Audio_bitSize/8;
	m_pcm_t->channels = Audio_Channels;
	m_pcm_t->samplerate = Audio_SampleRate;
	m_pcm_t->bigendian = 0;

	m_hencode = faacEncOpen( m_pcm_t->samplerate, m_pcm_t->channels, &m_inputSamples, &m_maxOutputBytes );
	if( m_hencode==NULL )
	{
		return false;
	}
		
	if( m_AudioCap.OpenAudio( AudioCfg.get_Audio_cap_src(), m_pcm_t->channels, m_pcm_t->samplerate, m_pcm_t->samplebytes*8, Audio_ByteRate ) )// 8192,0
	{
		m_PCM_buf = new int32_t[m_inputSamples*m_pcm_t->samplebytes];//(int32_t *)malloc(m_inputSamples*sizeof(int32_t));
		memset( m_PCM_buf, 0, sizeof(int32_t)*m_inputSamples*m_pcm_t->samplebytes );
		
		faacEncConfigurationPtr fec = faacEncGetCurrentConfiguration( m_hencode );

		fec->mpegVersion = 4;
		fec->outputFormat = 0; // flash player 只支持Raw AAC播放，采用ADTS时aac文件可以播放，flash player不能播放。201403
		fec->quantqual = 100;
		fec->aacObjectType = defAACObjectType_Default;

		
		m_aacObjectType = fec->aacObjectType;

		if( !faacEncSetConfiguration( m_hencode, fec ) )
		{
			//LOGMSGEX( defLOGNAME, defLOG_INFO, "faacEncSetConfiguration failed!" );

			this->ReleaseRes_AAC();
			m_AudioCap.CloseAudio();
			return false;
		}
	}
	else
	{
		this->ReleaseRes_AAC();
		return false;
	}
		
	return m_AudioCap.StartCap();
	*/
}

void AudioCap_AAC::CloseAudio()
{
	m_AudioCap.CloseAudio();
	this->ReleaseRes_AAC();
}

void AudioCap_AAC::OnAudioBufferCB( double SampleTime, unsigned char *pBuffer, long BufferLen )
{
	/* //jyc20170323 remove
	if( this->m_cb && BufferLen > 0 && m_hencode )
	{
		unsigned char *bitbuf = m_AudioCap.new_bitbuf( m_maxOutputBytes );//m_maxOutputBytes

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
				//m_bufMerge.PushBuf( bitbuf, bitoutputsize, 0 );
			}
		}
		
		//unsigned char *pbitbuf_merge = m_bufMerge.GetDataP();
		uint32_t bitbuf_merge_size = m_bufMerge.GetDataSize();


		if( bitbuf_merge_size > 0 )
		{
#if defined(defTest_AudioSaveCapAAC)
			static FILE *fstream_dest_aac = NULL;
			if( !fstream_dest_aac )
			{
				fstream_dest_aac = fopen("d:\\file_cap_dest_aac.aac","wb+");
			}
			if( fstream_dest_aac )
				fwrite( pbitbuf_merge, 1, bitbuf_merge_size, fstream_dest_aac );
#endif
			//LOGMSG( "AudioCap_AAC::On ts=%d, ensize=%d", timeGetTime(), bitbuf_merge_size );
			this->m_cb->OnAudioBufferCB( SampleTime, pbitbuf_merge, bitbuf_merge_size );
		}

		m_bufMerge.ResetDataBuf();
	}*/
}
