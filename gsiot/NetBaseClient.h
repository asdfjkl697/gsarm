#pragma once
//#include <WinSock2.h>
//#include <windows.h>
//#include "common.h"
//#include "..//gloox-1.0//src//mutexguard.h"

#include <netinet/in.h>  
#include <sys/socket.h> 
#include <netdb.h>   
#include <arpa/inet.h>  

#include "common.h"
#include "gloox/mutexguard.h"




enum NET_PROTOCOL
{
	NET_PROTOCOL_UNKNOW = 0,

	NET_PROTOCOL_TCP=1,	// TCP 协议

	NET_PROTOCOL_UDP	// UDP 协议
};

class CNetBaseClient  
{
public:
	CNetBaseClient();
	virtual ~CNetBaseClient();

public:
	//jyc20160824 modify
	//virtual SOCKET	CreateSocket( const char *targetIp, long targetPort, long secondtime=5 ) = NULL;
	virtual SOCKET	CreateSocket( const char *targetIp, long targetPort, long secondtime=5 )= 0;
		
	virtual long	SndData(const char *buf,long dataLen) = 0;
	
	virtual long	RcvData(char *buf,long len) = 0;

	SOCKET  GetSocket();

	std::string  GetNetParam() {return m_NetParam;}
protected:

	BOOL	SetNoBlock();

	int		IsCanRecv(long sec,long usec);

	bool	IsCanSend(long sec,long usec);

	int		DoSend(const void* lpBuf, int nBufLen, int nFlags, DWORD timeout);

	virtual long Snd(const char *buf,long dataLen,int nFlags) = 0;

protected:

	SOCKADDR_IN  m_TargetAddr;	
	
	SOCKET       m_Socket;
	
public:

	std::string		 m_NetParam;
};

