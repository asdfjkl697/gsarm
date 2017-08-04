#include "XmppGSChange.h"
#include "GSIOTClient.h"
#include "gloox/util.h"
#include "common.h"

XmppGSChange::XmppGSChange( const struTagParam &TagParam, uint32_t change_count, uint32_t change_ts )
	:StanzaExtension(ExtIotChange), m_TagParam(TagParam), m_change_count(change_count), m_change_ts(change_ts)
{
}

XmppGSChange::XmppGSChange( const Tag* tag )
	:StanzaExtension(ExtIotChange), m_change_count(0), m_change_ts(0)
{
	if( !tag || tag->name() != "gsiot" || tag->xmlns() != XMLNS_GSIOT_Change )
		return;
}

XmppGSChange::~XmppGSChange(void)
{
}

void XmppGSChange::PrintTag( const Tag* tag, const Stanza *stanza ) const
{
	if( tag ) GSIOTClient::XmppPrint( tag, "recv", stanza, false );
}

const std::string& XmppGSChange::filterString() const
{
	static const std::string filter = "/iq/gsiot[@xmlns='" + XMLNS_GSIOT_Change + "']";
	return filter;
}

Tag* XmppGSChange::tag() const
{
	char buf[256] = {0};

	Tag* i = new Tag( "gsiot" );
	i->setXmlns( XMLNS_GSIOT_Change );

	Tag *tmgr = new Tag( i,"change" );

	Tag *tglobal = new Tag(tmgr, "global");

	snprintf( buf, sizeof(buf), "%08X%08X", m_change_count, m_change_ts );
	tglobal->addAttribute( "cfg", std::string(buf) );

	return i;
}
