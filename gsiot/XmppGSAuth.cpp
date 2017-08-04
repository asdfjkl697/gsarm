#include "XmppGSAuth.h"
#include "GSIOTUserMgr.h"
#include "common.h"
#include "GSIOTClient.h"

XmppGSAuth_base::XmppGSAuth_base( const std::string &srcmethod, defmapGSIOTUser &mapUser, const struTagParam &TagParam, bool mapSwapIn )
	: m_srcmethod(srcmethod), m_TagParam(TagParam), m_method(defCfgOprt_Unknown)
{
	if( mapSwapIn )
	{
		m_mapUser.swap( mapUser );
	}
	else
	{
		GSIOTUserMgr::usermapCopy( m_mapUser, mapUser );
	}
}

XmppGSAuth_base::XmppGSAuth_base( const XmppGSAuth_base &RightSides )
{
	*this = RightSides;
}

XmppGSAuth_base::XmppGSAuth_base()
	: m_method(defCfgOprt_Unknown)
{

}

XmppGSAuth_base& XmppGSAuth_base::operator= ( const XmppGSAuth_base &RightSides )
{
	if( this == &RightSides )
	{
		return *this;
	}

	this->m_TagParam = RightSides.m_TagParam;
	this->m_method = RightSides.m_method;
	this->m_srcmethod = RightSides.m_srcmethod;

	GSIOTUserMgr::usermapCopy( this->m_mapUser, RightSides.m_mapUser );

	return *this;
}

XmppGSAuth_base::~XmppGSAuth_base(void)
{
	GSIOTUserMgr::usermapRelease( m_mapUser );
}

Tag* XmppGSAuth_base::tagAuth_base( bool hasmethod ) const
{
	Tag *tAuth = new Tag( "authority" );

	if( hasmethod )
	{
		tAuth->addAttribute( "method", m_srcmethod );
	}
	
	// 先加入来宾
	defmapGSIOTUser::const_iterator it = GSIOTUserMgr::usermapFind( m_mapUser, XMPP_GSIOTUser_Guest );
	if( it!=m_mapUser.end() )
	{
		GSIOTUser *pUser = it->second;
		tAuth->addChild( pUser->tag(m_TagParam) );
	}

	for( defmapGSIOTUser::const_iterator it=m_mapUser.begin(); it!=m_mapUser.end(); ++it )
	{
		GSIOTUser *pUser = it->second;

		if( GSIOTUserMgr::IsGuest(pUser) )
			continue;

		tAuth->addChild( pUser->tag(m_TagParam) );
	}

	return tAuth;
}

void XmppGSAuth_base::untagAuth_base( const Tag* tag )
{
	Tag *tAuth = tag->findChild("authority");

	if( tAuth )
	{
		if(tAuth->hasAttribute("method"))
		{
			m_srcmethod = tAuth->findAttribute("method");
			std::string method = m_srcmethod;
			g_toLowerCase( method );
			if( method == "add" )
			{
				m_method = defCfgOprt_AddModify;
			}
			else if( method == "edit" )
			{
				m_method = defCfgOprt_Modify;
			}
			else if( method == "delete" )
			{
				m_method = defCfgOprt_Delete;
			}
			else if( method == "getself" )
			{
				m_method = defCfgOprt_GetSelf;
			}
			else if( method == "getsimple" )
			{
				m_method = defCfgOprt_GetSimple;
			}
		}

		const TagList& l = tAuth->children();
		TagList::const_iterator it = l.begin();
		for( ; it != l.end(); ++it )
		{
			if((*it)->name() == "user")
			{
				GSIOTUser *pUser = new GSIOTUser(*it);
				std::string jid = pUser->GetKeyJid();

				if( GSIOTUserMgr::usermapFind( m_mapUser, jid ) == m_mapUser.end() )
				{
					GSIOTUserMgr::usermapInsert( m_mapUser, pUser );
				}
				else
				{
					delete pUser;
				}
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////


XmppGSAuth::XmppGSAuth( bool isSet, const std::string &srcmethod, defmapGSIOTUser &mapUser, const struTagParam &TagParam, bool mapSwapIn )
	: XmppGSAuth_base( srcmethod, mapUser, TagParam, mapSwapIn ), StanzaExtension( ExtIotAuthority ), m_isSet(isSet)
{
}

XmppGSAuth::XmppGSAuth( const Tag* tag )
	:StanzaExtension(ExtIotAuthority), m_isSet(false)
{
	if( !tag || tag->name() != "gsiot" || tag->xmlns() != XMLNS_GSIOT_AUTHORITY)
		return;

	untagAuth_base( tag );
}

XmppGSAuth::~XmppGSAuth(void)
{
}

void XmppGSAuth::PrintTag( const Tag* tag, const Stanza *stanza ) const
{
	if( tag ) GSIOTClient::XmppPrint( tag, "recv", stanza, false );
}

const std::string& XmppGSAuth::filterString() const
{
	static const std::string filter = "/iq/gsiot[@xmlns='" + XMLNS_GSIOT_AUTHORITY + "']";
	return filter;
}

Tag* XmppGSAuth::tag() const
{
	Tag* i = new Tag( "gsiot" );
	i->setXmlns( XMLNS_GSIOT_AUTHORITY );

	i->addChild( this->tagAuth_base(m_isSet) );

	return i;
}


//////////////////////////////////////////////////////////////////////


XmppGSAuth_User::XmppGSAuth_User( const std::string &srcmethod, defmapGSIOTUser &mapUser, const struTagParam &TagParam, bool mapSwapIn )
	: XmppGSAuth_base( srcmethod, mapUser, TagParam, mapSwapIn ), StanzaExtension( ExtIotAuthority_User )
{
}

XmppGSAuth_User::XmppGSAuth_User( const Tag* tag )
	:StanzaExtension(ExtIotAuthority_User)
{
	if( !tag || tag->name() != "gsiot" || tag->xmlns() != XMLNS_GSIOT_AUTHORITY_USER)
		return;

	untagAuth_base( tag );
}

XmppGSAuth_User::~XmppGSAuth_User(void)
{
}

void XmppGSAuth_User::PrintTag( const Tag* tag, const Stanza *stanza ) const
{
	if( tag ) GSIOTClient::XmppPrint( tag, "recv", stanza, false );
}

const std::string& XmppGSAuth_User::filterString() const
{
	static const std::string filter = "/iq/gsiot[@xmlns='" + XMLNS_GSIOT_AUTHORITY_USER + "']";
	return filter;
}

Tag* XmppGSAuth_User::tag() const
{
	Tag* i = new Tag( "gsiot" );
	i->setXmlns( XMLNS_GSIOT_AUTHORITY_USER );

	i->addChild( this->tagAuth_base(false) );

	return i;
}


