#pragma once
#include "../typedef.h"
#include "gloox/stanzaextension.h"
#include "gloox/tag.h"
#include "../gsiot/GSIOTConfig.h"

using namespace gloox;

class XmppGSRelation :
	public StanzaExtension
{
public:
	XmppGSRelation( const struTagParam &TagParam, IOTDeviceType device_type, int device_id, const deflstRelationChild &ChildList, defGSReturn result=defGSReturn_Null );
	XmppGSRelation( const Tag* tag );
	~XmppGSRelation(void);

	// reimplemented from StanzaExtension
	virtual const std::string& filterString() const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* newInstance( const Tag* tag ) const
	{
		return new XmppGSRelation( tag );
	}

	void PrintTag( const Tag* tag, const Stanza *stanza ) const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* clone() const
	{
		XmppGSRelation *pnew = new XmppGSRelation(NULL);
		*pnew = *this;
		return pnew;
	}

	// reimplemented from StanzaExt
	virtual Tag* tag() const;

	IOTDeviceType get_device_type() const
	{
		return m_device_type;
	}

	int get_device_id() const
	{
		return m_device_id;
	}

	const deflstRelationChild& get_ChildList() const
	{
		return m_ChildList;
	}
	
	void SetResult( defGSReturn result )
	{
		m_result = result;
	}

	defGSReturn GetResult() const
	{
		return m_result;
	}

private:
	struTagParam m_TagParam;
	defGSReturn m_result;

	IOTDeviceType m_device_type;
	int m_device_id;
	deflstRelationChild m_ChildList;
};
