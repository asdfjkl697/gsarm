#ifndef _HARDWARE_H
#define _HARDWARE_H

#include "iotask.h"
#include "packet.h"
#include "boardhandler.h"
#include "timerhandler.h" //jyc20160719
#include "clientshandler.h"
#include <list>

class Hardware: public IOTask, public BoardHandler
{
private:
	serial_packet prve_packet;
	uint8_t soft_version[20];
	uint8_t cache[255];
	int cache_length;
	std::list<ClientsHandler *> clients; 

public:
	Hardware(void);
	~Hardware(void);

	void OnEvent();
	int GetFD();
	bool Open();
	void Close();

	int Send(uint8_t *data, int size);

	void RemoveClientsHandler(ClientsHandler *handler);
	void AddClientsHandler(ClientsHandler *handler);

	uint8_t *GetVersion(){ return soft_version; };
	
private:
	void onPacketRead(serial_packet *data);
	void GetSoftVersion();
};

#endif
