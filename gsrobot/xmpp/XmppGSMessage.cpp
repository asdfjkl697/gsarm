#include "XmppGSMessage.h"
#include "../gsiot/GSIOTClient.h"
#include "gloox/util.h"

XmppGSMessage::XmppGSMessage( const struTagParam &TagParam, const std::string &tojid, const std::string &subject, const std::string &body, uint32_t level )
	:StanzaExtension(ExtIotMessage), m_TagParam(TagParam), m_tojid(tojid), m_subject(subject), m_body(body), m_level(level), m_state(defGSReturn_Null)
{
}

XmppGSMessage::XmppGSMessage( const Tag* tag )
	:StanzaExtension(ExtIotMessage), m_level(5), m_state(defGSReturn_Null)
{
	if( !tag || tag->name() != "gsiot" || tag->xmlns() != XMLNS_GSIOT_MESSAGE )
		return;

	Tag *tmgr = tag->findChild("message");

	if( tmgr )
	{
		if( tmgr->findChild("id") )
		{
			m_id = tmgr->findChild("id")->cdata();
		}

		if( tmgr->findChild("state") )
		{
			std::string strState = tmgr->findChild("state")->cdata();
			g_toLowerCase( strState );
			if( strState == "success" )
			{
				m_state = defGSReturn_Success;
			}
		}
	}
}

XmppGSMessage::~XmppGSMessage(void)
{
}

void XmppGSMessage::PrintTag( const Tag* tag, const Stanza *stanza ) const
{
	if( tag ) GSIOTClient::XmppPrint( tag, "recv", stanza, false );
}

const std::string& XmppGSMessage::filterString() const
{
	static const std::string filter = "/iq/gsiot[@xmlns='" + XMLNS_GSIOT_MESSAGE + "']";
	return filter;
}

Tag* XmppGSMessage::tag() const
{
	Tag* i = new Tag( "gsiot" );
	i->setXmlns( XMLNS_GSIOT_MESSAGE );

	Tag *tmgr = new Tag( i,"message" );

	new Tag( tmgr, "tojid", m_tojid );
	new Tag( tmgr, "subject", ASCIIToUTF8(m_subject) );
	new Tag( tmgr, "body", ASCIIToUTF8(m_body) );
	new Tag( tmgr, "level", util::int2string(m_level) );

	return i;
}
