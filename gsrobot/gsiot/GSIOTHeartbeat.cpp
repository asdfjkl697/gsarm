#include "GSIOTHeartbeat.h"
#include "gloox/tag.h"
#include "gloox/util.h"
#include "GSIOTClient.h"

GSIOTHeartbeat::GSIOTHeartbeat(int deviceID,const std::string& ver)
	: StanzaExtension( ExtIotHeartbeat ), m_ver(ver), m_deviceID(deviceID)
{
}

GSIOTHeartbeat::GSIOTHeartbeat( const Tag* tag )
	:StanzaExtension( ExtIotHeartbeat )
{
	if( !tag || tag->name() != "gsiot" || tag->xmlns() != XMLNS_GSIOT_HEARTBEAT )
       return;

	Tag *h = tag->findChild("heartbeat");
	if(h){
	    this->m_ver = h->findAttribute("ver");
		this->m_deviceID = atoi(h->findAttribute("deviceid").c_str());
	}
}

GSIOTHeartbeat::~GSIOTHeartbeat(void)
{
}

void GSIOTHeartbeat::PrintTag( const Tag* tag, const Stanza *stanza ) const
{
	if( tag ) GSIOTClient::XmppPrint( tag, "recv", stanza, false );
}

const std::string& GSIOTHeartbeat::filterString() const
{
	static const std::string filter = "/iq/gsiot[@xmlns='" + XMLNS_GSIOT_HEARTBEAT + "']";
	return filter;
}


Tag* GSIOTHeartbeat::tag() const
{
	Tag* t = new Tag( "gsiot" );
	t->setXmlns( XMLNS_GSIOT_HEARTBEAT );

	Tag *hb = new Tag(t,"heartbeat");
	hb->addAttribute("ver",this->m_ver);
	hb->addAttribute("deviceid", util::int2string(this->m_deviceID));
	return t;
}