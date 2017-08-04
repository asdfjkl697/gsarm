#pragma once
#include "../typedef.h"
#include "gloox/stanzaextension.h"
#include "gloox/tag.h"
#include "../gsiot/GSIOTDevice.h"

using namespace gloox;

class XmppGSTalk :
	public StanzaExtension
{
public:
	enum defTalkCmd
	{
		defTalkCmd_Unknown	= 0,
		defTalkCmd_request	= 1,	// ���� in, ��url
		defTalkCmd_accept,			// ���� out, ��url
		defTalkCmd_reject,			// �ܾ� out, ��url
		defTalkCmd_session,			// �Ự in/out, ��url
		defTalkCmd_adddev,			// ���ӶԽ��豸 in/out, ��url
		defTalkCmd_removedev,		// �Ƴ�Խ��豸 in/out, ��url
		defTalkCmd_keepalive,		// ���� in/out, ��url
		defTalkCmd_quit,			// ���� in/out, ��url
		defTalkCmd_forcequit,		// ǿ���˳� in, ��url

		//-- �ڲ�����
		defTalkSelfCmd_GetDevice = 901, // ��ȡ�豸��Դ
	};

public:
	XmppGSTalk( const struTagParam &TagParam, defTalkCmd cmd, const std::string &url, const defvecDevKey &vecdev, const bool result=true );
	XmppGSTalk( const Tag* tag );
	~XmppGSTalk(void);

	static std::string TalkCmd2String( const defTalkCmd cmd );

	// reimplemented from StanzaExtension
	virtual const std::string& filterString() const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* newInstance( const Tag* tag ) const
	{
		return new XmppGSTalk( tag );
	}

	void PrintTag( const Tag* tag, const Stanza *stanza ) const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* clone() const
	{
		XmppGSTalk *pnew = new XmppGSTalk(NULL);
		*pnew = *this;
		return pnew;
	}

	// reimplemented from StanzaExt
	virtual Tag* tag() const;

	defTalkCmd get_cmd() const
	{
		return m_cmd;
	}

	const std::string& get_url() const
	{
		return m_url;
	}

	const defvecDevKey &get_vecdev() const
	{
		return m_vecdev;
	}

	const std::string& get_strSrcCmd() const
	{
		return m_strSrcCmd;
	}

private:
	struTagParam m_TagParam;

	defTalkCmd m_cmd;			// [in/out]
	std::string m_url;			// [in/out]
	defvecDevKey m_vecdev;		// [in/out]
	bool m_result;				// [out]

	std::string m_strSrcCmd;	// [in] save
};

