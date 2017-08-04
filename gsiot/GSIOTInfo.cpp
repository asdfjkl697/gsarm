#include "GSIOTInfo.h"
#include "gloox/tag.h"
#include "common.h"
#include "GSIOTClient.h"
#include <functional>

//#include <string.h>
#include <stdlib.h>


struct sort_GSIOTDevice_name : std::greater < GSIOTDevice* >
{
	bool operator()(const GSIOTDevice *_X, const GSIOTDevice *_Y) const
	{
		return (_X->getName() < _Y->getName());
	}
};


GSIOTInfo::GSIOTInfo(std::list<GSIOTDevice *>& deviceList, bool isInputDevice)
: StanzaExtension( ExtIot ), m_deviceList( deviceList ), m_isInputDevice(isInputDevice)
{
	m_deviceList.sort( sort_GSIOTDevice_name() );
}

GSIOTInfo::GSIOTInfo( const Tag* tag )
	:StanzaExtension( ExtIot ), m_isInputDevice(false)
{
	if( !tag || tag->name() != "gsiot" || tag->xmlns() != XMLNS_GSIOT )
       return;

	Tag *d = tag->findChild("device");
	if(d)
	{
		Tag *ttypelist = d->findChild("typelist");
		if( ttypelist )
		{
			const TagList& l = ttypelist->children();
			TagList::const_iterator it = l.begin();
			for( ; it != l.end(); ++it )
			{
				if((*it)->name() != "type")
					continue;

				m_getForType.insert( (IOTDeviceType)atoi( (*it)->cdata().c_str() ) );
			}
		}
	}

	//const TagList& l = tag->children();
	//TagList::const_iterator it = l.begin();
	//for( ; it != l.end(); ++it )
	//{
	//	if((*it)->name() == "device"){
	//		this->m_deviceList.push_back(new GSIOTDevice(*it));
	//	}
	//}
}


GSIOTInfo::~GSIOTInfo(void)
{
	if( !m_deviceList.empty() )
	{
		if( !m_isInputDevice )
		{
			for( std::list<GSIOTDevice*>::iterator it=m_deviceList.begin(); it!=m_deviceList.end(); ++it )
			{
				delete (*it);
			}
		}

		m_deviceList.clear();
	}
}

void GSIOTInfo::PrintTag( const Tag* tag, const Stanza *stanza ) const
{
	if( tag ) GSIOTClient::XmppPrint( tag, "recv", stanza, false );
}

const std::string& GSIOTInfo::filterString() const
{
	static const std::string filter = "/iq/gsiot[@xmlns='" + XMLNS_GSIOT + "']";
	return filter;
}


Tag* GSIOTInfo::tag() const
{
	Tag* t = new Tag( "gsiot" );
	t->setXmlns( XMLNS_GSIOT );

	Tag *tiot = new Tag( t, "iot" );
	tiot->addAttribute( "ver", g_IOTGetVersion()+"(build "+g_IOTGetBuildInfo()+")" );

	if( m_deviceList.size()>0){
		std::list<GSIOTDevice *>::const_iterator it = m_deviceList.begin();
		for(;it!=m_deviceList.end();it++){
		    Tag *c = new Tag(t, "device");
			c->addAttribute("id",(*it)->getId());
			c->addAttribute("name",ASCIIToUTF8((*it)->getName()));
			c->addAttribute("type",(*it)->getType());
			if( (*it)->getType()!=(*it)->getExType() && IOT_DEVICE_Unknown!=(*it)->getExType() ){ c->addAttribute( "extype", (*it)->getExType() ); }

			if( !(*it)->getVer().empty() && (*it)->getVer()!="1.0" ) c->addAttribute("ver",(*it)->getVer());
			c->addAttribute("readtype",(*it)->getReadType());
			
			const defUseable useable = (*it)->get_all_useable_state();
			if( defUseable_OK != useable ) { c->addAttribute("useable",useable); }; // 默认可用，不可用时才提供此值

			const defAlarmState AlarmState = (*it)->GetCurAlarmState();
			if( macAlarmState_IsAlarm(AlarmState) ) { c->addAttribute( "almst", AlarmState ); }; // 默认正常

			const std::string PrePicChangeCode = (*it)->GetPrePicChangeCode();
			if( !PrePicChangeCode.empty() ) { c->addAttribute( "prepic", PrePicChangeCode ); };
		}
	}
	return t;
}
