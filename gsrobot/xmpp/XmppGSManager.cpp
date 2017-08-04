#include "XmppGSManager.h"
#include "../common.h"
#include "../gsiot/GSIOTClient.h"

XmppGSManager::XmppGSManager( const std::string &srcmethod, const std::list<GSIOTDevice*> &devices, const struTagParam &TagParam )
	:StanzaExtension(ExtIotManager), m_method(defCfgOprt_Unknown), m_srcmethod(srcmethod), m_TagParam(TagParam)
{
	for( std::list<GSIOTDevice*>::const_iterator it=devices.begin(); it!=devices.end(); ++it )
	{
		m_devices.push_back( (*it)->clone() );
	}
}

XmppGSManager::XmppGSManager( const Tag* tag )
	:StanzaExtension(ExtIotManager), m_method(defCfgOprt_Unknown)
{
	if( !tag || tag->name() != "gsiot" || tag->xmlns() != XMLNS_GSIOT_MANAGER )
		return;

	Tag *tmgr = tag->findChild("manager");

	if( tmgr )
	{
		if(tmgr->hasAttribute("method"))
		{
			m_srcmethod = tmgr->findAttribute("method");
			std::string method = m_srcmethod;
			g_toLowerCase( method );
			if( method == "add" )
			{
				m_method = defCfgOprt_Add;
			}
			else if( method == "edit" )
			{
				m_method = defCfgOprt_Modify;
			}
			else if( method == "delete" )
			{
				m_method = defCfgOprt_Delete;
			}
		}

		const TagList& l = tmgr->children();
		TagList::const_iterator it = l.begin();
		for( ; it != l.end(); ++it )
		{
			if((*it)->name() == "device")
			{
				m_devices.push_back( new GSIOTDevice(*it) );
			}
		}
	}
}

XmppGSManager& XmppGSManager::operator= ( const XmppGSManager &RightSides )
{
	if( this == &RightSides )
	{
		return *this;
	}

	this->m_TagParam = RightSides.m_TagParam;
	this->m_method = RightSides.m_method;
	this->m_srcmethod = RightSides.m_srcmethod;

	for( std::list<GSIOTDevice*>::const_iterator it=RightSides.m_devices.begin(); it!=RightSides.m_devices.end(); ++it )
	{
		this->m_devices.push_back( (*it)->clone() );
	}

	return *this;
}

XmppGSManager::~XmppGSManager(void)
{
	while( !m_devices.empty() )
	{
		GSIOTDevice *p = m_devices.front();
		delete(p);
		m_devices.pop_front();
	}
}

void XmppGSManager::PrintTag( const Tag* tag, const Stanza *stanza ) const
{
	if( tag ) GSIOTClient::XmppPrint( tag, "recv", stanza, false );
}

const std::string& XmppGSManager::filterString() const
{
	static const std::string filter = "/iq/gsiot[@xmlns='" + XMLNS_GSIOT_MANAGER + "']";
	return filter;
}


Tag* XmppGSManager::tag() const
{
	Tag* i = new Tag( "gsiot" );
	i->setXmlns( XMLNS_GSIOT_MANAGER );

	Tag *tmgr = new Tag( i,"manager" );

	tmgr->addAttribute( "method", m_srcmethod );

	for( std::list<GSIOTDevice*>::const_iterator it=m_devices.begin(); it!=m_devices.end(); ++it )
	{
		tmgr->addChild( (*it)->tag(m_TagParam) );
	}

	return i;
}
