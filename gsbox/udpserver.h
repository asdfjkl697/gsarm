#ifndef _UDPSERVER_H
#define _UDPSERVER_H

#include "sockettask.h"

typedef struct _network_adapter
{
	uint8_t ipaddr[18];
	uint8_t netmask[18];
	uint8_t gateway[18];
	uint8_t dns[18];
	uint8_t mac[20];
	uint8_t sn[30]; //jyc20160711
}NETWORK_ADAPTER;

class UDPServer:public SocketTask
{
	private:
		uint16_t _port;
		SOCKET socket_fd;
		NETWORK_ADAPTER adapter;
	    NETWORK_ADAPTER adapter2;
		uint8_t *_version;
		
	public:
		UDPServer(uint16_t port);
		~UDPServer(void);

		void Run(){};
		bool Open();
		int Send(struct sockaddr_in *addr, char *buffer,int size);
		int Read(struct sockaddr_in *addr, char *buffer,int size);
		void Close();

		void SetVerion(uint8_t *ver){ _version = ver; };
		uint8_t *GetMacPtr(){ return adapter.mac; };

		void PacketRead(uint8_t *data, int size, struct sockaddr_in *addr);
		SOCKET GetSocket(){ return socket_fd; };
};

#endif
