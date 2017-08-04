#ifndef _OBSOLETE_H
#define _OBSOLETE_H

#include "typedef.h"
#include "packet.h"
#include "tcpservertask.h"
#include "boardhandler.h"
#include "clientshandler.h"
#include "taskmanager.h"
#include <list>

class ObsoleteServer:public TCPServerTask, public ClientsHandler, public TimerHandler
{
	private:
		uint16_t _port;		
        	int receiver_freq;
		uint8_t receiver_mode;
        	int transmit_freq;
		serial_packet *prve_packet;
		BoardHandler *boardhandler;
		std::list<Client *> clients;
		TaskManager *_task;
		
	public:
		ObsoleteServer(uint16_t port);
		~ObsoleteServer(void);

		void Run(){};
		void Send(serial_packet *ser_pkt);
		void AddBoardHandler(BoardHandler *handler){ boardhandler = handler; };
        	void OnAccept(Client *client);
		void OnDisconnect(Client *client);
        	void OnDataRead(Client *client, uint8_t *data, int size);
		uint16_t GetPort() { return _port; };

		void OnTimer();

		void AddTask(TaskManager *task){ _task = task; };

	private:
		void getFreq();	
		void setfreq(uint8_t module, uint32_t freq);
		void setrecmode(uint8_t module, uint8_t recmode);
		void transform(serial_packet *pkt);
};

#endif
