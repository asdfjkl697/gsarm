#include "XmppGSTalk.h"
#include "../gsiot/GSIOTClient.h"
#include "gloox/util.h"


XmppGSTalk::XmppGSTalk( const struTagParam &TagParam, defTalkCmd cmd, const std::string &url, const defvecDevKey &vecdev, const bool result )
	:StanzaExtension(ExtIotTalk), m_TagParam(TagParam), m_cmd(cmd), m_url(url), m_result(result)
{
	m_vecdev = vecdev;
}

XmppGSTalk::XmppGSTalk( const Tag* tag )
	:StanzaExtension(ExtIotTalk), m_cmd(defTalkCmd_Unknown), m_result(true)
{
	if( !tag || tag->name() != "gstalk" || tag->xmlns() != XMLNS_GSIOT_TALK )
		return;

	Tag *tmgr = tag->findChild("gstalk");

	if( tmgr )
	{
		if( tmgr->findChild("command") )
		{
			m_strSrcCmd = tmgr->findChild("command")->cdata();
			std::string strcmd = m_strSrcCmd;
			g_toLowerCase( strcmd );
			if( strcmd == "request" )
			{
				m_cmd = defTalkCmd_request;
			}
			else if( strcmd == "accept" )
			{
				m_cmd = defTalkCmd_accept;
			}
			else if( strcmd == "reject" )
			{
				m_cmd = defTalkCmd_reject;
			}
			else if( strcmd == "session" )
			{
				m_cmd = defTalkCmd_session;
			}
			else if( strcmd == "adddev" )
			{
				m_cmd = defTalkCmd_adddev;
			}
			else if( strcmd == "removedev" )
			{
				m_cmd = defTalkCmd_removedev;
			}
			else if( strcmd == "keepalive" )
			{
				m_cmd = defTalkCmd_keepalive;
			}
			else if( strcmd == "quit" )
			{
				m_cmd = defTalkCmd_quit;
			}
			else if( strcmd == "forcequit" )
			{
				m_cmd = defTalkCmd_forcequit;
			}
			else
			{
				m_cmd = defTalkCmd_Unknown;
			}
		}

		if( tmgr->findChild("url") )
		{
			m_url = tmgr->findChild("url")->cdata();
		}

		Tag *tdevicelist = tmgr->findChild("devicelist");
		if( tdevicelist )
		{
			const TagList& l = tdevicelist->children();
			TagList::const_iterator it = l.begin();
			for( ; it != l.end(); ++it )
			{
				Tag *tDev = (*it);
				if( tDev->name() == "device" )
				{
					if( tDev->hasAttribute("type") && tDev->hasAttribute("id") )
					{
						IOTDeviceType dev_type = (IOTDeviceType)atoi( tDev->findAttribute("type").c_str() );
						int dev_id = atoi( tDev->findAttribute("id").c_str() );

						m_vecdev.push_back( GSIOTDeviceKey(dev_type,dev_id) );
					}
				}
			}
		}
	}
}

XmppGSTalk::~XmppGSTalk(void)
{
}

void XmppGSTalk::PrintTag( const Tag* tag, const Stanza *stanza ) const
{
	if( tag ) GSIOTClient::XmppPrint( tag, "recv", stanza, false );
}

std::string XmppGSTalk::TalkCmd2String( const defTalkCmd cmd )
{
	switch( cmd )
	{
	case defTalkCmd_request:
		return "request";

	case defTalkCmd_accept:
		return "accept";

	case defTalkCmd_reject:
		return "reject";

	case defTalkCmd_session:
		return "session";

	case defTalkCmd_adddev:
		return "adddev";

	case defTalkCmd_removedev:
		return "removedev";

	case defTalkCmd_keepalive:
		return "keepalive";

	case defTalkCmd_quit:
		return "quit";

	case defTalkCmd_forcequit:
		return "forcequit";

	default:
		return "unknown";
	}

	return "unknown";
}

const std::string& XmppGSTalk::filterString() const
{
	static const std::string filter = "/iq/gstalk[@xmlns='" + XMLNS_GSIOT_TALK + "']";
	return filter;
}

Tag* XmppGSTalk::tag() const
{
	Tag* i = new Tag( "gstalk" );
	i->setXmlns( XMLNS_GSIOT_TALK );

	Tag *tmgr = new Tag( i,"gstalk" );

	std::string strcmd = TalkCmd2String( m_cmd );
	
	new Tag( tmgr, "command", strcmd );

	new Tag( tmgr, "url", m_url );

	if( !m_vecdev.empty() )
	{
		Tag *tdevicelist = new Tag( tmgr,"devicelist" );
		for( int i=0; i<m_vecdev.size(); ++i )
		{
			Tag *tDev = new Tag( tdevicelist, "device" );
			tDev->addAttribute( "type", (int)m_vecdev[i].m_type );
			tDev->addAttribute( "id", (int)m_vecdev[i].m_id );
		}
	}

	switch( m_cmd )
	{
	case defTalkCmd_adddev:
	case defTalkCmd_removedev:
		{
			new Tag( tmgr, "result", m_result?"true":"false" );
		}
		break;

	default:
		break;
	}

	return i;
}
