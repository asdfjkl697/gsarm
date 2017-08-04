#pragma once
#include "typedef.h"
#include "gloox/stanzaextension.h"
#include "gloox/tag.h"

using namespace gloox;

class XmppGSPlayback :
	public StanzaExtension
{
public:
	enum defPBState
	{
		defPBState_Unknown	= 0,
		defPBState_Start	= 1,
		defPBState_Stop		= 2,
		defPBState_Set		= 3,
		defPBState_Get,
		defPBState_Pause,				// 暂停播放
		defPBState_Resume,				// 暂停恢复，从暂停恢复到暂停前状态播放
		defPBState_NormalPlay,			// 正常速度
		defPBState_FastPlay,			// 快放
		defPBState_FastPlay1,			// 快放1级
		defPBState_FastPlay2,			// 快放2级
		defPBState_FastPlayThrow,		// 抛帧快放
		defPBState_SlowPlay,			// 慢放
		defPBState_SlowPlay1,			// 慢放1级
		defPBState_SlowPlay2,			// 慢放2级
		defPBState_StopAll	= 99
	};

public:
	XmppGSPlayback( const struTagParam &TagParam, uint32_t camera_id, const std::string &url, const std::string &peerid, const std::string &streamid, const std::string &key, defPBState state=defPBState_Unknown, uint32_t startdt=0, uint32_t enddt=0, int m_sound=-1 );
	XmppGSPlayback( const Tag* tag );
	~XmppGSPlayback(void);

	static std::string TransState2Str( const defPBState state );

	// reimplemented from StanzaExtension
	virtual const std::string& filterString() const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* newInstance( const Tag* tag ) const
	{
		return new XmppGSPlayback( tag );
	}

	void PrintTag( const Tag* tag, const Stanza *stanza ) const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* clone() const
	{
		XmppGSPlayback *pnew = new XmppGSPlayback(NULL);
		*pnew = *this;
		return pnew;
	}

	// reimplemented from StanzaExt
	virtual Tag* tag() const;

	uint32_t get_camera_id() const
	{
		return m_camera_id;
	}

	uint32_t get_startdt() const
	{
		return m_startdt;
	}

	uint32_t get_enddt() const
	{
		return m_enddt;
	}

	const std::string& get_url() const
	{
		return m_url;
	}

	const std::string& get_peerid() const
	{
		return m_peerid;
	}

	const std::string& get_streamid() const
	{
		return m_streamid;
	}

	const std::vector<std::string>& get_url_backup() const
	{
		return this->m_url_backup;
	}

	const std::string& get_key() const
	{
		return m_key;
	}

	defPBState get_state() const
	{
		return m_state;
	}

	int get_sound() const
	{
		return m_sound;
	}

private:
	struTagParam m_TagParam;

	uint32_t m_camera_id;	// [in/out]
	uint32_t m_startdt;		// [in] 开始时间  UTC本地时间精确到秒
	uint32_t m_enddt;		// [in] 结束时间   UTC本地时间精确到秒
	std::string m_url;		// [in/out]地址
	std::string m_peerid;	// [out]peerid
	std::string m_streamid;	// [out]streamid
	std::vector<std::string> m_url_backup; // [in]备份地址
	std::string m_key;		// [in/out]密钥，心跳请求必须，服务端生成
	defPBState m_state;		// [in] 传输状态  start/stop
	int m_sound;			// [in] 音频播放标志
};

