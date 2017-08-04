#ifndef _RADIO_PACKET_H
#define _RADIO_PACKET_H

#include "packet.h"

class RadioPacket:public Packet
{
	private:
		uint8_t _addr;
	    uint8_t _cmd;
	    uint8_t _rssi;
	    uint8_t _lqi;
		uint8_t index;
		bool done;
		
	public:
		RadioPacket(void);
		RadioPacket(uint8_t addr, uint8_t cmd);
		RadioPacket(serial_packet *pkt);
		~RadioPacket(void);

		uint8_t GetAddr(){ return _addr; };
	    uint8_t GetCmd(){ return _cmd; };
	    uint8_t GetRssi(){ return _rssi; };
	    uint8_t GetRqi(){ return _lqi; };
	    uint8_t GetIndex(){ return ++index; };

		void WriteIndex(uint8_t idx);
		bool IsExPacket(){ return index > 0; };
		uint8_t SetAddr(uint8_t addr){ _addr = addr; };
	    uint8_t SetCmd(uint8_t cmd){ _cmd = cmd; };

		bool CompareExt(uint8_t idx, uint8_t cmd);
		bool IsDone(){ return done; };
		bool Reset();

		bool Decode();
};

#endif
