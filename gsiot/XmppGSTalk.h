#pragma once
#include "typedef.h"
#include "stanzaextension.h"
#include "tag.h"
#include "GSIOTDevice.h"

using namespace gloox;

class XmppGSTalk :
	public StanzaExtension
{
public:
	enum defTalkCmd
	{
		defTalkCmd_Unknown	= 0,
		defTalkCmd_request	= 1,	// 请求 in, 无url
		defTalkCmd_accept,			// 接受 out, 无url
		defTalkCmd_reject,			// 拒绝 out, 无url
		defTalkCmd_session,			// 会话 in/out, 有url
		defTalkCmd_adddev,			// 增加对讲设备 in/out, 有url
		defTalkCmd_removedev,		// 移除对讲设备 in/out, 有url
		defTalkCmd_keepalive,		// 心跳 in/out, 有url
		defTalkCmd_quit,			// 结束 in/out, 有url
		defTalkCmd_forcequit,		// 强制退出 in, 无url

		//-- 内部命令
		defTalkSelfCmd_GetDevice = 901, // 获取设备资源
	};

public:
	XmppGSTalk( const struTagParam &TagParam, defTalkCmd cmd, const std::string &url, const defvecDevKey &vecdev, const bool result=true );
	XmppGSTalk( const Tag* tag );
	~XmppGSTalk(void);

	static std::string TalkCmd2String( const defTalkCmd cmd );

	// reimplemented from StanzaExtension
	virtual const std::string& filterString() const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* newInstance( const Tag* tag ) const
	{
		return new XmppGSTalk( tag );
	}

	void PrintTag( const Tag* tag, const Stanza *stanza ) const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* clone() const
	{
		XmppGSTalk *pnew = new XmppGSTalk(NULL);
		*pnew = *this;
		return pnew;
	}

	// reimplemented from StanzaExt
	virtual Tag* tag() const;

	defTalkCmd get_cmd() const
	{
		return m_cmd;
	}

	const std::string& get_url() const
	{
		return m_url;
	}

	const defvecDevKey &get_vecdev() const
	{
		return m_vecdev;
	}

	const std::string& get_strSrcCmd() const
	{
		return m_strSrcCmd;
	}

private:
	struTagParam m_TagParam;

	defTalkCmd m_cmd;			// [in/out]
	std::string m_url;			// [in/out]
	defvecDevKey m_vecdev;		// [in/out]
	bool m_result;				// [out]

	std::string m_strSrcCmd;	// [in] save
};

