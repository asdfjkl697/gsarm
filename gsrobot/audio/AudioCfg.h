#pragma once
#include <string>

#define defAACObjectType_Default LOW

// ��ƵԴ
enum defAudioSource_
{
	 defAudioSource_Null		= 0, // 无音频
	 defAudioSource_Camera		= 1, // 摄像头音频
	 defAudioSource_LocalCap	= 2, // 本地音频
	 defAudioSource_File		= 3, // Read File
	 defAudioSource_CamReEnc	= 4, // 摄像头音频转码
};

// ��Ƶ��ʽ, 0~15��flv��Ӧ
enum defAudioFmtType_
{
	defAudioFmtType_Unknown				 = -1,
	defAudioFmtType_PCM                  = 0,
	defAudioFmtType_ADPCM                = 1,
	defAudioFmtType_MP3                  = 2,
	defAudioFmtType_PCM_LE               = 3,
	defAudioFmtType_NELLYMOSER_16KHZ_MONO = 4,
	defAudioFmtType_NELLYMOSER_8KHZ_MONO = 5,
	defAudioFmtType_NELLYMOSER           = 6,
	defAudioFmtType_PCM_ALAW             = 7,
	defAudioFmtType_PCM_MULAW            = 8,
	defAudioFmtType_AAC                  = 10,
	defAudioFmtType_SPEEX                = 11,
	defAudioFmtType_MP3_8KHZ             = 14,
	defAudioFmtType_Other				 = 15,
};

enum defAudioParamDef_
{
	defAudioParamDef_SysDefault		= 0, // ����ϵͳĬ��
	defAudioParamDef_UserDefine		= 1, // �û��Զ���
	defAudioParamDef_Analyse		= 2, // ������ƵԴ
};

enum defAudioCapType_
{
	defAudioCapType_Unknown		= 0,	//
	defAudioCapType_G711,				// alaw or ulaw
	defAudioCapType_AAC,				// AAC
};

class CAudioCfg
{
public:
	CAudioCfg(void);
	~CAudioCfg(void);

	struct struAudioParam
	{
		defAudioFmtType_ Audio_FmtType;
		int Audio_Channels;
		int Audio_bitSize;
		int Audio_SampleRate;
		int Audio_ByteRate;

		//struAudioParam()
		//{
		//	Audio_FmtType = defAudioFmtType_PCM;
		//	Audio_Channels = 0;
		//	Audio_bitSize = 0;
		//	Audio_SampleRate = 0;
		//	Audio_ByteRate = 0;
		//}

		//struAudioParam( defAudioFmtType_ in_Audio_FmtType, int in_Audio_Channels, int in_Audio_bitSize, int in_Audio_SampleRate, int in_Audio_ByteRate )
		//	: Audio_FmtType(in_Audio_FmtType), Audio_Channels(in_Audio_Channels), Audio_bitSize(in_Audio_bitSize), Audio_SampleRate(in_Audio_SampleRate), Audio_ByteRate(in_Audio_ByteRate)
		//{
		//}
	};

	static defAudioCapType_ trans_FmtType2AudioCapType( defAudioFmtType_ Audio_FmtType );

	// ��ȡĬ�ϲ���
	static void get_defaultForFmtType( const defAudioFmtType_ Audio_FmtType, 
		int &Audio_Channels, 
		int &Audio_bitSize, 
		int &Audio_SampleRate, 
		int &Audio_ByteRate );

	// ����Kbpsת�ֽ���
	static int BitRate2Bytes( const int Audio_BitRate );
	static const std::string getstr_AudioSource( const defAudioSource_ Audio_Source, bool lngcn=true );
	static const std::string getstr_AudioFmtType( const defAudioFmtType_ Audio_FmtType );
	static const std::string getstr_AudioParamDef( const defAudioParamDef_ Audio_ParamDef, bool lngcn=true );
	static const std::string getstr_AudioChannels( const int Audio_Channels );

	static const std::string getstr_struAudioParam( const CAudioCfg::struAudioParam &ap );
	static const CAudioCfg::struAudioParam& getExample( unsigned int index );
	static const unsigned int getExampleCount();

