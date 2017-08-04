#include "NetUdpClient.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CNetUdpClient::CNetUdpClient()
{
}

CNetUdpClient::~CNetUdpClient()
{

}

SOCKET	CNetUdpClient::CreateSocket( const char *targetIp, long targetPort, long secondtime )
{
	//if((m_Socket=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP))==SOCKET_ERROR)   
	if((m_Socket=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP))<0) 
	{   
		return INVALID_SOCKET;
	} 

	if(!this->SetNoBlock())
	{
		m_Socket = INVALID_SOCKET;

		return m_Socket;
	}

	m_TargetAddr.sin_family =AF_INET;

	m_TargetAddr.sin_port =htons(targetPort);

	m_TargetAddr.sin_addr .s_addr=inet_addr(targetIp);

	return m_Socket;
}


long	CNetUdpClient::SndData(const char *buf,long dataLen)
{
	if(m_Socket == INVALID_SOCKET) return -1;

	if(!buf || dataLen < 1) return 0;

	int flags = 0;
#ifndef _WIN32
	flags = MSG_NOSIGNAL;
#endif
	
	return this->DoSend(buf, dataLen, flags,1000);//SENDTIMEOUT
}


long	CNetUdpClient::RcvData(char *buf,long len)
{
	if(m_Socket == INVALID_SOCKET)		return -1;

	const int iscan = this->IsCanRecv(0,1);
	if( iscan<=0 )	return iscan;

	SOCKADDR_IN  fromAddr;

	int fromAddrSize = sizeof(fromAddr);

	//jyc20160824
	//return recvfrom(m_Socket, buf, len, 0,(SOCKADDR *)&fromAddr, &fromAddrSize);
	return 0;
}

long	CNetUdpClient::Snd(const char *buf,long dataLen,int nFlags)
{
	return sendto(m_Socket,buf,dataLen,nFlags,(SOCKADDR *)&m_TargetAddr,sizeof(SOCKADDR_IN));
}
