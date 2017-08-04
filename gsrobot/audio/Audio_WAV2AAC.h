#pragma once
#include "../common.h"
#include "faac.h"
//#include "faac/input.h"
#include "AudioCfg.h"
//#include "BufferMerge.h"  //jyc20170322 remove

class Audio_WAV2AAC
{
public:
	Audio_WAV2AAC();
	~Audio_WAV2AAC(void);
	
	virtual bool IsOpen() const
	{
		return ( m_hencode!=NULL );
	}

	unsigned char *new_bitbuf( unsigned int needsize )
	{
		if( m_bitbuf && needsize > m_bitbufSize )
		{
			delete []m_bitbuf;
			m_bitbuf = NULL;
			m_bitbufSize = 0;
		}

		if( !m_bitbuf )
		{
			m_bitbufSize = needsize;
			m_bitbuf = new unsigned char[m_bitbufSize];
			memset( m_bitbuf, 0, m_bitbufSize );
		}

		return m_bitbuf;
	}

	bool OpenAudio( const CAudioCfg &AudioCfg );
	void CloseAudio();

	unsigned char* OnAudioBufferCB( unsigned char *pBuffer, long BufferLen, long &OutBufferLen );

private:
	void ReleaseRes_AAC();
		
	faacEncHandle m_hencode;
	//pcmfile_t *m_pcm_t; //jyc20170322 remove
	int32_t *m_PCM_buf;
	unsigned long m_inputSamples;
	unsigned long m_maxOutputBytes;
	unsigned int m_aacObjectType;

	unsigned char *m_bitbuf;
	unsigned int m_bitbufSize;

	//CBufferMerge m_bufMerge;
};
