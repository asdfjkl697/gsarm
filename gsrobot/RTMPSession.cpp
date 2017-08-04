#include "RTMPSession.h"
//#include <Windows.h>

RTMPSession::RTMPSession(const JID& jid)
	:m_jid(jid)
{
	this->timestamp = timeGetTime();
	this->m_start_time = this->timestamp;
}


RTMPSession::~RTMPSession(void)
{
}

void RTMPSession::Update()
{
	this->timestamp = timeGetTime();
}

bool RTMPSession::Check()
{
   uint32_t expired = timeGetTime() - timestamp;
   if(expired / 1000 > 30){
       return false;
   }
   return true;
}
