#pragma once
#include "NetBaseClient.h"

class CNetTcpClient   :public CNetBaseClient
{
public:
	CNetTcpClient();
	virtual ~CNetTcpClient();

public:
	
	SOCKET	CreateSocket( const char *targetIp, long targetPort, long secondtime=5 );
		
	long	SndData(const char *buf,long dataLen);
	
	long	RcvData(char *buf,long len);

protected:

	long	Snd(const char *buf,long dataLen,int nFlags);

private:

	bool	IsConnectOK(long sec,long usec,unsigned long &error);


private:
};

