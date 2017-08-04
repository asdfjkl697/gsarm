#ifndef _CLIENTS_HANDLER_H
#define _CLIENTS_HANDLER_H

class ClientsHandler
{
	public:
		virtual void Send(serial_packet *pkt) = 0;
};
#endif
