#include "ModbusProc.h"

// modbus地址偏移
// 程序内部由1开始，通讯时进行偏移
#define defModbusAddressOffset_Decode (1)
#define defModbusAddressOffset_Encode (-1)

/***
1、总格式：
从设备地址		(1字节, 0x01-0xFF)
功能码			(1字节, 0x03/0x06等等)
数据区			(*)
CRC校验码		(2字节)

CRC检验码低位在前、高位在后,寄存器地址, 寄存器个数,数据均为高位在前、低位在后;

2、响应码规则：

2.1) 请求的正常响应
响应功能码=请求功能码

2.2) 请求的异常响应
响应功能码=请求功能码+0x80
并且后面提供一个异常码
***/


ModbusProc::ModbusProc()
{
}

ModbusProc::~ModbusProc(void)
{
}

/***
只解析出设备地址和功能码
格式符号如下：
从设备地址		(0x01-0xFF  1字节)
功能码			(1字节)
...
CRC校验码		(2字节)
***/
bool ModbusProc::Decode_dev_cmd( uint8_t* const originalBuf, const uint16_t buflen, uint8_t &devid, uint8_t &cmdcode )
{
	devid = 0;
	cmdcode = 0;

	if( buflen < 4 )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "ModbusProc::Decode_dev_cmd buflen error!!!\r\n");
		return false;
	}

	uint16_t crcFromDecode;
	memcpy( &crcFromDecode, originalBuf+buflen-2, 2 );
	uint16_t crcFromCal = crc16_verify( originalBuf, buflen-2 );

	if( crcFromDecode != crcFromCal )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "ModbusProc::Decode_dev_cmd crc error!!! crcFromDecode=%d, crcFromCal=%d\r\n", crcFromDecode, crcFromCal );
		return false;
	}

	devid = originalBuf[0];
	cmdcode = originalBuf[1];

	return true;
}

/***
解析出 错误响应 错误码
格式符号如下：
从设备地址		(1字节, 0x01-0xFF)
功能码			(1字节, 功能码(差错码)=0x8?)
异常码			(1字节)
CRC校验码		(2字节)
***/
bool ModbusProc::Decode_Err_Response( uint8_t* const originalBuf, const uint16_t buflen, uint8_t &ErrCode )
{
	ErrCode = *(originalBuf+2);
	return true;
}

/***
只解析出 多个寄存器值
格式符号如下：
从设备地址		(1字节, 0x01-0xFF)
功能码			(1字节)
数据区字节数		(1字节, 取值=2*寄存器个数)
寄存器值			(寄存器个数*2字节)
CRC校验码		(2字节)
***/
bool ModbusProc::Decode_Response_addressData( uint8_t* const originalBuf, const uint16_t buflen, std::vector<uint16_t> &vecAddressData, char *loginfo )
{
	uint8_t *ptemp = originalBuf+2;
	uint8_t *ptempEnd = originalBuf+buflen-2;

	uint8_t AddressCount = *ptemp;
	ptemp++;

	if( ptemp > ptempEnd )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "ModbusProc::Decode_Response_addressData(%s) ptempEnd error!!!\r\n", loginfo?loginfo:"" );
		return false;
	}

	if( AddressCount > (ptempEnd-ptemp) )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "ModbusProc::Decode_Response_addressData(%s) address count error!!! decoded count=%d\r\n", loginfo?loginfo:"", AddressCount );
		return false;
	}

	uint8_t *paddressData = ptemp;
	for( ; paddressData<ptempEnd; )
	{
		vecAddressData.push_back( Big_ByteToInt16( paddressData ) );

		paddressData +=2;
	}

	return true;
}

