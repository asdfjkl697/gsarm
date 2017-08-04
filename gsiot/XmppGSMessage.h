#pragma once
#include "typedef.h"
#include "gloox/stanzaextension.h"
#include "gloox/tag.h"

using namespace gloox;

class XmppGSMessage :
	public StanzaExtension
{
public:
	XmppGSMessage( const struTagParam &TagParam, const std::string &tojid, const std::string &subject, const std::string &body, uint32_t level=5 );
	XmppGSMessage( const Tag* tag );
	~XmppGSMessage(void);

	// reimplemented from StanzaExtension
	virtual const std::string& filterString() const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* newInstance( const Tag* tag ) const
	{
		return new XmppGSMessage( tag );
	}

	void PrintTag( const Tag* tag, const Stanza *stanza ) const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* clone() const
	{
		XmppGSMessage *pnew = new XmppGSMessage(NULL);
		*pnew = *this;
		return pnew;
	}

	// reimplemented from StanzaExt
	virtual Tag* tag() const;

	const std::string& get_id() const
	{
		return m_id;
	}

	defGSReturn get_state() const
	{
		return m_state;
	}

private:
	struTagParam m_TagParam;

	std::string m_id;		//消息ID
	defGSReturn m_state;	// success 或 fail

	std::string m_tojid;	//原JID
	std::string m_subject;	//主题
	std::string m_body;		//内容
	uint32_t m_level;		//消息等级（未启用）
};

