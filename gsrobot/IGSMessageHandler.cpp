#include "IGSMessageHandler.h"
//#include "logFileExFunc.h"
//#include "windows.h"
#include "xmpp/XmppGSAuth.h"
#include "xmpp/XmppGSManager.h"
#include "xmpp/XmppGSEvent.h"
#include "xmpp/XmppGSRelation.h"
#include "xmpp/XmppGSPreset.h"
#include "xmpp/XmppGSVObj.h"


GSMessage::GSMessage( const defGSMsgType_ MsgType, const JID& From, const std::string& id, StanzaExtension *const pEx )
	:m_MsgType(MsgType), m_From(From), m_id(id), m_pEx(pEx) 
{
	this->m_ts = timeGetTime();
}

bool GSMessage::isOverTime( uint32_t overtime ) const
{
	return (timeGetTime()-m_ts > overtime);
}

GSMessage::~GSMessage(void)
{
	switch(m_pEx->extensionType())
	{
	case ExtIotAuthority:
		{
			if( m_pEx ) delete( (XmppGSAuth*)m_pEx );
		}
		break;

	case ExtIotManager:
		{
			if( m_pEx ) delete( (XmppGSManager*)m_pEx );
		}
		break;
		
	case ExtIotEvent:
		{
			if( m_pEx ) delete( (XmppGSEvent*)m_pEx );
		}
		break;

	case ExtIotRelation:
		{
			if( m_pEx ) delete( (XmppGSRelation*)m_pEx );
		}
		break;
		
	case ExtIotPreset:
		{
			if( m_pEx ) delete( (XmppGSPreset*)m_pEx );
		}
		break;
		
	case ExtIotVObj:
		{
			if( m_pEx ) delete( (XmppGSVObj*)m_pEx );
		}
		break;
		
	default:
		{
			//LOGMSGEX( defLOGNAME, defLOG_ERROR, "~GSMessage type=%d err, UserTypeBase=%d", m_pEx->extensionType(), gloox::ExtUser );
		}
		break;
	}

	m_pEx = NULL;
}
