#include "packet.h"
#include <string.h>
#include <stdlib.h>

Packet::Packet(void)
:_size(0)
{
	buffer = (uint8_t *)malloc(BUFFER_SIZE);
	data_ptr = this->buffer;
}

Packet::Packet(int size)
:_size(0)
{
	buffer = (uint8_t *)malloc(size);
	data_ptr = this->buffer;
}

Packet::Packet(uint8_t *data, int size)
:_size(size)
{
    this->buffer = (uint8_t *)malloc(BUFFER_SIZE);
	memcpy(this->buffer, data, size);
	data_ptr = this->buffer;
}

Packet::~Packet()
{
	delete(buffer);
}

uint8_t Packet::ReadByte()
{
	_size--;
	return *data_ptr++;
}

int16_t Packet::ReadInt16()
{
	int16_t value = (data_ptr[0] & 0xFF) | (data_ptr[1] << 8);
	data_ptr += 2;
	_size-=2;
	return value;
}

int32_t Packet::ReadInt32()
{
	int32_t value = ((data_ptr[0] & 0xFF) | (data_ptr[1] << 8) | (data_ptr[2] << 16) | (data_ptr[3] << 24));
	data_ptr += 4;
	_size-=4;
	return value;
}

int Packet::ReadBuffer(uint8_t *data, int size)
{
	if(size > _size){
		size = _size;
	}
	memcpy(data, data_ptr, size);
	data_ptr+=size;
	_size-=size;

	return size;
}

int Packet::ReadPacket(Packet *pkt)
{
	pkt->WriteBuffer(data_ptr, _size);
	data_ptr+=_size;
	_size = 0;
}

void Packet::WriteByte(uint8_t value)
{
	*data_ptr++ = value;
	_size++;
}

void Packet::WriteInt16(uint16_t value)
{
	data_ptr[0] = value & 0xFF;
	data_ptr[1] = value >> 8;
	data_ptr += 2;
	_size += 2;
}

void Packet::WriteInt32(uint32_t value)
{
	data_ptr[0] = value & 0xFF;
	data_ptr[1] = (value >> 8);
	data_ptr[2] = (value >> 16);
	data_ptr[3] = (value >> 24);
	data_ptr += 4;
	_size += 4;
}

void Packet::WriteBuffer(uint8_t *data, int size)
{
	memcpy(data_ptr, data, size);	
	data_ptr += size;
	_size += size;
}

void Packet::EncodeInt16(uint16_t value)
{
	*data_ptr++ = DATA_INT16;
	_size++;
	WriteInt16(value);
}

void Packet::EncodeInt32(uint32_t value)
{
	*data_ptr++ = DATA_INT32;
	_size++;
	WriteInt32(value);
}

void Packet::EncodeBoolean(bool value)
{
	data_ptr[0] = DATA_BOOLEAN;
	data_ptr[1] = value ? 0x1 : 0;
	data_ptr+=2;
	_size+=2;
}

void Packet::EncodeByte(uint8_t value)
{
	data_ptr[0] = DATA_BYTE;
	data_ptr[1] = value;
	data_ptr+=2;
	_size+=2;
}

void Packet::EncodeString(const char *value)
{
	uint16_t len = strlen(value);
	*data_ptr++ = DATA_STRING;
	_size++;
	WriteInt16(len);
	memcpy(data_ptr, value, len);
    data_ptr+= len;
	_size+=len;
}

uint8_t Packet::DecodeByte()
{
	if(data_ptr[0] == DATA_BYTE){
		uint8_t val = data_ptr[1];
		data_ptr+=2;
		_size-=2;
		return val;
	}
	
	return 0;
}

uint16_t Packet::DecodeInt16()
{
	if(data_ptr[0] == DATA_INT16){
	   data_ptr++;
	   _size--;
	   return ReadInt16();
	}
	return 0;
}

uint32_t Packet::DecodeInt32()
{
	if(data_ptr[0] == DATA_INT32){
	   data_ptr++;
	   _size--;
	   return ReadInt32();
	}
	return 0;
}

bool Packet::DecodeBoolean()
{
	if(data_ptr[0] == DATA_BOOLEAN){		
	    bool value = data_ptr[1]== 0x1;
		data_ptr+=2;
	   _size-=2;
		return value;
	}
	return false;
}

bool Packet::DecodeString(str_st *value)
{
	if(data_ptr[0] == DATA_STRING){
		data_ptr++;
		_size--;
        value->len = ReadInt16();
	    value->str = data_ptr;
		data_ptr+= value->len;
		return true;
	}

	return false;
}

void Packet::Skip(int size)
{
	data_ptr +=size;
	_size -= size;
}

void Packet::SetPos(int size)
{
	data_ptr = buffer + size;
}

void Packet::stringCopy(uint8_t *temp, str_st *value)
{
	memcpy(temp, value->str, value->len);
	temp[value->len] = '\0';
}

uint8_t* Packet::toHardware(uint8_t *data)
{
	uint8_t *enc = data;

	*enc++= 0xBC;
	*enc++= 0xAC;
	*enc++= _size;
	int len = ReadBuffer(enc, _size);
	enc+= len;
	*enc++= 0xAC;
	
	return enc;
}



