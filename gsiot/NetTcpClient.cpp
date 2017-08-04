#include "NetTcpClient.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CNetTcpClient::CNetTcpClient()
{

}

CNetTcpClient::~CNetTcpClient()
{

}

SOCKET	CNetTcpClient::CreateSocket( const char *targetIp, long targetPort, long secondtime )
{
	if(-1 == (m_Socket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP))) 
	{   
		return INVALID_SOCKET;
	} 

	if(!this->SetNoBlock())
	{
		//jyc20160823
		//closesocket(m_Socket);
		close(m_Socket);
		m_Socket = INVALID_SOCKET;
		return m_Socket;
	}
		m_TargetAddr.sin_family =AF_INET;
	m_TargetAddr.sin_port =htons(targetPort);	m_TargetAddr.sin_addr.s_addr=inet_addr(targetIp);

	if(-1 == connect(m_Socket,(struct sockaddr*)&m_TargetAddr,sizeof(struct sockaddr)))
    {
        printf("connect error\n");
        //exit(0);
		return INVALID_SOCKET;
    }
	//return m_Socket; //jyc20160829 test

	unsigned long error = 0;	if(this->IsConnectOK(secondtime,1,error)) //jyc20160826	{	
		return m_Socket;	//与服务器建立连接 
	}

	close(m_Socket);
	m_Socket = INVALID_SOCKET;	
	return INVALID_SOCKET;
}

/*
SOCKET	CNetTcpClient::CreateSocket( const char *targetIp, long targetPort, long secondtime )
{
	//if((m_Socket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP))==SOCKET_ERROR)   //jyc20160823
	if((m_Socket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP))<0) 
	{   
		return INVALID_SOCKET;
	} 

	if(!this->SetNoBlock())
	{
		//jyc20160823
		//closesocket(m_Socket);
		close(m_Socket);

		m_Socket = INVALID_SOCKET;
		
		return m_Socket;
	}
	
	m_TargetAddr.sin_family =AF_INET;
	
	m_TargetAddr.sin_port =htons(targetPort);
	
	m_TargetAddr.sin_addr .s_addr=inet_addr(targetIp);

	//jyc20160823
	//if(::connect(m_Socket,(LPSOCKADDR)&m_TargetAddr,sizeof(SOCKADDR_IN)) != SOCKET_ERROR)
	//{		
	//	return m_Socket;
	//}

	unsigned long error = 0;

	if(this->IsConnectOK(secondtime,1,error))
	{	
		return m_Socket;	//与服务器建立连接 
	}

	//jyc20160823
	//closesocket(m_Socket);
	close(m_Socket);
	
	m_Socket = INVALID_SOCKET;	
	return INVALID_SOCKET;
}*/

long	CNetTcpClient::SndData(const char *buf,long dataLen)
{
	if(m_Socket == INVALID_SOCKET)	return -1;
	
	if(!buf || dataLen < 1)			return 0;

	int flags = 0;

	int rez= this->DoSend(buf, dataLen, flags,1000);//SENDTIMEOUT

	return rez;
}

long	CNetTcpClient::RcvData(char *buf,long len)
{
	if(m_Socket == INVALID_SOCKET)		return -1;
	
	const int iscan = this->IsCanRecv(0,1);
	if( iscan<=0 )	return iscan;

	//const int recvlen = recv(m_Socket,buf,len,0);
	const int recvlen = recv(m_Socket,buf,len,MSG_DONTWAIT);
	/*jyc20160824*/
	if( recvlen == SOCKET_ERROR )
	{
		//int Error = WSAGetLastError();
		//printf("rec err...\n"); //jyc20160913 test
		
	}//*/

	if(recvlen < 0)
		return 0;

	return recvlen;
}

bool CNetTcpClient::IsConnectOK(long sec,long usec,unsigned long &error)
{
	struct timeval to;   
	fd_set fdsWrite, fdsExcept;   
	to.tv_sec = sec; 
	to.tv_usec = usec;	 
	
	FD_ZERO(&fdsExcept);
	FD_ZERO(&fdsWrite);   
	FD_SET(m_Socket, &fdsWrite);   
	FD_SET(m_Socket, &fdsExcept);   
	
	int nfds = 0;
#ifndef _WIN32
	nfds = m_Socket + 1;
#endif
	/* jyc20160826 */
	if(select(nfds, 0, &fdsWrite, &fdsExcept, &to) > 0 ) 
	{
		//int len = sizeof(unsigned long);
		socklen_t len = sizeof(unsigned long);
		
		getsockopt(m_Socket, 
		           SOL_SOCKET, SO_ERROR, 
		           (char*)&error, 
		           &len);
		
		if(error == 0) return true;
	}
	else
	{
		//error = WSAGetLastError();//WSAETIMEDOUT;
		return false;
	}//*/
	//return  false;
	//*/
	//return true;
}

long	CNetTcpClient::Snd(const char *buf,long dataLen,int nFlags)
{
	return ::send(m_Socket,buf,dataLen,nFlags);
}
