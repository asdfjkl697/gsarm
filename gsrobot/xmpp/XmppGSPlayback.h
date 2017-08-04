#pragma once
#include "../typedef.h"
#include "gloox/stanzaextension.h"
#include "gloox/tag.h"

using namespace gloox;

class XmppGSPlayback :
	public StanzaExtension
{
public:
	enum defPBState
	{
		defPBState_Unknown	= 0,
		defPBState_Start	= 1,
		defPBState_Stop		= 2,
		defPBState_Set		= 3,
		defPBState_Get,
		defPBState_Pause,				// ��ͣ����
		defPBState_Resume,				// ��ͣ�ָ�������ͣ�ָ�����ͣǰ״̬����
		defPBState_NormalPlay,			// ���ٶ�
		defPBState_FastPlay,			// ���
		defPBState_FastPlay1,			// ���1��
		defPBState_FastPlay2,			// ���2��
		defPBState_FastPlayThrow,		// ��֡���
		defPBState_SlowPlay,			// ���
		defPBState_SlowPlay1,			// ���1��
		defPBState_SlowPlay2,			// ���2��
		defPBState_StopAll	= 99
	};

public:
	XmppGSPlayback( const struTagParam &TagParam, uint32_t camera_id, const std::string &url, const std::string &peerid, const std::string &streamid, const std::string &key, defPBState state=defPBState_Unknown, uint32_t startdt=0, uint32_t enddt=0, int m_sound=-1 );
	XmppGSPlayback( const Tag* tag );
	~XmppGSPlayback(void);

	static std::string TransState2Str( const defPBState state );

	// reimplemented from StanzaExtension
	virtual const std::string& filterString() const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* newInstance( const Tag* tag ) const
	{
		return new XmppGSPlayback( tag );
	}

	void PrintTag( const Tag* tag, const Stanza *stanza ) const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* clone() const
	{
		XmppGSPlayback *pnew = new XmppGSPlayback(NULL);
		*pnew = *this;
		return pnew;
	}

	// reimplemented from StanzaExt
	virtual Tag* tag() const;

	uint32_t get_camera_id() const
	{
		return m_camera_id;
	}

	uint32_t get_startdt() const
	{
		return m_startdt;
	}

	uint32_t get_enddt() const
	{
		return m_enddt;
	}

	const std::string& get_url() const
	{
		return m_url;
	}

	const std::string& get_peerid() const
	{
		return m_peerid;
	}

	const std::string& get_streamid() const
	{
		return m_streamid;
	}

	const std::vector<std::string>& get_url_backup() const
	{
		return this->m_url_backup;
	}

	const std::string& get_key() const
	{
		return m_key;
	}

	defPBState get_state() const
	{
		return m_state;
	}

	int get_sound() const
	{
		return m_sound;
	}

private:
	struTagParam m_TagParam;

	uint32_t m_camera_id;	// [in/out]
	uint32_t m_startdt;		// [in] ��ʼʱ��  UTC����ʱ�侫ȷ����
	uint32_t m_enddt;		// [in] ����ʱ��   UTC����ʱ�侫ȷ����
	std::string m_url;		// [in/out]��ַ
	std::string m_peerid;	// [out]peerid
	std::string m_streamid;	// [out]streamid
	std::vector<std::string> m_url_backup; // [in]���ݵ�ַ
	std::string m_key;		// [in/out]��Կ������������룬��������
	defPBState m_state;		// [in] ����״̬  start/stop
	int m_sound;			// [in] ��Ƶ���ű�־
};

