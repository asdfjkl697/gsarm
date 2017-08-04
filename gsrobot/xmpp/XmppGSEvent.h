#pragma once

#include "../typedef.h"
#include "gloox/stanzaextension.h"
#include "gloox/tag.h"
#include "../ControlEvent.h"
#include "../gsiot/GSIOTDevice.h"
#include "../EditAttrMgr.h"

class XmppGSEvent :
	public StanzaExtension,
	public EditAttrMgr
{
public:
	XmppGSEvent( const std::string &srcmethod, const GSIOTDevice *pDevice, std::list<ControlEvent*> &Events, uint32_t RunState, const struTagParam &TagParam, bool mapSwapIn );
	XmppGSEvent( const XmppGSEvent &RightSides );
	XmppGSEvent( const Tag* tag );
	XmppGSEvent& operator= ( const XmppGSEvent &RightSides );
	~XmppGSEvent(void);

	// reimplemented from StanzaExtension
	virtual const std::string& filterString() const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* newInstance( const Tag* tag ) const
	{
		return new XmppGSEvent( tag );
	}

	void PrintTag( const Tag* tag, const Stanza *stanza ) const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* clone() const
	{
		XmppGSEvent *pnew = new XmppGSEvent(NULL);
		*pnew = *this;
		return pnew;
	}

	// reimplemented from StanzaExt
	virtual Tag* tag() const;


	const std::list<ControlEvent*>& GetEventList() const
	{
		return this->m_Events;
	}

	GSIOTDevice* GetDevice() const
	{
		return this->m_pDevice;
	}

	defCfgOprt_ GetMethod() const
	{
		return this->m_method;
	}

	const std::string& GetSrcMethod() const
	{
		return this->m_srcmethod;
	}
	
	uint32_t GetRunState() const
	{
		return this->m_runstate;
	}

private:
	struTagParam m_TagParam;
	GSIOTDevice *m_pDevice;
	std::list<ControlEvent*> m_Events;
	defCfgOprt_ m_method;
	std::string m_srcmethod;
	uint32_t m_runstate;
};

