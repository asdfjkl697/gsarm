#pragma once
#include "../typedef.h"
#include "gloox/stanzaextension.h"
#include "gloox/tag.h"

using namespace gloox;

class XmppGSChange :
	public StanzaExtension
{
public:
	XmppGSChange( const struTagParam &TagParam, uint32_t change_count, uint32_t change_ts );
	XmppGSChange( const Tag* tag );
	~XmppGSChange(void);

	// reimplemented from StanzaExtension
	virtual const std::string& filterString() const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* newInstance( const Tag* tag ) const
	{
		return new XmppGSChange( tag );
	}

	void PrintTag( const Tag* tag, const Stanza *stanza ) const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* clone() const
	{
		XmppGSChange *pnew = new XmppGSChange(NULL);
		*pnew = *this;
		return pnew;
	}

	// reimplemented from StanzaExt
	virtual Tag* tag() const;

private:
	struTagParam m_TagParam;
	uint32_t m_change_count;
	uint32_t m_change_ts;
};

