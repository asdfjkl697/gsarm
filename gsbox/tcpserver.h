#ifndef _TCPSERVER_H_
#define _TCPSERVER_H_

#include "typedef.h"
#include "netpacket.h"
#include "tcpservertask.h"
#include "boardhandler.h"
#include "clientshandler.h"
#include "radionetwork.h"
#include <list>

class TcpServer:public TCPServerTask, public ClientsHandler
{
	private:
		uint16_t _port;
		bool logged;
		char username[16];
	    	char password[16];
		BoardHandler *boardhandler;
		RadioNetwork *radio;
		std::list<Client *> clients;

	private:
		void onPacketRead(Client *client, NetPacket *pkt);
		bool CheckLogin(const char *user, const char *pass);
		
	public:
		TcpServer(uint16_t port);
		~TcpServer(void);

		void SetRadio(RadioNetwork *rd){ radio = rd; };
		void Run(){};
		void Send(serial_packet *ser_pkt);
		void AddBoardHandler(BoardHandler *handler){ boardhandler = handler; };
        	void OnAccept(Client *client);
		void OnDisconnect(Client *client);
        	void OnDataRead(Client *client, uint8_t *data, int size);
		uint16_t GetPort() { return _port; };
};

#endif
