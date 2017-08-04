#include "NetBaseClient.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CNetBaseClient::CNetBaseClient()
{
	m_Socket = INVALID_SOCKET;

	memset(&m_TargetAddr,0,sizeof(SOCKADDR_IN));
}

CNetBaseClient::~CNetBaseClient()
{
	if(m_Socket != INVALID_SOCKET)
	{
		//closesocket(m_Socket); //jyc20160824
		close(m_Socket);
	}

	m_Socket = INVALID_SOCKET;
}

BOOL CNetBaseClient::SetNoBlock()
{

#ifdef	WIN32
	unsigned long noblocking = 1;
	
	if (SOCKET_ERROR == ioctlsocket(m_Socket, FIONBIO,(unsigned long*)&noblocking))
	{
		closesocket(m_Socket);
		
		m_Socket = INVALID_SOCKET;
		
		return false;
	}
#else

	int var;

	if((var=fcntl(m_Socket, F_GETFL, 0))<0)
	{
		//strerror(errno);
 		//cout<<"fcntl file error."<<endl;
		close(m_Socket);
		return false;
	}
	
	if(SOCKET_ERROR == fcntl(m_Socket,F_SETFL, O_NONBLOCK&var)) //jyc20160829 modify
	{
		close(m_Socket);		
		return false;
	}
	
	if(SOCKET_ERROR == fcntl(m_Socket,F_SETFD, FD_CLOEXEC))
	{
		close(m_Socket);
		return false;
	}
	
#endif	
	
	return true;
}

int CNetBaseClient::IsCanRecv(long sec,long usec)
{
	struct timeval to;   
	fd_set fdsRead;   
	to.tv_sec = sec;   
	to.tv_usec = usec; 
	
	FD_ZERO(&fdsRead);   
	FD_SET(m_Socket, &fdsRead);   
	
	int nfds = 0;
	
	//if(select(nfds, &fdsRead, 0, 0, &to) == SOCKET_ERROR) return -1;
	if(select(nfds, &fdsRead, 0, 0, &to) < 0) return -1;
	
	if(FD_ISSET(m_Socket, &fdsRead))  return 1;
	
	return  0;
}

bool CNetBaseClient::IsCanSend(long sec,long usec)
{
	struct timeval to;   
	
	fd_set fdsWrite;   
	
	to.tv_sec  = sec;   
	
	to.tv_usec = usec;	 
	
	FD_ZERO(&fdsWrite);   
	
	FD_SET(m_Socket, &fdsWrite);   
	
	//	if(select(0, 0, &fdsWrite, 0, &to)>0) return true;
	
	int nfds = 0;
#ifndef _WIN32
	nfds = m_Socket + 1;
#endif
	
	
	//if(select(nfds, 0, &fdsWrite, 0, &to) == SOCKET_ERROR) return false;
	if(select(nfds, 0, &fdsWrite, 0, &to) < 0) return false;
	
    if(FD_ISSET(m_Socket, &fdsWrite))  return true;
	
	return  false;
}

int CNetBaseClient::DoSend(const void* lpBuf, int nBufLen, int nFlags, DWORD timeout)
{
	int nLeft=0, nWritten=-1;
	char* pBuf = (char*)lpBuf;
	nLeft = nBufLen;
	int Error = 0;
	BOOL isDoSend = true;
	
	DWORD dwSendTime = timeGetTime();
	while (nLeft > 0) {
		
DoSend_HERE:			
	if( !isDoSend 
		&& this->IsCanSend(0,1) )
	{
		isDoSend = true;
	}
	
	//while ( isDoSend && (nWritten=this->Snd(pBuf, nLeft,nFlags)) == SOCKET_ERROR ) 
	while ( isDoSend && (nWritten=this->Snd(pBuf, nLeft,nFlags)) == -1 ) 
	{
		
		isDoSend = false;
		
		//Error = WSAGetLastError();

		//if(errno != EAGAIN) //jyc20160824
		if(0)
		{
			return -1;;
		}
		else
		{
			if( (timeGetTime()-dwSendTime) > timeout )
			{
				return nBufLen - nLeft;
			}
			
		}

		nWritten = 0;
		goto DoSend_HERE;
		
		//this->IsCanSend(0,1);//Sleep(1);
	}
	
	if (nWritten > 0) {
		
		nLeft -= nWritten;
		pBuf += nWritten;
		dwSendTime = timeGetTime();
	}
	else if ((timeGetTime()-dwSendTime) > timeout)
	{
		
		return nBufLen - nLeft;
	}
	else
	{
		
		nWritten = 0;
		goto DoSend_HERE;
	}
	}
	
	return nBufLen - nLeft;
}

SOCKET  CNetBaseClient::GetSocket()
{
	return m_Socket;
}