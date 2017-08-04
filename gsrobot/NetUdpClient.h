#pragma once
#include "NetBaseClient.h"

class CNetUdpClient  :public CNetBaseClient
{
public:
	CNetUdpClient();
	virtual ~CNetUdpClient();

public:
	
	SOCKET	CreateSocket( const char *targetIp, long targetPort, long secondtime=5 );

	long	SndData(const char *buf,long dataLen);

	long	RcvData(char *buf,long len);
protected:
	
	long	Snd(const char *buf,long dataLen,int nFlags);
};

