#include "radiopacket.h"

RadioPacket::RadioPacket(void)
:index(0), done(false), Packet()
{
	
}

RadioPacket::RadioPacket(uint8_t addr, uint8_t cmd)
:index(0), done(false),_addr(addr), _cmd(cmd), Packet(32)
{
}

RadioPacket::RadioPacket(serial_packet *pkt)
:index(0), done(false), Packet(pkt->data+2, pkt->read_size-2)
{
}

RadioPacket::~RadioPacket(void)
{
}

void RadioPacket::WriteIndex(uint8_t idx)
{
	WriteByte(idx);
}

bool RadioPacket::CompareExt(uint8_t idx, uint8_t cmd)
{
	if((index > 0) && (idx == 0) && (cmd == _cmd)){
		done = true;
		return true;
	}
	
	return ((idx == (++index)) && cmd == _cmd);
}

bool RadioPacket::Reset()
{
	index = 0;
	done  = false;
}

bool RadioPacket::Decode()
{
	if(getLength() < 6) return false;
	
	_addr = ReadByte();
	_cmd  = ReadByte();

	SetPos(getLength());

	_rssi = ReadByte();
	_lqi  = ReadByte();

	//back to data
	SetPos(2);

	return true;
}
