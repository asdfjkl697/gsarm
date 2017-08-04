#include "GSIOTDeviceInfo.h"
#include "GSIOTClient.h"
#include "gloox/util.h"

GSIOTDeviceInfo::GSIOTDeviceInfo(GSIOTDevice *device, defUserAuth Auth, int share, bool isInputDevice)
	:StanzaExtension(ExtIotDeviceInfo),m_device(device), m_Auth(Auth), m_share(share), m_isInputDevice(isInputDevice)
{

}

GSIOTDeviceInfo::GSIOTDeviceInfo( const Tag* tag)
	:StanzaExtension(ExtIotDeviceInfo), m_device(NULL), m_isInputDevice(false), m_Auth(defUserAuth_RW), m_share(0)
{
	if( !tag || tag->name() != "gsiot" || tag->xmlns() != XMLNS_GSIOT_DEVICE)
       return;

	Tag *tShare = tag->findChild("share");
	if( tShare )
	{
		if( atoi( tShare->cdata().c_str() ) )
		{
			m_share = defRunCodeVal_Spec_Enable;
		}
	}

	Tag *d = tag->findChild("device");
	if(d){
		m_device = new GSIOTDevice(NULL);

		if( d->hasAttribute("id") )
		{
			m_device->setId( atoi(d->findAttribute("id").c_str()) );
		}

		if( d->hasAttribute("type") )
		{
			m_device->setType( (IOTDeviceType)atoi(d->findAttribute("type").c_str()) );
		}
	}
}


GSIOTDeviceInfo::~GSIOTDeviceInfo(void)
{
	if(!m_isInputDevice && m_device){
	   delete(m_device);
	}
	m_device = NULL;
}

void GSIOTDeviceInfo::PrintTag( const Tag* tag, const Stanza *stanza ) const
{
	if( tag ) GSIOTClient::XmppPrint( tag, "recv", stanza, false );
}

const std::string& GSIOTDeviceInfo::filterString() const
{
	static const std::string filter = "/iq/gsiot[@xmlns='" + XMLNS_GSIOT_DEVICE + "']";
	return filter;
}


Tag* GSIOTDeviceInfo::tag() const
{
	Tag* t = new Tag( "gsiot" );
	t->setXmlns( XMLNS_GSIOT_DEVICE );

	if( this->isShare() )
	{
		new Tag( t, "share", util::int2string(this->isShare()?1:0) );
	}

	if( m_device!=NULL){
		t->addChild(m_device->tag(struTagParam(true, false, 0, m_Auth)));
	}
	return t;
}

bool GSIOTDeviceInfo::isShare() const
{
	return ( defRunCodeVal_Spec_Enable==m_share );
}
