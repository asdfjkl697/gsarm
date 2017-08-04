#include "AudioCfg.h"
#include "../include/flv.h"
#include "AudioTestDefine.h"
#include "../RunCode.h"
#include "gloox/util.h"
//#include "logFileExFunc.h"

using namespace gloox;

#define defByteRate_base_k	1024

static const CAudioCfg::struAudioParam s_AudioExample[] =
{
	{ (defAudioFmtType_)0,			0,	0,	0,		0 },

	{ defAudioFmtType_PCM_ALAW,		1,	16,	8000,	2*defByteRate_base_k },
	{ defAudioFmtType_PCM_MULAW,	1,	16,	8000,	2*defByteRate_base_k },

	{ defAudioFmtType_AAC,			1,	16,	22050,	4*defByteRate_base_k },
	{ defAudioFmtType_AAC,			1,	16,	44100,	4*defByteRate_base_k },
	{ defAudioFmtType_AAC,			1,	16,	11025,	4*defByteRate_base_k },
	{ defAudioFmtType_AAC,			1,	16,	8000,	4*defByteRate_base_k },
	{ defAudioFmtType_AAC,			2,	16,	44100,	8*defByteRate_base_k },
	{ defAudioFmtType_AAC,			2,	16,	22050,	8*defByteRate_base_k },
};
static const unsigned int s_AudioExampleCount = sizeof(s_AudioExample)/sizeof(CAudioCfg::struAudioParam);

CAudioCfg::CAudioCfg(void)
{
	m_Audio_Source = defAudioSource_Null;
	m_Audio_FmtType = defAudioFmtType_PCM_MULAW;
	m_Audio_ParamDef = defAudioParamDef_SysDefault;
	get_defaultForFmtType( m_Audio_FmtType, m_Audio_Channels, m_Audio_bitSize, m_Audio_SampleRate, m_Audio_ByteRate );
	//jyc20170323 remove
	//memset( &m_AudioParam_Analyse, 0, sizeof(m_AudioParam_Analyse) );

#if defined(defTest_AudioCap_)
	m_Audio_Source = defAudioSource_LocalCap;//defAudioSource_Camera, defAudioSource_LocalCap, 
	m_Audio_FmtType = defAudioFmtType_AAC;
	get_defaultForFmtType( m_Audio_FmtType, m_Audio_Channels, m_Audio_bitSize, m_Audio_SampleRate, m_Audio_ByteRate );
#endif
}

CAudioCfg::~CAudioCfg(void)
{
}

defAudioCapType_ CAudioCfg::trans_FmtType2AudioCapType( defAudioFmtType_ Audio_FmtType )
{
	switch(Audio_FmtType)
	{
	case defAudioFmtType_PCM_ALAW:
	case defAudioFmtType_PCM_MULAW:
		{
			return defAudioCapType_G711;
		}
		break;

	case defAudioFmtType_AAC:
		{
			return defAudioCapType_AAC;
		}
		break;

	default:
		break;
	}

	return defAudioCapType_Unknown;
}

void CAudioCfg::get_defaultForFmtType( const defAudioFmtType_ Audio_FmtType, 
	int &Audio_Channels, 
	int &Audio_bitSize, 
	int &Audio_SampleRate, 
	int &Audio_ByteRate )
{
	switch(Audio_FmtType)
	{
	case defAudioFmtType_PCM_ALAW:
	case defAudioFmtType_PCM_MULAW:
		{
			Audio_Channels = 1;
			Audio_bitSize = 16;
			Audio_SampleRate = 8000;
			Audio_ByteRate = 2 * defByteRate_base_k;
		}
		break;

	case defAudioFmtType_AAC:
		{
			Audio_Channels = 1;
			Audio_bitSize = 16;
			Audio_SampleRate = 44100;//22050;//44100  //jyc20170526 modify
			Audio_ByteRate = 2 * defByteRate_base_k; //jyc20170526 modify 4 * -->  2 *
		}
		break;

	default:
		{
			Audio_Channels = 1;
			Audio_bitSize = 16;
			Audio_SampleRate = 8000;
			Audio_ByteRate = 2 * defByteRate_base_k;
		}
		break;
	}
}

int CAudioCfg::BitRate2Bytes( const int Audio_BitRate )
{
	int K_base = 1024;

	if( IsRUNCODEEnable(defCodeIndex_SYS_BitRateBaseK) )
	{
		K_base = RUNCODE_Get(defCodeIndex_SYS_BitRateBaseK,defRunCodeValIndex_2);
		if( K_base<0 )
			K_base = 1;
	}

	return ( (Audio_BitRate * K_base) / 8 ); //(1024/8)
}

