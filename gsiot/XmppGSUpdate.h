#pragma once
#include "typedef.h"
#include "gloox/stanzaextension.h"
#include "gloox/tag.h"

using namespace gloox;

class XmppGSUpdate :
	public StanzaExtension
{
public:
	enum defUPState
	{//状态 send: latest当前为最新版本；update 有新版本；progress  正在更新；updated 已经更新；recv: update开始更新
		defUPState_Unknown	= 0,
		defUPState_check,
		defUPState_checkfailed,
		defUPState_latest,
		defUPState_update,
		defUPState_updatefailed,
		defUPState_forceupdate,
		defUPState_progress,
		defUPState_successupdated
	};

public:
	XmppGSUpdate( const struTagParam &TagParam, const std::string &cur_ver, const std::string &last_ver, defUPState state=defUPState_Unknown );
	XmppGSUpdate( const Tag* tag );
	~XmppGSUpdate(void);

	// reimplemented from StanzaExtension
	virtual const std::string& filterString() const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* newInstance( const Tag* tag ) const
	{
		return new XmppGSUpdate( tag );
	}

	void PrintTag( const Tag* tag, const Stanza *stanza ) const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* clone() const
	{
		XmppGSUpdate *pnew = new XmppGSUpdate(NULL);
		*pnew = *this;
		return pnew;
	}

	// reimplemented from StanzaExt
	virtual Tag* tag() const;
	
	const std::string& get_cur_ver() const
	{
		return m_cur_ver;
	}

	const std::string& get_key() const
	{
		return m_last_ver;
	}

	defUPState get_state() const
	{
		return m_state;
	}

private:
	struTagParam m_TagParam;

	std::string m_cur_ver;	// [in]当前版本
	std::string m_last_ver;	// [in]最新版本
	defUPState m_state;		// [in/out] 状态 send: latest当前为最新版本；update 有新版本；progress  正在更新；updated 已经更新；recv: update开始更新
};
