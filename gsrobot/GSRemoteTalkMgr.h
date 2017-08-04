#pragma once
#include "common.h"
#include <string>
#include <map>
#include "gloox/gloox.h"
#include "gloox/util.h"
#include "gloox/mutexguard.h"
#include "xmpp/XmppGSTalk.h"
#include "gsiot/GSIOTDevice.h"

#define MAX_TALK			16
#define MAX_TalkQueueSize	99

using namespace gloox;

// ��ͨ������
struct struNewPlay_Param
{
	unsigned long ts; // ʱ���
	unsigned long key;
	XmppGSTalk::defTalkCmd cmd;
	std::string url;
	std::string from_Jid;
	std::string from_id;
	bool isOnlyOneTalk;
	defvecDevKey vecdev;

	struNewPlay_Param( XmppGSTalk::defTalkCmd in_cmd, const std::string &in_url, const std::string &in_from_Jid, const std::string &in_from_id, bool in_isOnlyOneTalk, const defvecDevKey &in_vecdev )
		: key(0), cmd(in_cmd), url(in_url), from_Jid(in_from_Jid), from_id(in_from_id), isOnlyOneTalk(in_isOnlyOneTalk), vecdev(in_vecdev)
	{
		settick();
	}

	struNewPlay_Param()
		: key(0), cmd(XmppGSTalk::defTalkCmd_Unknown), isOnlyOneTalk(false)
	{
		settick();
	}

	void settick();
};
typedef std::vector<struNewPlay_Param> defvecTalkParam;

// ͨ��״̬��¼
struct struTalkState
{
	unsigned long key;
	unsigned long ts; // ʱ������޸�playʱ����
	bool play;
	bool isOnlyOneTalk;
	std::string from_Jid;
	std::string from_id;

	unsigned long IsPlaying_ts; // ʱ���������������
	unsigned long keepalive_ts; // keepaliveʱ���
	defvecTalkParam vecTalkParam; // ����������/�Ƴ�Խ��豸

	struTalkState( unsigned long in_key, bool in_play, bool in_isOnlyOneTalk, const std::string &in_from_Jid, const std::string &in_from_id )
		: key(in_key), isOnlyOneTalk(in_isOnlyOneTalk), from_Jid(in_from_Jid), from_id(in_from_id)
	{
		set_play(in_play);
	}

	struTalkState()
		: key(0), isOnlyOneTalk(false)
	{
		set_play(false);
	}

	void set_play( bool in_play );
	void set_IsPlaying_ts();
};

typedef std::map<std::string, struTalkState> defmapTalk; // <url,key>

class ITalkNotify
{
public:
	virtual void* OnTalkNotify( const XmppGSTalk::defTalkCmd cmd, const std::string &url, const std::string &from_Jid, const std::string &from_id, bool isSyncReturn, const defvecDevKey &vecdev, bool result=true, IOTDeviceType getdev_type=IOT_DEVICE_Unknown, int getdev_id=0 ) = 0;
};

/*
���Ź���

*/
class GSRemoteTalkMgr
{
public:
	GSRemoteTalkMgr(void);
	~GSRemoteTalkMgr(void);

	static std::string GetStateInfoForMap( const defmapTalk &mapTalk );
	static void GetCountInfoForMap( const defmapTalk &mapTalk, unsigned long &QueueCount, unsigned long &PlayCount, bool &isOnlyOneTalk );

	void set_ITalkNotify( ITalkNotify *cb )
	{
		m_ITalkNotify = cb;
	}

	defGSReturn StartTalk( const std::string &url, const std::string &from_Jid, const std::string &from_id, const defvecDevKey &vecdev, const bool doforce=true, const bool isOnlyOneTalk=true );
	defGSReturn AdddevTalk( const std::string &url, const std::string &from_Jid, const std::string &from_id, const defvecDevKey &vecdev );
	defGSReturn RemovedevTalk( const std::string &url, const std::string &from_Jid, const std::string &from_id, const defvecDevKey &vecdev );
	bool StopTalk( const std::string &url );
	bool StopTalk_All();
	bool StopTalk_AllButOneTalk();
	bool StopTalk_AnyOne();
	bool isPlaying_s()
	{
		bool isOnlyOneTalk = false;
		return isPlaying( isOnlyOneTalk );
	}
	bool isPlaying( bool &isOnlyOneTalk );
	bool isPlaying_url( const std::string &url, bool &isOnlyOneTalk, bool iskeepalive=false );

	bool UrlKey_push_cmd( const std::string &url, const struNewPlay_Param &param );

	void GetCountInfo( unsigned long &QueueCount, unsigned long &PlayCount, bool &isOnlyOneTalk );
	bool CopyMapTalk( defmapTalk &mapTalk );

	void end_OnProc(struNewPlay_Param param); //jyc20170612 add
	void OnProc( const int Play_id );

private:
	ITalkNotify *m_ITalkNotify;

	gloox::util::Mutex m_mutex_TalkMgr;
	unsigned long m_last_checktime;

	// map talk
	defmapTalk m_mapTalk;
	unsigned long m_last_key;
	unsigned long UrlKey_Add_nolock( const struNewPlay_Param &param );
	bool UrlKey_Del( const std::string &url, const unsigned long key );
	bool UrlKey_IsPlaying( const std::string &url, const unsigned long key, defvecTalkParam &vecTalkParam, bool popcmd );
	void UrlKey_Check( bool forceCheck );

	// NewPlay
	struNewPlay_Param m_newPlay_param;
	uint32_t m_ts_newPlay_param;
	bool NewPlay_ParamPush( const struNewPlay_Param &param );
	bool NewPlay_ParamPop( struNewPlay_Param &param );
};

