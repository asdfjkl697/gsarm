#ifndef _PACKET_H
#define _PACKET_H

#include <stdint.h>

#define BUFFER_SIZE 1024

#define PROTOCOL_FLAG 0x47534b4a //"GSKJ"
#define PROTOCOL_VERSION 65536


enum COMMAND
{
	CMD_BROADCAST_INFO = 0x10,
	CMD_BROADCAST_INFO_EX = 0x11,
	CMD_BROADCAST_INFO_PARAM = 0x12,
	CMD_BROADCAST_NETCONFIG = 0x13,
	CMD_BROADCAST_WIFICONFIG = 0x14,
	CMD_SET_NETWORK_IP = 0x20,
	CMD_SET_WIFI_IP    = 0x30,
	CMD_LOGIN          = 0x40,
	CMD_SERIAL         = 0x50,
	CMD_DEVICE_INFO    = 0x60,
	CMD_SCHEDULE_TASK  = 0x70,
	CMD_TRIGGER        = 0x80,
	CMD_RADIONETWORK   = 0x90,
	CMD_SYSTEM         = 0xF0	
};

enum ACK
{
	ACK_NOP = 0xDA,
	ACK_ASK = 0xAC,
	ACK_REPLY = 0xE0
};

enum DATA_TYPE
{
	DATA_NONE = 0x0,
	DATA_INT32,
	DATA_STRING,
	DATA_BOOLEAN,
	DATA_BYTE,
	DATA_INT16,
	DATA_INT64,
	DATA_DOUBLE,
	DATA_FLOAT,
	DATA_OBJECT,
	DATA_KEYVALUE
};

enum OLD_SERIAL_PACKET
{
	OLD_RXB8_315 = 0x01,
	OLD_RXB8_433,
	OLD_RXB8_315_TX,
	OLD_RXB8_433_TX,
	OLD_CC1101_433,
	OLD_CAN_Chip,
	OLD_RS485,
	OLD_TX315_RAW,
	OLD_TX433_RAW,
	OLD_GSM,
	OLD_Module_Set
};
	
enum SERIAL_PACKET
{
	RF_Receiver = 0x10,
	RF_Recraw   = 0x11,
	RF_Transmit = 0x1A,
	IR_Receiver = 0x30,
	IR_Transmit = 0x3A,
	RS485       = 0x20,
	CC1101      = 0x40,
	SI5351      = 0x50,
	CAN         = 0X60,
	SYSTEM_INFO = 0x80,
	SYSTEM_SET  = 0xE0,
	SYSTEM_RETURN = 0xEA,
};

enum SYSTEM_ACTION
{
	SYSTEM_READ = 0x0D,
	SYSTEM_WIRTE = 0x1D,
	SYSTEM_QUERY = 0x0A,
	SYSTEM_SETTING = 0x1A,
	SYSTEM_FAIL  = 0xEE	
};

typedef struct _serial_packet
{
	uint8_t head;
	uint8_t ack;
	uint8_t data_size;
	uint8_t read_size;
	uint8_t data[260];
	uint8_t end_ack;
	struct _serial_packet *next;
}serial_packet;

typedef struct _str_st
{
	uint8_t *str;
	uint16_t len;
}str_st;

class Packet
{
	protected:		
	    int _size;
	    uint8_t  *data_ptr;
		uint8_t *buffer;
		
	public:
		Packet(void);
		Packet(int size);
		Packet(uint8_t *data, int size);
		~Packet(void);
		
		uint8_t* get_data_ptr() { return data_ptr; };
		
		uint8_t ReadByte();
		int16_t ReadInt16();
		int32_t ReadInt32();
		int ReadBuffer(uint8_t *data, int size);
		int ReadPacket(Packet *pkt);
		
		void WriteByte(uint8_t value);		
        void WriteInt16(uint16_t value);
        void WriteInt32(uint32_t value);
		void WriteBuffer(uint8_t *data, int size);
		
		void EncodeInt16(uint16_t value);
		void EncodeInt32(uint32_t value);
		void EncodeBoolean(bool value);
		void EncodeByte(uint8_t value);
		void EncodeString(const char *value);
		
		bool DecodeString(str_st *value);
		uint8_t DecodeByte();
		bool DecodeBoolean();
		uint16_t DecodeInt16();
		uint32_t DecodeInt32();

		void Skip(int size);
		void SetPos(int size);
		int GetPos(){ return (data_ptr - buffer); };
		
		uint8_t *getBuffer(){ return buffer; };
		int getLength(){ return _size; };
		void stringCopy(uint8_t *temp, str_st *value);
        uint8_t* toHardware(uint8_t *data);
};

#endif

