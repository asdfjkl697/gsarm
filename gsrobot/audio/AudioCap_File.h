#pragma once
#include "AudioCap.h"
#include "faac.h"
//#include "faac/input.h"
#include "AudioCapBase.h"

class AudioCap_File
{
public:
	AudioCap_File(void);
	~AudioCap_File(void);
	
	bool LoadFiles( const CAudioCfg &AudioCfg );
	/*
	pcmfile_t *GetPCM()
	{
		return m_pcm_t;
	}*/

	unsigned char * GetFrame( int &size );

	void Start( uint32_t startTime, bool usein );
	void Stop();

private:
	uint32_t m_audioTimeTick;

	double m_avSamplesPerSecByte;
	double m_readTotalAudioByte;
	int m_fileStartPos;

	faacEncHandle m_hencode;
	//pcmfile_t *m_pcm_t;
	int32_t *m_PCM_buf;
	unsigned long m_inputSamples;
	unsigned long m_maxOutputBytes;
	unsigned char *m_bitbuf;
};

