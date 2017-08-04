#include "XmppGSPlayback.h"
#include "../gsiot/GSIOTClient.h"
#include "gloox/util.h"


XmppGSPlayback::XmppGSPlayback( const struTagParam &TagParam, uint32_t camera_id, const std::string &url, const std::string &peerid, const std::string &streamid, const std::string &key, defPBState state, uint32_t startdt, uint32_t enddt, int m_sound )
	:StanzaExtension(ExtIotPlayback), m_TagParam(TagParam), m_camera_id(camera_id), m_startdt(startdt), m_enddt(enddt), m_url(url), m_peerid(peerid), m_streamid(streamid), m_key(key), m_state(state)
{
}

XmppGSPlayback::XmppGSPlayback( const Tag* tag )
	:StanzaExtension(ExtIotPlayback), m_camera_id(0), m_startdt(0), m_enddt(0), m_state(defPBState_Unknown), m_sound(-1)
{
	if( !tag || tag->name() != "gsiot" || tag->xmlns() != XMLNS_GSIOT_PLAYBACK )
		return;

	Tag *tmgr = tag->findChild("playback");

	if( tmgr )
	{
		Tag *tcamera = tmgr->findChild("device");
		if( tcamera )
		{
			if( tcamera->hasAttribute("id") )
			{
				m_camera_id = atoi( tcamera->findAttribute("id").c_str() );
			}
		}

		if( tmgr->findChild("startdate") )
		{
			m_startdt = atoi( tmgr->findChild("startdate")->cdata().c_str() );
		}

		if( tmgr->findChild("enddate") )
		{
			m_enddt = atoi( tmgr->findChild("enddate")->cdata().c_str() );
		}

		if( tmgr->findChild("url") )
		{
			m_url = tmgr->findChild("url")->cdata();
		}

		//#ifdef _DEBUG
#if 0
		std::string useUrl_lastback;
		if( IsRUNCODEEnable( defCodeIndex_TEST_Debug_TempCode ) )
		{
			if( !g_IsRTMFP_url( m_url ) ) useUrl_lastback = m_url;

			//m_url = "rtmfp://192.168.0.76:1985/p2p";
			m_url = "rtmfp://p2p.gsss.cn:1985/p2p";
			//m_url = "rtmfp://unknownp2p.gsss.cn:1985/p2p";

			GUID guid;
			::CoCreateGuid( &guid );
			m_url += "/";
			m_url += g_BufferToString( (unsigned char*)&guid, sizeof( guid ), false, false );
		}
#endif

		if( IsRUNCODEEnable( defCodeIndex_RTMFP_DelFromUrlStreamID ) && g_IsRTMFP_url( m_url ) )
		{
			std::vector<std::string> useUrl_unit_vec;
			split( m_url, useUrl_unit_vec, "/" );

			const int unitsize = useUrl_unit_vec.size();
			// ż�����ʾ���һ��Ϊstreamid��ȥ��
			if( 0 == (unitsize%2) )
			{
				g_replace_all_distinct( m_url, std::string( "/" )+useUrl_unit_vec[unitsize-1], "" );
			}
		}

		Tag *tchildlist_url_backup = tmgr->findChild("url_backup");
		if( tchildlist_url_backup )
		{
			const TagList& l = tchildlist_url_backup->children();
			TagList::const_iterator it = l.begin();
			for( ; it != l.end(); ++it )
			{
				Tag *tChild = (*it);
				if( tChild->name() == "url" )
				{
					m_url_backup.push_back( tChild->cdata() );
				}
			}
		}
		//if( !useUrl_lastback.empty() ) m_url_backup.push_back( useUrl_lastback );

		if( tmgr->findChild("key") )
		{
			m_key = tmgr->findChild("key")->cdata();
		}

		if( tmgr->findChild("state") )
		{
			std::string strState = tmgr->findChild("state")->cdata();
			g_toLowerCase( strState );
			if( strState == "start" )
			{
				m_state = defPBState_Start;
			}
			else if( strState == "stop" )
			{
				m_state = defPBState_Stop;
			}
			else if( strState == "set" )
			{
				m_state = defPBState_Set;
			}
			else if( strState == "get" )
			{
				m_state = defPBState_Get;
			}
			else if( strState == "pause" )
			{
				m_state = defPBState_Pause;
			}
			else if( strState == "resume" )
			{
				m_state = defPBState_Resume;
			}
			else if( strState == "normalplay" )
			{
				m_state = defPBState_NormalPlay;
			}
			else if( strState == "fastplay" )
			{
				m_state = defPBState_FastPlay;
			}
			else if( strState == "fastplaythrow" )
			{
				m_state = defPBState_FastPlayThrow;
			}
			else if( strState == "fastplay1" )
			{
				m_state = defPBState_FastPlay1;
			}
			else if( strState == "fastplay2" )
			{
				m_state = defPBState_FastPlay2;
			}
			else if( strState == "slowplay" )
			{
				m_state = defPBState_SlowPlay;
			}
			else if( strState == "slowplay1" )
			{
				m_state = defPBState_SlowPlay1;
			}
			else if( strState == "slowplay2" )
			{
				m_state = defPBState_SlowPlay2;
			}
			else if( strState == "stopall" )
			{
				m_state = defPBState_StopAll;
			}
		}

		if( tmgr->findChild("sound") )
		{
			m_sound = atoi( tmgr->findChild("sound")->cdata().c_str() );
		}
		else
		{
			m_sound = -1; // ��Чֵ
		}
	}
}

XmppGSPlayback::~XmppGSPlayback(void)
{
}

void XmppGSPlayback::PrintTag( const Tag* tag, const Stanza *stanza ) const
{
	if( tag ) GSIOTClient::XmppPrint( tag, "recv", stanza, false );
}

std::string XmppGSPlayback::TransState2Str( const defPBState state )
{
	switch( state )
	{
	case defPBState_Start:
		return std::string("start");

	case defPBState_Stop:
		return std::string("stop");

	case defPBState_Pause:
		return std::string("pause");

	case defPBState_NormalPlay:
		return std::string("normalplay");

	case defPBState_FastPlay:
	case defPBState_FastPlayThrow:
		return std::string("fastplay");

	case defPBState_FastPlay1:
		return std::string( "fastplay1" );

	case defPBState_FastPlay2:
		return std::string( "fastplay2" );

	case defPBState_SlowPlay:
		return std::string( "slowplay" );

	case defPBState_SlowPlay1:
		return std::string( "slowplay1" );

	case defPBState_SlowPlay2:
		return std::string( "slowplay2" );
	}

	return std::string("");
}

const std::string& XmppGSPlayback::filterString() const
{
	static const std::string filter = "/iq/gsiot[@xmlns='" + XMLNS_GSIOT_PLAYBACK + "']";
	return filter;
}

Tag* XmppGSPlayback::tag() const
{
	Tag* i = new Tag( "gsiot" );
	i->setXmlns( XMLNS_GSIOT_PLAYBACK );

	Tag *tmgr = new Tag( i,"playback" );

	Tag *tcamera = new Tag( tmgr, "device" );
	tcamera->addAttribute( "id", (int)m_camera_id );

	new Tag( tmgr, "url", m_url );
	new Tag( tmgr, "peerid", m_peerid );
	new Tag( tmgr, "streamid", m_streamid );

	new Tag( tmgr, "key", m_key );
	new Tag( tmgr, "state", this->TransState2Str(m_state) );

	return i;
}
