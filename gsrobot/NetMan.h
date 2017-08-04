#pragma once

#include "NetBaseClient.h"
class CNetBaseClient;

class CNetMan  
{
public:
	CNetMan();
	virtual ~CNetMan();

public:

	static bool	TCPConnectTest( const char* ip, int port, long secondtime=3 );

	void*	IsCreateClient(const char* ip, int port, NET_PROTOCOL netType=NET_PROTOCOL_TCP);

	void*	CreateClient(const char* ip, int port, NET_PROTOCOL netType=NET_PROTOCOL_TCP);

	BOOL	CloseClient(void* Device);

	int		Write(void* Device,void* buffer,unsigned long buflen);

	int		Read(void* Device,void* buffer,unsigned long buflen);

private:

	std::map<SOCKET,CNetBaseClient* > m_mapNetClient;

	std::map<std::string,SOCKET>		   m_mapClientInfo;

	gloox::util::Mutex					   m_mutex;
};

