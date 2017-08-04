#include "netpacket.h"

NetPacket::NetPacket(void)
:_id(0), Packet()
{
	data_ptr = buffer+18;
}

NetPacket::NetPacket(int size)
:_id(0), Packet(size+18)
{
	data_ptr = buffer+18;
}

NetPacket::NetPacket(uint8_t *data, int size)
:_id(0), Packet(data, size)
{
}

NetPacket::~NetPacket(void)
{
	
}

int32_t NetPacket::data2Int()
{
	int32_t value = ((data_ptr[0] << 24) | (data_ptr[1] << 16) | (data_ptr[2] << 8) | (data_ptr[3] & 0xFF));
	data_ptr += 4;
	_size-=4;
	return value;
}


void NetPacket::int2Data(uint32_t value)
{
	data_ptr[0] = (value >> 24);
	data_ptr[1] = (value >> 16);
	data_ptr[2] = (value >> 8);
	data_ptr[3] = value & 0xFF;
	data_ptr += 4;
	_size += 4;
}


bool NetPacket::Decode()
{	
	if(_size < 16) return false;
	
	this->_flag = data2Int();
	this->_version = ReadInt32();

	if(this->_flag != PROTOCOL_FLAG) return false;
	
	this->_id = ReadInt32();
	
	this->_command = ReadByte();
	this->_ack = ReadByte();
	this->_length = ReadInt32();
	
	return true;
}

void NetPacket::EncodeBuffer()
{	
	data_ptr = buffer;
	this->_length = this->_size;
	
	int2Data(PROTOCOL_FLAG);
	WriteInt32(PROTOCOL_VERSION);
	WriteInt32(this->_id);
	WriteByte(this->_command);
	WriteByte(this->_ack);
	WriteInt32(this->_length);
}


void NetPacket::SetDataPos(int size)
{
	data_ptr = buffer + 18 + size;
	_size = size;
}

void NetPacket::ResetToSend()
{
	data_ptr = buffer + 18;
	_size = 0;
}

void NetPacket::AutoAck()
{
	if(_ack == ACK_ASK){
	    _ack = ACK_REPLY;
	}else{
		_ack = ACK_NOP;
	}
}

