#include "AudioCap_G711.h"
#include "AudioTestDefine.h"


#if defined(defTest_AudioSaveCapG711)
static FILE *fstream_A_G711_src = NULL;
#endif

//#include "g711.cpp"

AudioCap_G711::AudioCap_G711()
	: m_G711Type(alaw)
{
	m_AudioCap.setAudioCB( this );
}

AudioCap_G711::~AudioCap_G711(void)
{
	this->CloseAudio();
}

void AudioCap_G711::ReleaseRes()
{
}

bool AudioCap_G711::OpenAudio( const CAudioCfg &AudioCfg )
{
	if( AudioCfg.get_Audio_cap_src().empty() )
	{
		//LOGMSGEX( defLOGNAME, defLOG_INFO, "AudioCap_G711::OpenAudio is null!" );
		return false;
	}

	int Audio_Channels = 0;
	int Audio_bitSize = 0;
	int Audio_SampleRate = 0;
	int Audio_ByteRate = 0;
	AudioCfg.get_param_use( Audio_Channels, Audio_bitSize, Audio_SampleRate, Audio_ByteRate );

#if defined(defTest_AudioSaveCapG711)
	if( !fstream_A_G711_src )
	{
		char out_buffer[1024];
		int out_len = 0;
		char *pout_buffer = out_buffer;

		WAVEFORMATEX wavepak;
		memset( &wavepak, 0, sizeof(wavepak) );

		WAVEFORMATEX *wave = &wavepak;

		switch( m_G711Type )
		{
		case alaw:
			wave->wFormatTag = 6;
			break;

		case ulaw:
			wave->wFormatTag = 7;
			break;

		default:
			wave->wFormatTag = WAVE_FORMAT_PCM;			// 6=WAVE_FORMAT_ALAW, 7=WAVE_FORMAT_MULAW, WAVE_FORMAT_PCM;WAVE_FORMAT_4M16
			break;
		}

		wave->nSamplesPerSec = Audio_SampleRate;//8000;//Audio_SampleRate;	// KHZ
		wave->nChannels = Audio_Channels;
		wave->wBitsPerSample = 8;//Audio_bitSize;		// Bit

		wave->nBlockAlign = wave->nChannels * (wave->wBitsPerSample/8);
		wave->nAvgBytesPerSec = wave->nSamplesPerSec * wave->nBlockAlign;
		wave->cbSize=0;

		write_wav_head( wave, NULL, 0, pout_buffer, &out_len );
		if( out_len > 0 )
		{
			fstream_A_G711_src = fopen("d:\\file_cap_G711.wav","wb+");

			if( fstream_A_G711_src )
				fwrite( pout_buffer, 1, out_len, fstream_A_G711_src );
		}
	}
#endif

	if( !m_AudioCap.OpenAudio( AudioCfg.get_Audio_cap_src(), Audio_Channels, Audio_SampleRate, Audio_bitSize, Audio_ByteRate ) )
	{
		this->ReleaseRes();
		return false;
	}

	//return m_AudioCap.StartCap();
}

void AudioCap_G711::CloseAudio()
{
	m_AudioCap.CloseAudio();
	this->ReleaseRes();
}

void AudioCap_G711::OnAudioBufferCB( double SampleTime, unsigned char *pBuffer, long BufferLen )
{
	if( this->m_cb && BufferLen > 0 )
	{
		unsigned char *bitbuf = m_AudioCap.new_bitbuf( BufferLen );

		int enclen = 0;
		switch( m_G711Type )
		{
		case alaw:
			//enclen = defNS_G711::EncodedData_alaw( bitbuf, pBuffer, BufferLen );
			break;

		case ulaw:
			//enclen = defNS_G711::EncodedData_ulaw( bitbuf, pBuffer, BufferLen );
			break;

		default:
			this->m_cb->OnAudioBufferCB( SampleTime, pBuffer, BufferLen );
			return;
		}

#if defined(defTest_AudioSaveCapG711)
		if( fstream_A_G711_src )
			fwrite( bitbuf, 1, enclen, fstream_A_G711_src );
#endif

		if( enclen > 0 )
			this->m_cb->OnAudioBufferCB( SampleTime, bitbuf, enclen );
	}
}
