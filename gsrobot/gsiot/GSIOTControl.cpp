#include "GSIOTControl.h"
#include "gloox/tag.h"
#include "GSIOTClient.h"


GSIOTControl::GSIOTControl(GSIOTDevice *device, defUserAuth Auth, bool isInputDevice)
	:StanzaExtension(ExtIotControl),m_device(device), m_NeedRet(0), m_Auth(Auth), m_isInputDevice(isInputDevice)
{
}
GSIOTControl::GSIOTControl( const Tag* tag)
	:StanzaExtension(ExtIotControl), m_device(NULL), m_NeedRet(0), m_Auth(defUserAuth_RW), m_isInputDevice(false)
{
	if( !tag || tag->name() != "gsiot" || tag->xmlns() != XMLNS_GSIOT_CONTROL)
       return;

	Tag *pret = tag->findChild("needret");
	if(pret)
	{
		m_NeedRet = atoi( pret->cdata().c_str() );
	}

	Tag *d = tag->findChild("device");
	if(d){
	    m_device = new GSIOTDevice(d);
	}
}

GSIOTControl::~GSIOTControl(void)
{
	if(!m_isInputDevice && m_device){
	   delete(m_device);
	}
	m_device = NULL;
}

void GSIOTControl::PrintTag( const Tag* tag, const Stanza *stanza ) const
{
	if( tag ) GSIOTClient::XmppPrint( tag, "recv", stanza, false );
}

const std::string& GSIOTControl::filterString() const
{
	static const std::string filter = "/iq/gsiot[@xmlns='" + XMLNS_GSIOT_CONTROL + "']";
	return filter;
}


Tag* GSIOTControl::tag() const
{
	Tag* t = new Tag( "gsiot" );
	t->setXmlns( XMLNS_GSIOT_CONTROL );

	if( m_device!=NULL){
		t->addChild(m_device->tag(struTagParam(true, false, 0, m_Auth)));
	}
	return t;
}
