#include "NetModule.h"

CNetMan     *g_NetMan = NULL;

bool NetModule_Init()
{
	if( !g_NetMan )
	{
		g_NetMan = new CNetMan();
		return ( g_NetMan ? true:false );
	}

	return false;
}

void NetModule_UnInit()
{
	if( g_NetMan )
	{
		delete g_NetMan;
		g_NetMan = NULL;
	}
}

void *NetModule_IsOpen(const char* ip, int port, NET_PROTOCOL netType)			
{
	void *p = NULL;

	try
	{
		p = g_NetMan->IsCreateClient(ip, port, netType);
	}
	catch(...)
	{
		if(p) NetModule_Close(p);
	}

	return p;
}

void *NetModule_Open(const char* ip, int port, NET_PROTOCOL netType)			
{
	void *p = NULL;
	
	try
	{
		p = g_NetMan->CreateClient(ip, port, netType);
	}
	catch(...)
	{
		if(p) NetModule_Close(p);
	}

	return p;
}

bool NetModule_Close(void* Device)
{
	if(!g_NetMan) return 0;
	if(!Device) return false;

	BOOL rez  = false;
	
	try
	{
		rez = g_NetMan->CloseClient(Device);
	}
	catch(...)
	{
	}

	if(rez)
	{
		return true;
	}
	else
	{
		return false;
	}

}

int	NetModule_Write(void* Device,void* buffer,unsigned long buflen)
{
	if(!g_NetMan) return 0;
	if(!Device) return 0;

	int res = 0;
	
	try
	{
		res = g_NetMan->Write(Device,buffer,buflen);
	}
	catch(...)
	{
	}

	//if(res < 0) res =0;

	return res;
}

int	NetModule_Read(void* Device,void* buffer,unsigned long buflen)
{
	if(!g_NetMan) return 0;
	if(!Device) return 0;

	int res = 0;
	
	try
	{
		res = g_NetMan->Read(Device,buffer,buflen);
	}
	catch(...)
	{
	}
	
	//if(res < 0) res =0;
	
	return res;
}

bool NetModule_TCPConnectTest( const char* ip, int port )
{
	bool ret = false;

	try
	{
		ret = CNetMan::TCPConnectTest(ip, port);
	}
	catch(...)
	{
	}

	return ret;
}
