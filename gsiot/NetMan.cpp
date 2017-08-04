#include "NetMan.h"
#include "NetTcpClient.h"
#include "NetUdpClient.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CNetMan::CNetMan()
{
#ifdef _WIN32
	WSADATA   wsaData;   
	WSAStartup(MAKEWORD(2,2),&wsaData);
#endif	

}

CNetMan::~CNetMan()
{
#ifdef WIN32
	WSACleanup( );
#endif 

}

bool CNetMan::TCPConnectTest( const char* ip, int port, long secondtime )
{
	if( !ip || 0==port )
		return false;

	char msg[256];
	memset(msg,0,256);

	snprintf( msg, sizeof(msg), "%s:%d,%d", ip, port, NET_PROTOCOL_TCP );

	CNetTcpClient tcpclient;
	tcpclient.m_NetParam = msg;

	const SOCKET s = tcpclient.CreateSocket( ip, port, secondtime );

	return ( INVALID_SOCKET != s ); 
}

void* CNetMan::IsCreateClient(const char* ip, int port, NET_PROTOCOL netType)
{
	{
		gloox::util::MutexGuard mutexguard( this->m_mutex );

		if( m_mapClientInfo.empty() )
		{
			return NULL;
		}

		char msg[256];
		memset(msg,0,256);
		snprintf( msg, sizeof(msg), "%s:%d,%d", ip, port, netType );

		std::map<std::string,SOCKET>::iterator iter = m_mapClientInfo.find(std::string(msg));

		if(iter != m_mapClientInfo.end())
		{
			std::map<SOCKET,CNetBaseClient* >::iterator iterC = m_mapNetClient.find(iter->second);

			if(iter != m_mapClientInfo.end())
			{
				return ( iterC->second );
			}

			m_mapClientInfo.erase( iter );
		}
	}

	return NULL;
}

void *CNetMan::CreateClient(const char* ip, int port, NET_PROTOCOL netType)
{
	CNetBaseClient *pRet = NULL;
	int CurSockCount_mapNetClient = 0;
	int CurSockCount_mapClientInfo = 0;
	
	char msg[256];
	memset(msg,0,256);
	
	snprintf( msg, sizeof(msg), "%s:%d,%d", ip, port, netType );
	std::string para = msg;

	{
		gloox::util::MutexGuard mutexguard( this->m_mutex );

		std::map<std::string,SOCKET>::iterator iter = m_mapClientInfo.find(para);

		if(iter != m_mapClientInfo.end()) goto End_CreateClient; //ÒÑ´æÔÚ
	}

	switch (netType)
	{
	case NET_PROTOCOL_TCP:
		{
			CNetTcpClient *p = new CNetTcpClient;
			p->m_NetParam = para;
			
			SOCKET s = p->CreateSocket(ip,port);

			if(s == INVALID_SOCKET) 
			{
				delete p;
				p = NULL;

				goto End_CreateClient;
			}

			pRet = p;
			break;
		}
	case NET_PROTOCOL_UDP:
		{
			CNetUdpClient *p = new CNetUdpClient;
			p->m_NetParam = para;
			
			SOCKET s = p->CreateSocket(ip,port);
			
			if(s == INVALID_SOCKET) 
			{
				delete p;
				p = NULL;
				
				goto End_CreateClient;
			}
			
			pRet = p;
			break;
		}
	default:
		goto End_CreateClient;
	}
	
End_CreateClient:
	
	if( pRet )
	{
		{
			gloox::util::MutexGuard mutexguard( this->m_mutex );
			
			m_mapNetClient[pRet->GetSocket()] = pRet;
			
			m_mapClientInfo[para] = pRet->GetSocket();
			
			CurSockCount_mapNetClient = m_mapNetClient.size();
			CurSockCount_mapClientInfo = m_mapClientInfo.size();
		}

		LOGMSG( "NetOpen(%s) success, sock=%d, CurCount(mapNetClient=%d,mapClientInfo=%d)\n",
			para.c_str(), pRet->GetSocket(),
			CurSockCount_mapNetClient, CurSockCount_mapClientInfo );
	}
	else
	{
		LOGMSG( "NetOpen(%s) Failed! \n", para.c_str() );
	}
	
	return (void *)pRet;
}

BOOL	CNetMan::CloseClient(void* Device)
{
	CNetBaseClient *pClient = NULL;
	int CurSockCount_mapNetClient = 0;
	int CurSockCount_mapClientInfo = 0;

	{
		gloox::util::MutexGuard mutexguard( this->m_mutex );
		
		std::map<SOCKET,CNetBaseClient* >::iterator iter = m_mapNetClient.begin();
		
		for (; iter != m_mapNetClient.end(); iter++)
		{
			if(iter->second != (CNetBaseClient*)Device) continue;
			
			pClient = iter->second;
			m_mapNetClient.erase(iter);
			break;
		}
		
		if( !pClient ) return false;

		SOCKET sockId = pClient->GetSocket();

		std::map<std::string,SOCKET>::iterator iterB =  m_mapClientInfo.begin();
		
		for (; iterB != m_mapClientInfo.end(); iterB++)
		{
			if(iterB->second != sockId) continue;
			
			m_mapClientInfo.erase(iterB);
			
			break;
		}

		CurSockCount_mapNetClient = m_mapNetClient.size();
		CurSockCount_mapClientInfo = m_mapClientInfo.size();
	}

   if( pClient )
   {
	   LOGMSG( "NetClose(%s) sock=%d, CurCount(mapNetClient=%d,mapClientInfo=%d)",
		   pClient->m_NetParam.c_str(), pClient->GetSocket(),
		   CurSockCount_mapNetClient, CurSockCount_mapClientInfo );

	   delete pClient;
	   return true;
   }

	return false;
}

int		CNetMan::Write(void* Device,void* buffer,unsigned long buflen)
{
	CNetBaseClient *pClient = NULL;
	
	{
		gloox::util::MutexGuard mutexguard( this->m_mutex );
		
		std::map<SOCKET,CNetBaseClient* >::iterator iter = m_mapNetClient.begin();
		
		for (; iter != m_mapNetClient.end(); iter++)
		{
			if(iter->second != (CNetBaseClient*)Device) continue;
			
			pClient = iter->second;
			break;
		}
	}
	
	if(!pClient) return -1;
	
	return pClient->SndData((const char *)buffer,buflen);
}

int		CNetMan::Read(void* Device,void* buffer,unsigned long buflen)
{
	CNetBaseClient *pClient = NULL;
	
	{
		gloox::util::MutexGuard mutexguard( this->m_mutex );
		
		std::map<SOCKET,CNetBaseClient* >::iterator iter = m_mapNetClient.begin();
		
		for (; iter != m_mapNetClient.end(); iter++)
		{
			if(iter->second != (CNetBaseClient*)Device) continue;
			
			pClient = iter->second;
			break;
		}
	}
	
	if(!pClient) return -1;
	
	return pClient->RcvData((char *)buffer,buflen);
}
