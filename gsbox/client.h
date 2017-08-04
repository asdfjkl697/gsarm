#ifndef _CLIENT_H
#define _CLIENT_H

#include "task.h"
#include "netpacket.h"
#include "radiohandler.h"

class Client:public RadioDataHandler
{
	private:
		bool _login;
		void *_task;
		serial_packet prve_packet;
		struct bufferevent *_bev;
		
	public:
		Client(void *task);
		~Client(void);
		
		void SetEvent(struct bufferevent *bev){_bev = bev;};
		void *GetTask(){ return _task; };
		void Send(uint8_t *data, int size);
		bool IsLogin(){ return _login; };
		void SetLogin(bool isLogin){ _login = isLogin; };
		serial_packet *GetPrvePkt(){ return &prve_packet; };

		void onRadioComData(Packet *pkt, uint16_t rid, uint8_t level);
		void onRadioConfigRead(Packet *pkt, uint16_t rid, uint8_t level);
		void onDiscovery(uint8_t code);
		void onDiscoveryChild(uint8_t code, uint16_t rid, uint8_t level);
};

#endif
