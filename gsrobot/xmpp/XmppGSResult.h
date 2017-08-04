#pragma once

#include "../typedef.h"
#include "gloox/stanzaextension.h"
#include "gloox/tag.h"

using namespace gloox;

class XmppGSResult : public StanzaExtension
{
public:
	XmppGSResult( const std::string &from_namespace, const defGSReturn code, const defNormResultMod_ module=defNormResultMod_null, const std::string &message=EmptyString );
	XmppGSResult( const Tag* tag );
	~XmppGSResult(void);

	// reimplemented from StanzaExtension
	virtual const std::string& filterString() const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* newInstance( const Tag* tag ) const
	{
		return new XmppGSResult( tag );
	}

	void PrintTag( const Tag* tag, const Stanza *stanza ) const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* clone() const
	{
		return new XmppGSResult( *this );
	}

	// reimplemented from StanzaExt
	virtual Tag* tag() const;

private:
	std::string m_namespace;
	defNormResultMod_ m_module;
	defGSReturn m_code;
	std::string m_message;
};

