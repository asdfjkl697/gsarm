#ifndef ModbusProc_H_
#define ModbusProc_H_

#pragma once
#include "common.h"

enum defModbusCmd_
{
	defModbusCmd_Read	= 0x03,
	defModbusCmd_Read04	= 0x04,
	defModbusCmd_Write	= 0x06,
	
	defModbusCmd_Err_Spec	= 0x80,

	defModbusCmd_Err_Read	= 0x83, // 0x03的错误响应
	defModbusCmd_Err_Read04	= 0x84, // 0x04的错误响应
	defModbusCmd_Err_Write	= 0x86	// 0x06的错误响应
};

class ModbusProc
{
public:
	ModbusProc(void);
	~ModbusProc(void);

public:
	static bool Decode_dev_cmd( uint8_t* const originalBuf, const uint16_t buflen, uint8_t &devid, uint8_t &cmdcode );

public:
	static bool Encode_Read03_Request( uint8_t* destBuf, uint16_t &buflen, const uint8_t devid, const uint16_t addressStart, const uint16_t addressCount );
	static bool Decode_Read03_Response( uint8_t* const originalBuf, const uint16_t buflen, std::vector<uint16_t> &vecAddressData );
	static bool Decode_Read03_Err_Response( uint8_t* const originalBuf, const uint16_t buflen, uint8_t &ErrCode );
	
	static bool Encode_Read04_Request( uint8_t* destBuf, uint16_t &buflen, const uint8_t devid, const uint16_t addressStart, const uint16_t addressCount );
	static bool Decode_Read04_Response( uint8_t* const originalBuf, const uint16_t buflen, std::vector<uint16_t> &vecAddressData );
	static bool Decode_Read04_Err_Response( uint8_t* const originalBuf, const uint16_t buflen, uint8_t &ErrCode );

	static bool Encode_Write06_Request( uint8_t* destBuf, uint16_t &buflen, const uint8_t devid, const uint16_t address, const uint16_t addressValue );
	static bool Decode_Write06_Response( uint8_t* const originalBuf, const uint16_t buflen, uint16_t &address, uint16_t &addressValue );
	static bool Decode_Write06_Err_Response( uint8_t* const originalBuf, const uint16_t buflen, uint8_t &ErrCode );
	
	static bool Encode_Write1601_Request( uint8_t* destBuf, uint16_t &buflen, const uint8_t devid, const uint16_t address, const uint16_t addressValue );

private:
	static bool Decode_Err_Response( uint8_t* const originalBuf, const uint16_t buflen, uint8_t &ErrCode );
	static bool Decode_Response_addressData( uint8_t* const originalBuf, const uint16_t buflen, std::vector<uint16_t> &vecAddressData, char *loginfo=NULL );
	static bool Decode_address_value( uint8_t* const originalBuf, const uint16_t buflen, uint16_t &address, uint16_t &addressValue, char *loginfo=NULL );

private:
	static bool Encode_data_2short( uint8_t* destBuf, uint16_t &buflen, const uint8_t devid, const uint8_t cmd, const uint16_t short1, const uint16_t short2, char *loginfo=NULL );
	static bool Encode_data_1Byte( uint8_t* destBuf, uint16_t &buflen, const uint8_t devid, const uint8_t cmd, const uint8_t byte1, const uint8_t byte2, char *loginfo=NULL );
};

#endif