/***
只解析出寄存器地址和寄存器值
格式符号如下：
从设备地址		(0x01-0xFF  1字节)
功能码			(1字节)
寄存器地址		(2字节)
寄存器值			(2字节)
CRC校验码		(2字节)
***/
bool ModbusProc::Decode_address_value( uint8_t* const originalBuf, const uint16_t buflen, uint16_t &address, uint16_t &addressValue, char *loginfo )
{
	address = 0;
	addressValue = 0;

	if( buflen < 4 )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "ModbusProc::Decode_address_value(%s) buflen error!!!\r\n", loginfo?loginfo:"" );
		return false;
	}

	address = Big_ByteToInt16( originalBuf+2 )+defModbusAddressOffset_Decode;
	addressValue = Big_ByteToInt16( originalBuf+4 );
	return true;
}


/***
通用编码函数
格式符号如下：
从设备地址		(1字节, 0x01-0xFF)
功能码			(1字节, 0x06)
2字节整数1		(2字节)
2字节整数2		(2字节)
CRC校验码		(2字节)
***/
bool ModbusProc::Encode_data_2short( uint8_t* destBuf, uint16_t &buflen, const uint8_t devid, const uint8_t cmd, const uint16_t short1, const uint16_t short2, char *loginfo )
{
	if( buflen < 8 )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "ModbusProc::Encode_data_2short(%s) buflen=%d error!!! devid=%d, cmd=%d\r\n", loginfo?loginfo:"", buflen, devid, cmd );
		return false;
	}

	uint8_t *enc = destBuf;

	*enc = devid; enc++;
	*enc = cmd; enc++;
	
	enc = Big_Int16ToByte( enc, short1 );
	enc = Big_Int16ToByte( enc, short2 );

	uint16_t crcFromCal = crc16_verify( destBuf, enc-destBuf );
	memcpy( enc, &crcFromCal, 2 );

	buflen = 8;
	return true;
}

/***
通用编码函数
格式符号如下：
从设备地址		(1字节, 0x01-0xFF)
功能码			(1字节, 0xXX)
1字节整数1		(1字节)
1字节整数2		(1字节)
CRC校验码		(2字节)
***/
bool ModbusProc::Encode_data_1Byte( uint8_t* destBuf, uint16_t &buflen, const uint8_t devid, const uint8_t cmd, const uint8_t byte1, const uint8_t byte2, char *loginfo )
{
	if( buflen < 6 )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "ModbusProc::Encode_data_1Byte(%s) buflen=%d error!!! devid=%d, cmd=%d\r\n", loginfo?loginfo:"", buflen, devid, cmd );
		return false;
	}

	uint8_t *enc = destBuf;

	*enc = devid; enc++;
	*enc = cmd; enc++;

	*enc = byte1; enc++;
	*enc = byte2; enc++;

	uint16_t crcFromCal = crc16_verify( destBuf, enc-destBuf );
	memcpy( enc, &crcFromCal, 2 );

	buflen = 6;
	return true;
}

//////////////////////////////////////////////////////////

/***
03(0x03)读保持寄存器 请求
从设备地址		(1字节, 0x01-0xFF)
功能码			(1字节, 0x03)
起始寄存器地址	(2字节)
寄存器数量		(2字节)
CRC校验码		(2字节)
***/
bool ModbusProc::Encode_Read03_Request( uint8_t* destBuf, uint16_t &buflen, const uint8_t devid, const uint16_t addressStart, const uint16_t addressCount )
{
	return Encode_data_2short( destBuf, buflen, devid, 0x03, addressStart+defModbusAddressOffset_Encode, addressCount, "Encode_Read03_Request" );
}

/***
03(0x03)读保持寄存器 响应
从设备地址		(1字节, 0x01-0xFF)
功能码			(1字节, 0x03)
数据区字节数		(1字节, 取值=2*寄存器个数)
寄存器值			(寄存器个数*2字节)
CRC校验码		(2字节)
***/
bool ModbusProc::Decode_Read03_Response( uint8_t* const originalBuf, const uint16_t buflen, std::vector<uint16_t> &vecAddressData )
{
	return Decode_Response_addressData( originalBuf, buflen, vecAddressData );
}

/***
03(0x03)读保持寄存器 错误
从设备地址		(1字节, 0x01-0xFF)
功能码			(1字节, 功能码(差错码)=0x83)
异常码			(1字节)
CRC校验码		(2字节)
***/
bool ModbusProc::Decode_Read03_Err_Response( uint8_t* const originalBuf, const uint16_t buflen, uint8_t &ErrCode )
{
	return Decode_Err_Response( originalBuf, buflen, ErrCode );
}

