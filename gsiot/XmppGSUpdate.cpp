#include "XmppGSUpdate.h"

#include "XmppGSPlayback.h"
#include "GSIOTClient.h"
#include "gloox/util.h"


XmppGSUpdate::XmppGSUpdate( const struTagParam &TagParam, const std::string &cur_ver, const std::string &last_ver, defUPState state )
	:StanzaExtension(ExtIotUpdate), m_TagParam(TagParam), m_cur_ver(cur_ver), m_last_ver(last_ver), m_state(state)
{
}

XmppGSUpdate::XmppGSUpdate( const Tag* tag )
	:StanzaExtension(ExtIotUpdate), m_state(defUPState_Unknown)
{
	if( !tag || tag->name() != "gsiot" || tag->xmlns() != XMLNS_GSIOT_UPDATE )
		return;

	Tag *tmgr = tag->findChild("update");

	if( tmgr )
	{
		if( tmgr->findChild("state") )
		{
			std::string strState = tmgr->findChild("state")->cdata();
			g_toLowerCase( strState );
			if( strState == "check" )
			{
				m_state = defUPState_check;
			}
			else if( strState == "update" )
			{
				m_state = defUPState_update;
			}
			else if( strState == "forceupdate" )
			{
				m_state = defUPState_forceupdate;
			}
		}
	}
}

XmppGSUpdate::~XmppGSUpdate(void)
{
}

void XmppGSUpdate::PrintTag( const Tag* tag, const Stanza *stanza ) const
{
	if( tag ) GSIOTClient::XmppPrint( tag, "recv", stanza, false );
}

const std::string& XmppGSUpdate::filterString() const
{
	static const std::string filter = "/iq/gsiot[@xmlns='" + XMLNS_GSIOT_UPDATE + "']";
	return filter;
}

Tag* XmppGSUpdate::tag() const
{
	Tag* i = new Tag( "gsiot" );
	i->setXmlns( XMLNS_GSIOT_UPDATE );

	Tag *tmgr = new Tag( i,"update" );

	if( !m_cur_ver.empty() ) { new Tag( tmgr, "version", m_cur_ver ); }
	if( !m_last_ver.empty() ) { new Tag( tmgr, "latest", m_last_ver ); }

	std::string strstate;
	switch(m_state)
	{
	case defUPState_checkfailed:
		strstate = "check failed";
		break;

	case defUPState_latest:
		strstate = "latest";
		break;

	case defUPState_update:
		strstate = "update";
		break;

	case defUPState_updatefailed:
		strstate = "update failed";
		break;

	case defUPState_progress:
		strstate = "progress";
		break;

	case defUPState_successupdated:
		strstate = "updated";
		break;
	}
	new Tag( tmgr, "state", strstate );

	return i;
}
