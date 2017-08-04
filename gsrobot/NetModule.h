#include "NetMan.h"

bool NetModule_Init();
void NetModule_UnInit();

void *NetModule_IsOpen(const char* ip, int port, NET_PROTOCOL netType=NET_PROTOCOL_TCP);

void *NetModule_Open(const char* ip, int port, NET_PROTOCOL netType=NET_PROTOCOL_TCP);

bool NetModule_Close(void* Device);

int	NetModule_Write(void* Device,void* buffer,unsigned long buflen);

int	NetModule_Read(void* Device,void* buffer,unsigned long buflen);

bool NetModule_TCPConnectTest( const char* ip, int port );