	unsigned char get_FlvAudioHead() const;
	void get_AAC_AudioSpecificConfig( unsigned char &aac_cfg1, unsigned char &aac_cfg2, unsigned int aacObjectType ) const;

	void set_AudioParam_Analyse( const CAudioCfg::struAudioParam &ap_Analyse );

	// ��ʹ�ò���ʱ�������ô˺����ȡ����ʱ����
	void get_param_use(
		int &Audio_Channels, 
		int &Audio_bitSize, 
		int &Audio_SampleRate, 
		int &Audio_ByteRate ) const;

	bool isChanged( const CAudioCfg &AudioCfg ) const;

	//
	defAudioSource_ get_Audio_Source() const
	{
		return m_Audio_Source;
	}
	void set_Audio_Source( defAudioSource_ Audio_Source )
	{
		m_Audio_Source = Audio_Source;
	}

	//
	defAudioFmtType_ get_Audio_FmtType() const
	{
		return m_Audio_FmtType;
	}
	void set_Audio_FmtType( defAudioFmtType_ Audio_FmtType )
	{
		m_Audio_FmtType = Audio_FmtType;
	}

	//
	defAudioParamDef_ get_Audio_ParamDef() const
	{
		return m_Audio_ParamDef;
	}
	void set_Audio_ParamDef( defAudioParamDef_ Audio_ParamDef )
	{
		m_Audio_ParamDef = Audio_ParamDef;
	}

	//
	int get_Audio_Channels() const
	{
		return m_Audio_Channels;
	}
	void set_Audio_Channels( int Audio_Channels )
	{
		m_Audio_Channels = Audio_Channels;
	}

	//
	int get_Audio_bitSize() const
	{
		return m_Audio_bitSize;
	}
	void set_Audio_bitSize( int Audio_bitSize )
	{
		m_Audio_bitSize = Audio_bitSize;
	}

	//
	int get_Audio_SampleRate() const
	{
		return m_Audio_SampleRate;
	}
	void set_Audio_SampleRate( int Audio_SampleRate )
	{
		m_Audio_SampleRate = Audio_SampleRate;
	}

	//
	int get_Audio_ByteRate() const
	{
		return m_Audio_ByteRate;
	}
	void set_Audio_ByteRate( int Audio_ByteRate )
	{
		m_Audio_ByteRate = Audio_ByteRate;
	}

	//
	std::string get_Audio_cap_src() const
	{
		return m_Audio_cap_src;
	}
	void set_Audio_cap_src( std::string &Audio_cap_src )
	{
		m_Audio_cap_src = Audio_cap_src;
	}

	//
	std::string get_Audio_cap_param() const
	{
		return m_Audio_cap_param;
	}
	void set_Audio_cap_param( std::string &Audio_cap_param )
	{
		m_Audio_cap_param = Audio_cap_param;
	}


private:
	defAudioSource_ m_Audio_Source;		// ��ƵԴ
	defAudioFmtType_ m_Audio_FmtType;	// ��Ƶ��ʽ
	defAudioParamDef_ m_Audio_ParamDef;	// ������
	int m_Audio_Channels;	// ��Ƶ��� 1 or 2
	int m_Audio_bitSize;	// bit ��Ƶ����λ 8 or 16
	int m_Audio_SampleRate;	// ��Ƶ����Ƶ��, 5512Hz, 8000Hz, 11025HZ, 22050HZ, 44100HZ �ȵ�
	int m_Audio_ByteRate;	// �ֽ�����

	//int m_Audio_Info1;		// Ԥ�� ��Ƶ��Ϣ1
	//int m_Audio_Info2;		// Ԥ�� ��Ƶ��Ϣ2
	std::string m_Audio_cap_src;	// ��Ƶ���زɼ�Դ����Ƶ�����豸���
	std::string m_Audio_cap_param;	// ��Ƶ���زɼ��������

	struAudioParam m_AudioParam_Analyse;
};