const std::string CAudioCfg::getstr_AudioSource( const defAudioSource_ Audio_Source, bool lngcn )
{
	switch( Audio_Source )
	{
	case defAudioSource_Null:
		if( lngcn )
			return std::string("����Ƶ");
		else
			return std::string("Null");

	case defAudioSource_Camera:
		if( lngcn )
			return std::string("����ͷ��Ƶ");
		else
			return std::string("Camera");

	case defAudioSource_LocalCap:
		if( lngcn )
			return std::string("���زɼ���Ƶ");
		else
			return std::string("LocalCap");

	case defAudioSource_File:
		if( lngcn )
			return std::string("��Ƶ�ļ�");
		else
			return std::string("File");

	case defAudioSource_CamReEnc:
		if( lngcn )
			return std::string("����ͷ��Ƶת��");
		else
			return std::string("CamReEnc");
	}

	return std::string("");
}

const std::string CAudioCfg::getstr_AudioFmtType( const defAudioFmtType_ Audio_FmtType )
{
	switch( Audio_FmtType )
	{
	case defAudioFmtType_PCM:
		return std::string("PCM");

	case defAudioFmtType_ADPCM:
		return std::string("ADPCM");

	case defAudioFmtType_MP3:
		return std::string("MP3");

	case defAudioFmtType_PCM_LE:
		return std::string("PCM_LE");

	case defAudioFmtType_PCM_ALAW:
		return std::string("alaw");

	case defAudioFmtType_PCM_MULAW:
		return std::string("ulaw");

	case defAudioFmtType_AAC:
		return std::string("AAC");

	case defAudioFmtType_SPEEX:
		return std::string("SPEEX");

	case defAudioFmtType_MP3_8KHZ:
		return std::string("MP3_8KHZ");

	case defAudioFmtType_Other:
		return std::string("Other");
	}

	return std::string("");
}

const std::string CAudioCfg::getstr_AudioParamDef( const defAudioParamDef_ Audio_ParamDef, bool lngcn )
{
	switch( Audio_ParamDef )
	{
	case defAudioParamDef_SysDefault:
		if( lngcn )
			return std::string("����ϵͳĬ��");
		else
			return std::string("SysDefault");

	case defAudioParamDef_UserDefine:
		if( lngcn )
			return std::string("�Զ���");
		else
			return std::string("UserDefine");

	case defAudioParamDef_Analyse:
		if( lngcn )
			return std::string("������ƵԴ");
		else
			return std::string("Analyse");
	}

	return std::string("");
}

const std::string CAudioCfg::getstr_AudioChannels( const int Audio_Channels )
{
	switch( Audio_Channels )
	{
	case 1:
		return std::string("�����");

	case 2:
		return std::string("˫���");
	}

	return util::int2string( Audio_Channels );
}

const std::string CAudioCfg::getstr_struAudioParam( const CAudioCfg::struAudioParam &ap )
{
	std::string des = getstr_AudioFmtType( ap.Audio_FmtType ) + ", " +
		getstr_AudioChannels( ap.Audio_Channels ) + ", " +
		util::int2string( ap.Audio_bitSize ) + "bit, " +
		util::int2string( ap.Audio_SampleRate ) + "Hz, " +
		util::int2string( ap.Audio_ByteRate ) + "Byte";

	return des;
}

const CAudioCfg::struAudioParam& CAudioCfg::getExample( const unsigned int index )
{
	if( index < getExampleCount() )
	{
		return s_AudioExample[index];
	}

	return s_AudioExample[0];
}

const unsigned int CAudioCfg::getExampleCount()
{
	return s_AudioExampleCount;
}

unsigned char CAudioCfg::get_FlvAudioHead() const
{
	if( defAudioSource_Null == this->m_Audio_Source )
		return 0;

	unsigned char AudioHead = 0;

	int Audio_Channels = 0;
	int Audio_bitSize = 0;
	int Audio_SampleRate = 0;
	int Audio_ByteRate = 0;
	get_param_use( Audio_Channels, Audio_bitSize, Audio_SampleRate, Audio_ByteRate );
	/*
	LOGMSG( "get_FlvAudioHead audio param: src=%d(%s), type=%d(%s), paramdef=%d(%s), ch=%d, bitsize=%d, sample=%d, Byte=%d\r\n",
		get_Audio_Source(), getstr_AudioSource(get_Audio_Source(),false).c_str(), 
		get_Audio_FmtType(), getstr_AudioFmtType(get_Audio_FmtType()).c_str(), 
		get_Audio_ParamDef(), getstr_AudioParamDef(get_Audio_ParamDef(),false).c_str(), 
		Audio_Channels, Audio_bitSize, Audio_SampleRate, Audio_ByteRate );
	*/
	defAudioFmtType_ AudioFmtType = get_Audio_FmtType();
	if( AudioFmtType >= 0 && AudioFmtType <= 15 )
	{
		AudioHead = AudioFmtType << FLV_AUDIO_CODECID_OFFSET; //jyc20170323 remove
	}
	else
	{
		AudioHead = defAudioFmtType_Other << FLV_AUDIO_CODECID_OFFSET; //jyc20170323 remove
	}

	if( 16 == Audio_bitSize )
	{
		AudioHead |= FLV_SAMPLESSIZE_16BIT;
	}
	else
	{
		AudioHead |= FLV_SAMPLESSIZE_8BIT;
	}

	if( 2 == Audio_Channels )
	{
		AudioHead |= FLV_STEREO;
	}
	else
	{
		AudioHead |= FLV_MONO;
	}

	switch( Audio_SampleRate )
	{
	case 44100:
		{
			AudioHead |= FLV_SAMPLERATE_44100HZ;
			break;
		}
	case 22050:
		{
			AudioHead |= FLV_SAMPLERATE_22050HZ;
			break;
		}
	case 11025:
		{
			AudioHead |= FLV_SAMPLERATE_11025HZ;
			break;
		}
	default:
		{
			AudioHead |= FLV_SAMPLERATE_SPECIAL;
			break;
		}
	}

	return AudioHead;
}

