#pragma once
#include "AudioCap.h"
#include "AudioCapBase.h"

class AudioCap_G711 : public AudioCapBase, public AudioCapCallBack
{
public:
	AudioCap_G711();
	~AudioCap_G711(void);

	enum defType
	{
		alaw,
		ulaw
	};
	
	virtual defAudioCapType_ GetAudioCapType() const
	{
		return defAudioCapType_G711;
	}

	void setG711Type( defAudioFmtType_ Audio_FmtType )
	{
		switch( Audio_FmtType )
		{
		case defAudioFmtType_PCM_ALAW:
			m_G711Type = alaw;
			break;

		case defAudioFmtType_PCM_MULAW:
			m_G711Type = ulaw;
			break;
		}
	}

	virtual bool IsOpen() const
	{
		return m_AudioCap.IsOpen();
	}

	virtual bool OpenAudio( const CAudioCfg &AudioCfg );
	virtual void CloseAudio();

	virtual void OnAudioBufferCB( double SampleTime, unsigned char *pBuffer, long BufferLen );

private:
	void ReleaseRes();
	defType m_G711Type;

	AudioCap m_AudioCap;
};