/***
04(0x04)读保持寄存器 请求
从设备地址		(1字节, 0x01-0xFF)
功能码			(1字节, 0x04)
起始寄存器地址	(2字节)
寄存器数量		(2字节)
CRC校验码		(2字节)
***/
bool ModbusProc::Encode_Read04_Request( uint8_t* destBuf, uint16_t &buflen, const uint8_t devid, const uint16_t addressStart, const uint16_t addressCount )
{
	return Encode_data_2short( destBuf, buflen, devid, 0x04, addressStart+defModbusAddressOffset_Encode, addressCount, "Encode_Read04_Request" );
}

/***
04(0x04)读保持寄存器 响应
从设备地址		(1字节, 0x01-0xFF)
功能码			(1字节, 0x04)
数据区字节数		(1字节, 取值=2*寄存器个数)
寄存器值			(寄存器个数*2字节)
CRC校验码		(2字节)
***/
bool ModbusProc::Decode_Read04_Response( uint8_t* const originalBuf, const uint16_t buflen, std::vector<uint16_t> &vecAddressData )
{
	return Decode_Response_addressData( originalBuf, buflen, vecAddressData );
}

/***
04(0x04)读保持寄存器 错误
从设备地址		(1字节, 0x01-0xFF)
功能码			(1字节, 功能码(差错码)=0x84)
异常码			(1字节)
CRC校验码		(2字节)
***/
bool ModbusProc::Decode_Read04_Err_Response( uint8_t* const originalBuf, const uint16_t buflen, uint8_t &ErrCode )
{
	return Decode_Err_Response( originalBuf, buflen, ErrCode );
}

/***
06(0x06)写单个寄存器 请求
从设备地址		(1字节, 0x01-0xFF)
功能码			(1字节, 0x06)
起始寄存器地址	(2字节)
写入寄存器的值	(2字节)
CRC校验码		(2字节)
***/
bool ModbusProc::Encode_Write06_Request( uint8_t* destBuf, uint16_t &buflen, const uint8_t devid, const uint16_t address, const uint16_t addressValue )
{
	return Encode_data_2short( destBuf, buflen, devid, 0x06, address+defModbusAddressOffset_Encode, addressValue, "Encode_Write06_Request" );
}

/***
06(0x06)写单个寄存器 响应
从设备地址		(1字节, 0x01-0xFF)
功能码			(1字节, 0x06)
寄存器地址		(2字节)
寄存器值			(2字节)
CRC校验码		(2字节)
***/
bool ModbusProc::Decode_Write06_Response( uint8_t* const originalBuf, const uint16_t buflen, uint16_t &address, uint16_t &addressValue )
{
	return Decode_address_value( originalBuf, buflen, address, addressValue );
}

/***
06(0x06)写单个寄存器 错误
从设备地址		(1字节, 0x01-0xFF)
功能码			(1字节, 功能码(差错码)=0x86)
异常码			(1字节)
CRC校验码		(2字节)
***/
bool ModbusProc::Decode_Write06_Err_Response( uint8_t* const originalBuf, const uint16_t buflen, uint8_t &ErrCode )
{
	return Decode_Err_Response( originalBuf, buflen, ErrCode );
}


/***
16(0x10)写寄存器FC16单字节 请求 (可能非标准)
从设备地址		(1字节, 0x01-0xFF)
功能码			(1字节, 0x10)
起始寄存器地址	(1字节)
写入寄存器的值	(1字节)
CRC校验码		(2字节)
***/
bool ModbusProc::Encode_Write1601_Request( uint8_t* destBuf, uint16_t &buflen, const uint8_t devid, const uint16_t address, const uint16_t addressValue )
{
	return Encode_data_1Byte( destBuf, buflen, devid, 0x10, address+defModbusAddressOffset_Encode, addressValue, "Encode_Write1601_Request" );
}
