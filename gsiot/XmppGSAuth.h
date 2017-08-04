#pragma once

#include "typedef.h"
#include "gloox/stanzaextension.h"
#include "gloox/tag.h"
#include "GSIOTUser.h"

using namespace gloox;

class XmppGSAuth_base
{
public:
	XmppGSAuth_base( const std::string &srcmethod, defmapGSIOTUser &mapUser, const struTagParam &TagParam, bool mapSwapIn );
	XmppGSAuth_base( const XmppGSAuth_base &RightSides );
	XmppGSAuth_base();
	XmppGSAuth_base& operator= ( const XmppGSAuth_base &RightSides );
	~XmppGSAuth_base(void);

	// reimplemented from StanzaExt
	Tag* tagAuth_base( bool hasmethod ) const;

	//void XmppGSAuth_base::untagAuth_base( const Tag* tag );
	void untagAuth_base( const Tag* tag ); //20160606


	// 权限配置相关
	const defmapGSIOTUser& GetList_User() const
	{
		return this->m_mapUser;
	}

	// 权限配置相关
	defCfgOprt_ GetMethod() const
	{
		return this->m_method;
	}

	const std::string& GetSrcMethod() const
	{
		return this->m_srcmethod;
	}

private:
	struTagParam m_TagParam;
	defmapGSIOTUser m_mapUser;
	defCfgOprt_ m_method;
	std::string m_srcmethod;
};


//////////////////////////////////////////////////////////////////////


class XmppGSAuth :
	public XmppGSAuth_base,
	public StanzaExtension
{
public:
	XmppGSAuth( bool isSet, const std::string &srcmethod, defmapGSIOTUser &mapUser, const struTagParam &TagParam, bool mapSwapIn );
	XmppGSAuth( const Tag* tag );
	~XmppGSAuth(void);

	// reimplemented from StanzaExtension
	virtual const std::string& filterString() const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* newInstance( const Tag* tag ) const
	{
		return new XmppGSAuth( tag );
	}

	void PrintTag( const Tag* tag, const Stanza *stanza ) const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* clone() const
	{
		return new XmppGSAuth( *this );
	}

	// reimplemented from StanzaExt
	virtual Tag* tag() const;
	
private:
	bool m_isSet;
};


//////////////////////////////////////////////////////////////////////


class XmppGSAuth_User :
	public XmppGSAuth_base,
	public StanzaExtension
{
public:
	XmppGSAuth_User( const std::string &srcmethod, defmapGSIOTUser &mapUser, const struTagParam &TagParam, bool mapSwapIn );
	XmppGSAuth_User( const Tag* tag );
	~XmppGSAuth_User(void);

	// reimplemented from StanzaExtension
	virtual const std::string& filterString() const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* newInstance( const Tag* tag ) const
	{
		return new XmppGSAuth_User( tag );
	}

	void PrintTag( const Tag* tag, const Stanza *stanza ) const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* clone() const
	{
		return new XmppGSAuth_User( *this );
	}

	// reimplemented from StanzaExt
	virtual Tag* tag() const;

private:
};
