#pragma once

#include "typedef.h"
#include "gloox/stanzaextension.h"
#include "gloox/tag.h"
#include "GSIOTDevice.h"

using namespace gloox;

class XmppGSManager :
	public StanzaExtension
{
public:
	XmppGSManager( const std::string &srcmethod, const std::list<GSIOTDevice*> &devices, const struTagParam &TagParam );
	XmppGSManager( const XmppGSManager &RightSides );
	XmppGSManager( const Tag* tag );
	XmppGSManager& operator= ( const XmppGSManager &RightSides );
	~XmppGSManager(void);

	// reimplemented from StanzaExtension
	virtual const std::string& filterString() const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* newInstance( const Tag* tag ) const
	{
		return new XmppGSManager( tag );
	}

	void PrintTag( const Tag* tag, const Stanza *stanza ) const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* clone() const
	{
		XmppGSManager *pnew = new XmppGSManager(NULL);
		*pnew = *this;
		return pnew;
	}

	// reimplemented from StanzaExt
	virtual Tag* tag() const;


	const std::list<GSIOTDevice*>& GetDeviceList() const
	{
		return this->m_devices;
	}

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
	std::list<GSIOTDevice*> m_devices;
	defCfgOprt_ m_method;
	std::string m_srcmethod;
};

