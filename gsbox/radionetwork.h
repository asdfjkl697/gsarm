#ifndef _RADIO_NETWORK_H
#define _RADIO_NETWORK_H

#include "packet.h"
#include "client.h"
#include "timerhandler.h"
#include "boardhandler.h"
#include "clientshandler.h"
#include "taskmanager.h"
#include "netpacket.h"
#include "radiopacket.h"
#include "radiosynchandler.h"
#include <list>

typedef struct _radio_child
{
	uint16_t id;
	uint8_t timeout_count;
	uint8_t state;
	uint8_t addr;
	uint8_t channel;
	uint8_t role;
	uint8_t recv_rssi;
	uint8_t send_rssi;
	uint8_t ipver;
	uint8_t mac[20]; 
	struct _radio_child *child;
	struct _radio_child *next;
}RADIO_CHILD;

class RadioNetwork:public TimerHandler,public ClientsHandler, RadioTimerHandler, RadioDataHandler
{
	private:
		uint16_t id;
		uint16_t child_id;
		uint8_t state;
		uint8_t addr;
		uint8_t channel;
		uint8_t role;
		uint8_t dis_read;
		uint8_t dis_count;
		uint8_t dis_addr;
		uint8_t dis_channel;
		uint8_t dis_state;
		uint8_t dis_level;
		uint8_t dis_id;
		uint8_t recv_rssi;
		uint8_t hb_count;
		uint8_t reg_addr;
		uint8_t reg_channel;
		uint8_t *mac;
        RadioDataHandler *rdHandler;
		bool _hasheatbeart;
		RADIO_CHILD *child;
		BoardHandler *boardhandler;
		TaskManager *task;
		RadioSyncHandler *syncHandler;
		RadioPacket *lastpacket;
 		
	public:
		RadioNetwork(BoardHandler *board, TaskManager *tm);
		~RadioNetwork(void);

		void OnNetData(Client *client, NetPacket *pkt);
		void SetMac(uint8_t *mac_ptr){ mac = mac_ptr; };
		void Send(serial_packet *ser_pkt);
		void onRadioConfigRead(Packet *pkt, uint16_t rid, uint8_t level);
		void onDiscovery(uint8_t code);
		void onDiscoveryChild(uint8_t code, uint16_t rid, uint8_t level);
		void onRadioComData(Packet *pkt, uint16_t rid, uint8_t level);
		void OnTimer();
		void AsyncDiscoveryCall();
		void AsyncChildDiscoveryCall();
		void RadioDiscoveryCall();
		void AsyncPrecessPacketCall();
		void AsyncComDataCall();

	private:
		void loadConfig();
		bool saveConfig();
		void saveXml();
		void readConfig(RadioDataHandler *handler, Packet *pkt, uint16_t parent_id, uint16_t level);

		void setAddrFreq(uint8_t s_role,uint8_t s_addr, uint8_t s_channel);

		uint8_t updateChild(int rid);
		void writeChild(Packet *pkt, RADIO_CHILD *cl);
		RADIO_CHILD* getChild(RADIO_CHILD *child, uint16_t rid, uint16_t level);
		RADIO_CHILD* findChildDevice(uint16_t parent_id, uint16_t level);
		void cleanChild();
		void removeChild(RADIO_CHILD *child);
		uint8_t getChildAddr(uint8_t rm_addr);
		
        void addOrUpdateChild(RADIO_CHILD *child);		
		bool addRadio(NetPacket *pkt);
		bool editRadio(uint16_t rid, NetPacket *pkt);
		bool deleteRadio(uint16_t rid);

		bool sendDataToDevice(RadioDataHandler *handler, Packet *pkt, int length, uint16_t parent_id, uint16_t level);
		void sendRadio(RadioPacket *rd_pkt);
		void discoverySend();
		uint8_t childDiscovery(RadioDataHandler *handler, uint16_t sid, uint16_t rid, uint16_t level);
		uint8_t discovery(RadioDataHandler *handler);
		void sendDiscoveryToDevice(RADIO_CHILD *child, uint16_t sid, uint16_t rid, uint8_t level);
		void sendGetChildToDevice(RADIO_CHILD *child, uint16_t rid, uint8_t level);
		void discoveryCallback(bool child, uint8_t code, uint16_t rid, uint8_t level);

		void headBeart();
		void stopHeadBeart();

		void onRadioPacket(RadioPacket *pkt);
		void initLastPacket();
};

#endif
