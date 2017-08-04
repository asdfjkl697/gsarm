#include "XmppGSEvent.h"
#include "GSIOTClient.h"
#include "gloox/util.h"
#include "AutoControlEvent.h"
#include "AutoNoticeEvent.h"
#include "AutoSendSMSEvent.h"
#include "AutoEventthing.h"



XmppGSEvent::XmppGSEvent( const std::string &srcmethod, const GSIOTDevice *pDevice, std::list<ControlEvent*> &Events, uint32_t RunState, const struTagParam &TagParam, bool mapSwapIn )
	:StanzaExtension(ExtIotEvent), m_method(defCfgOprt_Unknown), m_srcmethod(srcmethod), m_TagParam(TagParam), m_runstate(RunState)
{
	m_pDevice = pDevice->clone(false);

	if( mapSwapIn )
	{
		this->m_Events.swap( Events );
	}
	else
	{
		for( std::list<ControlEvent*>::const_iterator it=Events.begin(); it!=Events.end(); ++it )
		{
			m_Events.push_back( (*it)->clone() );
		}
	}
}

XmppGSEvent::XmppGSEvent( const Tag* tag )
	:StanzaExtension(ExtIotEvent), m_method(defCfgOprt_Unknown), m_pDevice(NULL), m_runstate(1)
{
	if( !tag || tag->name() != "gsiot" || tag->xmlns() != XMLNS_GSIOT_EVENT )
		return;

	//GSIOTClient::XmppPrint( tag, "recv", stanza, false ); //jyc20170227 debug
	//XmppGSEvent::PrintTag( tag , ExtIotEvent);

	Tag *tmgr = tag->findChild("event");

	if( tmgr )
	{
		Tag *tDevice = tmgr->findChild("device");
		if( tDevice )
		{
			m_pDevice = new GSIOTDevice(tDevice);
		}
		else
		{
			LOGMSG( "not found device tag" );
			return;
		}

		this->UntagEditAttr( tmgr );

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

		Tag *tdo = tmgr->findChild("do");  //jyc20170223 trans  "edit trigger"
		if( tdo )
		{
			const TagList& l = tdo->children();
			TagList::const_iterator it = l.begin();
			for( ; it != l.end(); ++it )
			{
				ControlEvent *pNew = NULL;

				if((*it)->name() == "smsthing")
				{
					pNew = new AutoSendSMSEvent(*it);
				}
				else if((*it)->name() == "mailthing")
				{
					//...
				}
				else if((*it)->name() == "messagething")
				{
					pNew = new AutoNoticeEvent(*it);
				}
				else if((*it)->name() == "devicething")
				{
					pNew = new AutoControlEvent(*it);
				}
				else if((*it)->name() == "callthing")
				{
					//...
				}
				else if((*it)->name() == "eventthing")
				{
					pNew = new AutoEventthing(*it);
				}

				if( pNew )
				{
					if( m_pDevice )
					{
						pNew->SetDeviceType( m_pDevice->getType() );
						pNew->SetDeviceID( m_pDevice->getId() );
					}

					m_Events.push_back( pNew );
				}
			}
		}

		Tag *tState = tmgr->findChild("state");
		if( tState )
		{
			m_runstate = atoi( tState->cdata().c_str() );
		} 
	}
}

XmppGSEvent& XmppGSEvent::operator= ( const XmppGSEvent &RightSides )
{
	if( this == &RightSides )
	{
		return *this;
	}

	this->m_AttrMap = RightSides.m_AttrMap;

	this->m_TagParam = RightSides.m_TagParam;
	this->m_method = RightSides.m_method;
	this->m_srcmethod = RightSides.m_srcmethod;

	if( m_pDevice )
	{
		delete m_pDevice;
		m_pDevice = NULL;
	}

	if( RightSides.m_pDevice )
		this->m_pDevice = RightSides.m_pDevice->clone(false);

	this->m_runstate = RightSides.m_runstate;

	for( std::list<ControlEvent*>::const_iterator it=RightSides.m_Events.begin(); it!=RightSides.m_Events.end(); ++it )
	{
		this->m_Events.push_back( (*it)->clone() );
	}

	return *this;
}

XmppGSEvent::~XmppGSEvent(void)
{
	while( !m_Events.empty() )
	{
		ControlEvent *p = m_Events.front();
		delete(p);
		m_Events.pop_front();
	}

	if( m_pDevice )
	{
		delete m_pDevice;
		m_pDevice = NULL;
	}
}

void XmppGSEvent::PrintTag( const Tag* tag, const Stanza *stanza ) const
{
	if( tag ) GSIOTClient::XmppPrint( tag, "recv", stanza, false );
}

const std::string& XmppGSEvent::filterString() const
{
	static const std::string filter = "/iq/gsiot[@xmlns='" + XMLNS_GSIOT_EVENT + "']";
	return filter;
}

Tag* XmppGSEvent::tag() const
{
	Tag* i = new Tag( "gsiot" );
	i->setXmlns( XMLNS_GSIOT_EVENT );

	Tag *tmgr = new Tag( i,"event" );

	tmgr->addAttribute( "method", m_srcmethod );

	if( m_pDevice ) tmgr->addChild( m_pDevice->tag(m_TagParam) );

	if( m_TagParam.isValid && m_TagParam.isResult )
	{
		;
	}
	else
	{
		new Tag( tmgr, "state", m_runstate?"1":"0" );
	}

	Tag *cdo = new Tag( tmgr, "do" );

	for( std::list<ControlEvent*>::const_iterator it=m_Events.begin(); it!=m_Events.end(); ++it )
	{
		cdo->addChild( (*it)->tag(m_TagParam) );
	}

	return i;
}
