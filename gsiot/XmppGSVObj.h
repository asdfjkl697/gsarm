#pragma once
#include "typedef.h"
#include "gloox/stanzaextension.h"
#include "gloox/tag.h"
#include "GSIOTConfig.h"

using namespace gloox;

class XmppGSVObj :
	public StanzaExtension
{
public:
	XmppGSVObj( const struTagParam &TagParam, const std::string &srcmethod, const defmapVObjConfig &VObjCfgList, defGSReturn result=defGSReturn_Err );
	XmppGSVObj( const Tag* tag );
	~XmppGSVObj(void);

	// reimplemented from StanzaExtension
	virtual const std::string& filterString() const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* newInstance( const Tag* tag ) const
	{
		return new XmppGSVObj( tag );
	}

	void PrintTag( const Tag* tag, const Stanza *stanza ) const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* clone() const
	{
		XmppGSVObj *pnew = new XmppGSVObj(NULL);
		*pnew = *this;
		return pnew;
	}

	// reimplemented from StanzaExt
	virtual Tag* tag() const;

	const defmapVObjConfig& get_VObjCfgList() const
	{
		return m_VObjCfgList;
	}
	
	void SetResult( defGSReturn result )
	{
		m_result = result;
	}

	defGSReturn GetResult() const
	{
		return m_result;
	}

	bool isAllType() const
	{
		return m_getForType.empty();
	}

	bool isInGetType( IOTDeviceType type ) const
	{
		return ( m_getForType.find(type) != m_getForType.end() );
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
	defGSReturn m_result;
	
	std::set<IOTDeviceType> m_getForType;

	defCfgOprt_ m_method;
	std::string m_srcmethod;
	defmapVObjConfig m_VObjCfgList;
};
