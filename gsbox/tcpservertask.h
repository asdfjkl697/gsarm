#ifndef _TCPSERVER_TASK_H
#define _TCPSERVER_TASK_H

#include <stdint.h>
#include "client.h"

class TCPServerTask :public Task
{
	private:
	    struct evconnlistener *_listener;

	public:
		void SetListener(struct evconnlistener *listener){ _listener = listener; };
		struct evconnlistener *GetListener(){ return _listener; };
        virtual void OnAccept(Client *client) = 0;
		virtual void OnDisconnect(Client *client) = 0;
        virtual void OnDataRead(Client *client, uint8_t *data, int size) = 0;
		virtual uint16_t GetPort() = 0;
};

#endif