#pragma once

#include "typedef.h"
#include "common.h"

enum defRFSignalType
{
	defRFSignalType_invalid		= 0,	// 无效
	defRFSignalType_code		= 1,	// 解析后编码的信号格式RF
	defRFSignalType_original	= 2,	// 原始信号格式RF
	defRFSignalType_IR_code		= 3,	// 红外编码格式
	defRFSignalType_IR_original	= 4,	// 红外原始信号格式

	defRFSignalType_Ex_Spec_	= 9000,	// 信号类型分割线

	defRFSignalType_RemoteObj_ExType	= 9001,	// 远程对象 - 拓展类型，当为拓展类型时，拓展类型存储在code
};

enum defFreq
{
	defFreq_Null = 0,
	defFreq_315 = 315,
	defFreq_433 = 433
};

#define defRFSignalOriginalMax 125

struct RFSignal
{
	int id;
	defRFSignalType signal_type;// 信号格式类型
	defFreq freq;				// 315 or 443

	uint32_t code;				// 数据码时序
	uint8_t codeValidLen;		// (数据码时序)指令长度		指令有效位长度(位长)

	uint8_t headlen;			// 引导码长度，0表示没有引导码
	uint16_t headcode[7];		// 引导码缓存

	uint16_t one_high_time;		// 数据段BIT 1上升沿时间		时间单位：us
	uint16_t one_low_time;		// 数据段BIT 1下降沿时间		时间单位：us
	uint16_t zero_high_time;	// 数据段BIT 0上升沿时间		时间单位：us
	uint16_t zero_low_time;		// 数据段BIT 0下降沿时间		时间单位：us

	uint16_t silent_interval;	// silent间隔时间			时间单位：us

	uint8_t taillen;			// 结束码长度，0表示没有结束码
	uint16_t tailcode[7];		// 结束码缓存

	uint8_t signal_count;		//次数，接收时用于判断收到次数是否达到，发送时用于发送次数

	// * 为原始信号时存储
	uint8_t original_headflag;	// 原始信号 引导码正脉冲或负脉冲标志 0x00负脉冲，0x01正
	uint16_t original_headtime;	// 原始信号 引导码时间
	uint8_t original_len;		// 原始信号 数据长度
	uint16_t original[defRFSignalOriginalMax];		// 原始信号 数据序列

	RFSignal()
	{
		RFSignal::Init( *this );
	}

	static void Init( RFSignal &rfsignal )
	{
		memset( &rfsignal, 0, sizeof(RFSignal) );
	}

	static std::string Trans_Type2Str( const defRFSignalType signal_type );

	bool isValid() const
	{
		return ( defRFSignalType_invalid != this->signal_type );
	}

	bool isSignal() const
	{
		return ( this->signal_type < defRFSignalType_Ex_Spec_ );
	}

	bool operator == ( const RFSignal &other ) const;
	bool IsNear( const RFSignal &other ) const;
	void Update( const RFSignal &src, bool onlysignal );
	SerialPortModule GetModuleIndex( bool isSend=true );

	std::string Get_original( int format=1, const bool isByteStr=false ) const;
	bool Set_original( const std::string &stroriginal, const bool isByteStr=false );
	std::string Print( const char *info=NULL, bool doPrint=true ) const;
	std::string GetUIShow( const uint8_t spec_count=0 ) const;

	bool SaveToFile( const std::string &filename ) const;
	bool LoadFromFile( const std::string &filename );

	uint32_t GetSendNeedTime( const uint8_t spec_count=0 ) const;
	int GetFixUnit() const;
	std::string Get_DecodeDesc( bool showint=false ) const;
};

