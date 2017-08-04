#ifndef __SOCKET_TASK_H
#define __SOCKET_TASK_H

#include <stdint.h>
#include "task.h"

class SocketTask :public Task
	{
	private:
		struct event *_socket_event;

	public:
		void SetEvent(struct event *socket_event){ _socket_event = socket_event; };
		struct event *GetEvent(){ return _socket_event; };
		virtual void PacketRead(uint8_t *data, int size, struct sockaddr_in *addr) = 0;
		virtual SOCKET GetSocket() = 0;
};

#endif

