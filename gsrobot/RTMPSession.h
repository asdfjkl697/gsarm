#ifndef RTMPSESSIONMANAGER_H_
#define RTMPSESSIONMANAGER_H_

#include "gloox/jid.h"
#include <list>
#include <stdint.h>

#include "common.h"

using namespace gloox;

class RTMPSession
{
private:
	uint32_t m_start_time;
	JID m_jid;
    uint32_t timestamp;

public:
	RTMPSession(const JID& jid);
	~RTMPSession(void);

	const JID& GetJID() const
	{
		return this->m_jid;
	}

	uint32_t get_start_time() const
	{
		return this->m_start_time;
	}

	uint32_t get_timestamp() const
	{
		return this->timestamp;
	}

	void Update();
	bool Check();
};

struct RTMPSessionInfo
{
	JID jid;
	uint32_t timestamp;

	RTMPSessionInfo()
		:jid()
	{};

	RTMPSessionInfo( RTMPSession *Session )
		:jid( Session->GetJID() ), timestamp( Session->get_timestamp() )
	{};
};
typedef std::list<RTMPSessionInfo> deflstRTMPSessionInfo;

#endif

