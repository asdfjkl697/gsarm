#ifndef _NET_PACKET_H
#define _NET_PACKET_H

#include "packet.h"

class NetPacket:public Packet
{
	private:		
		int  _flag;
	    int      _version;
	    uint32_t _id;
	    uint8_t  _command;
	    uint8_t  _ack;
	    int  _length;
		
	private:
		void int2Data(uint32_t value);
		int32_t data2Int();
		
	public:
		NetPacket(void);
		NetPacket(int size);
		NetPacket(uint8_t *data, int size);
		~NetPacket(void);
		
		void SetId(uint32_t id){ _id = id;};
		void SetCommand(uint8_t cmd){ _command = cmd;};
		void SetAck(uint8_t ack){ _ack = ack;};
		uint8_t GetAck() { return _ack; };
		uint8_t GetCommand() { return _command; };
		void SetDataPos(int size);
		int GetPos(){ return (data_ptr - buffer)-18; };
		int getDataLength(){ return _length; };
		void ResetToSend();
		void EncodeBuffer();
		void AutoAck();		
		bool Decode();
};

#endif