void CAudioCfg::get_AAC_AudioSpecificConfig( unsigned char &aac_cfg1, unsigned char &aac_cfg2, unsigned int aacObjectType ) const
{
	int Audio_Channels = 0;
	int Audio_bitSize = 0;
	int Audio_SampleRate = 0;
	int Audio_ByteRate = 0;
	this->get_param_use( Audio_Channels, Audio_bitSize, Audio_SampleRate, Audio_ByteRate );

	int samplingFrequencyIndex = 4;
	switch( Audio_SampleRate )
	{
	case 96000: samplingFrequencyIndex = 0; break;
	case 88200: samplingFrequencyIndex = 1; break;
	case 64000: samplingFrequencyIndex = 2; break;
	case 48000: samplingFrequencyIndex = 3; break;
	case 44100: samplingFrequencyIndex = 4; break;
	case 32000: samplingFrequencyIndex = 5; break;
	case 24000: samplingFrequencyIndex = 6; break;
	case 22050: samplingFrequencyIndex = 7; break;
	case 16000: samplingFrequencyIndex = 8; break;
	case 12000: samplingFrequencyIndex = 9; break;
	case 11025: samplingFrequencyIndex = 10; break;
	case 8000: samplingFrequencyIndex = 11; break;
	}

	aac_cfg1 = ( aacObjectType<<3 ) | ( (samplingFrequencyIndex&0xe)>>1 );
	aac_cfg2 = ( (samplingFrequencyIndex&0x1)<<7 ) | ( Audio_Channels<<3 );
}

void CAudioCfg::set_AudioParam_Analyse( const CAudioCfg::struAudioParam &ap_Analyse )
{
	m_AudioParam_Analyse = ap_Analyse;
}

void CAudioCfg::get_param_use(
	int &Audio_Channels, 
	int &Audio_bitSize, 
	int &Audio_SampleRate, 
	int &Audio_ByteRate ) const
{
	get_defaultForFmtType( m_Audio_FmtType, Audio_Channels, Audio_bitSize, Audio_SampleRate, Audio_ByteRate );

	if( defAudioParamDef_SysDefault == this->m_Audio_ParamDef )
	{
	}
	else if( defAudioParamDef_UserDefine == this->m_Audio_ParamDef )
	{
		if( m_Audio_Channels ) Audio_Channels = m_Audio_Channels;
		if( m_Audio_bitSize ) Audio_bitSize = m_Audio_bitSize;
		if( m_Audio_SampleRate ) Audio_SampleRate = m_Audio_SampleRate;
		if( m_Audio_ByteRate ) Audio_ByteRate = m_Audio_ByteRate;
	}
	else if( defAudioParamDef_Analyse == this->m_Audio_ParamDef )
	{
		if( m_AudioParam_Analyse.Audio_Channels
			&& m_AudioParam_Analyse.Audio_bitSize
			&& m_AudioParam_Analyse.Audio_SampleRate
			)
		{
			Audio_Channels = m_AudioParam_Analyse.Audio_Channels;
			Audio_bitSize = m_AudioParam_Analyse.Audio_bitSize;
			Audio_SampleRate = m_AudioParam_Analyse.Audio_SampleRate;
			Audio_ByteRate = m_AudioParam_Analyse.Audio_ByteRate;
		}
	}
}

bool CAudioCfg::isChanged( const CAudioCfg &AudioCfg ) const
{
	if( this->m_Audio_Source != AudioCfg.m_Audio_Source )
		return true;

	if( defAudioSource_Null == this->m_Audio_Source ) // ����nullû�з���仯
		return false;

	if( this->m_Audio_FmtType != AudioCfg.m_Audio_FmtType )
		return true;

	if( this->m_Audio_ParamDef != AudioCfg.m_Audio_ParamDef )
		return true;

	if( defAudioParamDef_SysDefault != this->m_Audio_ParamDef )
	{
		if( this->m_Audio_Channels != AudioCfg.m_Audio_Channels )
			return true;

		if( this->m_Audio_bitSize != AudioCfg.m_Audio_bitSize )
			return true;

		if( this->m_Audio_SampleRate != AudioCfg.m_Audio_SampleRate )
			return true;

		if( this->m_Audio_ByteRate != AudioCfg.m_Audio_ByteRate )
			return true;
	}

	if( this->m_Audio_cap_src != AudioCfg.m_Audio_cap_src )
		return true;

	if( this->m_Audio_cap_param != AudioCfg.m_Audio_cap_param )
		return true;

	return false;
}
