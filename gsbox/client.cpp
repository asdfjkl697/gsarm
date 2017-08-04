#include "client.h"
#include <stdlib.h>
#include <string.h>
#include "event2/bufferevent.h"
#include "event2/buffer.h"

Client::Client(void *task)
:_task(task), _login(false)
{
	memset(&prve_packet, 0, sizeof(serial_packet));
}

Client::~Client(void)
{
}

void Client::Send(uint8_t *data, int size)
{
    bufferevent_write(_bev, data, size);
}


void Client::onRadioComData(Packet *pkt, uint16_t rid, uint8_t level)
{
	NetPacket nt_pkt;
	nt_pkt.SetId(0);
	nt_pkt.SetCommand(CMD_RADIONETWORK);
	nt_pkt.SetAck(ACK_NOP);
	nt_pkt.WriteByte(0xdd);
	nt_pkt.EncodeInt16(rid);
	nt_pkt.EncodeInt16(level);
	pkt->ReadPacket(&nt_pkt);
	nt_pkt.EncodeBuffer();
	Send(nt_pkt.getBuffer(), nt_pkt.getLength());
}

void Client::onRadioConfigRead(Packet *pkt, uint16_t rid, uint8_t level)
{	
	NetPacket nt_pkt;
	nt_pkt.SetId(0);
	nt_pkt.SetCommand(CMD_RADIONETWORK);
	nt_pkt.SetAck(ACK_NOP);
	nt_pkt.WriteByte(0x4);
	nt_pkt.WriteByte(0);
	nt_pkt.EncodeInt16(rid);
	nt_pkt.EncodeInt16(level);
	pkt->ReadPacket(&nt_pkt);	
	nt_pkt.EncodeBuffer();
	Send(nt_pkt.getBuffer(), nt_pkt.getLength());
}

void Client::onDiscovery(uint8_t code)
{	
	NetPacket pkt(32);
	pkt.SetId(0);
	pkt.SetCommand(CMD_RADIONETWORK);
	pkt.SetAck(ACK_NOP);
	pkt.WriteByte(0x3);
    pkt.EncodeByte(code);
	pkt.EncodeBuffer();
	Send(pkt.getBuffer(), pkt.getLength());
}

void Client::onDiscoveryChild(uint8_t code, uint16_t rid, uint8_t level)
{
	NetPacket pkt(32);
	pkt.SetId(0);
	pkt.SetCommand(CMD_RADIONETWORK);
	pkt.SetAck(ACK_NOP);
	pkt.WriteByte(0x5);
	pkt.WriteInt16(rid);
	pkt.WriteByte(level);
    pkt.EncodeByte(code);
	pkt.EncodeBuffer();
	Send(pkt.getBuffer(), pkt.getLength());
}
