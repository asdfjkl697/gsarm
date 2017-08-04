#pragma once
#include "AudioCap.h"
#include "faac.h"
//#include "faac/input.h"
#include "AudioCapBase.h"
//#include "BufferMerge.h"

#pragma comment(lib,"libfaac.lib")

class AudioCap_AAC : public AudioCapBase, public AudioCapCallBack
{
public:
	AudioCap_AAC();
	~AudioCap_AAC(void);
	
	virtual defAudioCapType_ GetAudioCapType() const
	{
		return defAudioCapType_AAC;
	}

	virtual bool IsOpen() const
	{
		return ( m_hencode!=NULL );
	}

	virtual bool OpenAudio( const CAudioCfg &AudioCfg );
	virtual void CloseAudio();

	virtual void OnAudioBufferCB( double SampleTime, unsigned char *pBuffer, long BufferLen );

private:
	void ReleaseRes_AAC();
	
	AudioCap m_AudioCap;
	
	faacEncHandle m_hencode;
	//pcmfile_t *m_pcm_t;  //jyc20170322 remove
	int32_t *m_PCM_buf;
	unsigned long m_inputSamples;
	unsigned long m_maxOutputBytes;
	unsigned int m_aacObjectType;

	//CBufferMerge m_bufMerge;
};
