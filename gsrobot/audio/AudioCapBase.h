#pragma once
#include "AudioCfg.h"

class AudioCapCallBack
{
public:
	AudioCapCallBack(){};
	virtual ~AudioCapCallBack(void){};
	virtual void OnAudioBufferCB( double SampleTime, unsigned char *pBuffer, long BufferLen ) = 0;
};

class AudioCapBase
{
public:
	AudioCapBase(void);
	~AudioCapBase(void);

	void setAudioCB( AudioCapCallBack *cb )
	{
		m_cb = cb;
	}
	
	virtual defAudioCapType_ GetAudioCapType() const = 0;
	virtual bool IsOpen() const = 0;
	virtual bool OpenAudio( const CAudioCfg &AudioCfg ) = 0;
	virtual void CloseAudio() = 0;

protected:
	AudioCapCallBack *m_cb;
};

