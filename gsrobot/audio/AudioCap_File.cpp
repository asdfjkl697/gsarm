#include "AudioCap_File.h"


AudioCap_File::AudioCap_File(void)
{
	m_inputSamples = 0;
	m_maxOutputBytes = 0;
	m_hencode = NULL;
	//m_pcm_t = NULL;
	m_PCM_buf = NULL;
	m_bitbuf = NULL;
}

AudioCap_File::~AudioCap_File(void)
{
	Stop();
}

bool AudioCap_File::LoadFiles( const CAudioCfg &AudioCfg )
{
	Stop();
	if( AudioCfg.get_Audio_cap_src().empty() )
	{
		//LOGMSGEX( defLOGNAME, defLOG_INFO, "AudioCap_File::LoadFiles is null!" );
		return false;
	}

	m_readTotalAudioByte = 0;
	/* jyc20170323 remove
	if( (m_pcm_t = wav_open_read(AudioCfg.get_Audio_cap_src().c_str(),0) ) != NULL ) //raw input false
	{
		m_fileStartPos = ftell(m_pcm_t->f);
		m_avSamplesPerSecByte = m_pcm_t->samplebytes * m_pcm_t->channels * m_pcm_t->samplerate;

		m_hencode = faacEncOpen( m_pcm_t->samplerate, m_pcm_t->channels, &m_inputSamples, &m_maxOutputBytes );

		if( m_hencode != NULL )
		{
			m_PCM_buf = new int32_t[m_inputSamples*m_pcm_t->samplebytes];//m_PCM_buf = (int32_t *)malloc(m_inputSamples*sizeof(int32_t));
			m_bitbuf = new unsigned char[m_maxOutputBytes];//m_bitbuf = (unsigned char*)malloc(m_maxOutputBytes*sizeof(unsigned char));

			faacEncConfigurationPtr fec = faacEncGetCurrentConfiguration(m_hencode);

			fec->mpegVersion = 4;
			fec->outputFormat = 0;
			fec->quantqual = 100;
			fec->aacObjectType= defAACObjectType_Default;

			return faacEncSetConfiguration(m_hencode,fec)>0;
		}

		//LOGMSGEX( defLOGNAME, defLOG_INFO, "AudioCap_File::faacEncOpen failed! \"%s\"", AudioCfg.get_Audio_cap_src().c_str() );
		return false;
	}*/

	//LOGMSGEX( defLOGNAME, defLOG_INFO, "AudioCap_File::LoadFiles failed! \"%s\"", AudioCfg.get_Audio_cap_src().c_str() );
	return false;
}

void AudioCap_File::Start( uint32_t startTime, bool usein )
{
	if( usein )
	{
		m_audioTimeTick = startTime;
	}
	else
	{
		m_audioTimeTick = timeGetTime();
	}
}

unsigned char * AudioCap_File::GetFrame( int &size )
{
	if( !m_hencode )
		return NULL;

	size = 0;
	uint32_t nTimeStamp = ( (m_readTotalAudioByte + m_inputSamples) / m_avSamplesPerSecByte ) * 1000;
	if( timeGetTime()-m_audioTimeTick > nTimeStamp )
	{
		/* jyc20170323 remove
		unsigned long readSamples = wav_read_int24( m_pcm_t, m_PCM_buf, m_inputSamples, NULL );
		if( readSamples < m_inputSamples )
		{
			fseek( m_pcm_t->f, this->m_fileStartPos, SEEK_SET ); // 文件复位
			m_fileStartPos = ftell( m_pcm_t->f );
			readSamples = wav_read_int24( m_pcm_t, m_PCM_buf, m_inputSamples, NULL );
		}

		const int read_buf_size = readSamples * m_pcm_t->samplebytes;
		int bitoutputsize = faacEncEncode( m_hencode, m_PCM_buf, readSamples, m_bitbuf, m_maxOutputBytes );

		m_readTotalAudioByte += read_buf_size;

		if( !read_buf_size )
		{
			fseek( m_pcm_t->f, this->m_fileStartPos, SEEK_SET ); //文件复位

			m_fileStartPos = ftell(m_pcm_t->f);
		}

		if( bitoutputsize>0 )
		{
			size = bitoutputsize;
			return m_bitbuf;
		}*/
	}

	return NULL;
}

void AudioCap_File::Stop()
{
	faacEncHandle temp_hencode = NULL;
	if( m_hencode )
	{
		temp_hencode = m_hencode;
		m_hencode = NULL;
	}

	if( m_PCM_buf )
	{
		delete []m_PCM_buf;
		m_PCM_buf = NULL;
	}

	if( m_bitbuf )
	{
		delete []m_bitbuf;
		m_bitbuf = NULL;
	}

	m_inputSamples = 0;
	m_maxOutputBytes = 0;

	/* jyc20170323 remove
	if( m_pcm_t )
	{
		wav_close(m_pcm_t);
		m_pcm_t = NULL;
	}

	if( temp_hencode )
		faacEncClose(temp_hencode);
	*/
}
