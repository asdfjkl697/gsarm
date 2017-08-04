#ifndef RS485DevControl_H_
#define RS485DevControl_H_

#pragma once
#include "gloox/tag.h"
#include "ControlBase.h"
#include "DeviceAddress.h"
#include "AddressManager.h"
#include "ModbusProc.h"
#include "SerialMessage.h"
#include "EditAttrMgr.h"

enum defProtocol_
{
	defProtocol_Unknown = 0, // 默认
	defProtocol_Modbus
};

#define RS485_BUF_MAX (255)

struct struRS485Msg
{
	uint8_t buf[RS485_BUF_MAX];
	uint16_t buflen;

	void Reset(void)
	{
		buflen = RS485_BUF_MAX;
	}
};
typedef std::list<struRS485Msg*> defRS485MsgQue;

void g_DeleteRS485MsgQue( defRS485MsgQue &que );

#define defRS485_Ver_Wind_03_201310A	"13101-F03"		// 20131014
#define defRS485_Ver_Wind_04_201312A	"13121-F04"		// 20131223
#define defRS485_Ver_Wind_1601_201501A	"15011-YGCFS"	// 20150112 // 易谷科技 风速
#define defRS485_Ver_Wind_Default		defRS485_Ver_Wind_1601_201501A

#define defRS485_Ver_Humidity_1601_201506A	"15061-YGCTS"	// 20150601 // 易谷科技 土壤湿度

#define defRS485_Ver_HCHO_201706A	"autoc-16121-HCHO"
#define defRS485_Ver_CO2_201706A	"autoc-16121-CO2"

//-----------------------------------------------
// 特定RS485地址

// 开关 控制,SwitchNum 1开始至10
#define defSwitchNumMin 1
#define defSwitchNumMax 10
#define defSwitchNumAddrFix (0x0B-1+1) // 转换成实际地址
#define defSpecAddr_Switch_Ctrl( pCtrl, SwitchNumStart, SwitchNumEnd ) \
	pCtrl->AddSpecAddr( DeviceAddress( SwitchNumStart+defSwitchNumAddrFix, "开关", IOT_DEVICE_Switch, IOT_Byte, IOT_READWRITE, "0", "1" ), \
	SwitchNumStart+defSwitchNumAddrFix, SwitchNumEnd+defSwitchNumAddrFix, SwitchNumStart );

#define defSpecAddr_CO2_Read( pCtrl ) \
	pCtrl->AddSpecAddr( DeviceAddress( 1+1, "CO2", IOT_DEVICE_CO2, IOT_Float, IOT_READ ) );

#define defSpecAddr_HCHO_Read( pCtrl ) \
	pCtrl->AddSpecAddr( DeviceAddress( 2+1, "HCHO", IOT_DEVICE_HCHO, IOT_Float, IOT_READ ) );

//#define defSpecAddr_PM25_Read( pCtrl ) \
	pCtrl->AddSpecAddr( DeviceAddress( 1+1, "PM2.5", IOT_DEVICE_PM25, IOT_Float, IOT_READ ) );

#define defSpecAddr_Temperature_Read( pCtrl ) \
	pCtrl->AddSpecAddr( DeviceAddress( 1+1, "温度", IOT_DEVICE_Temperature, IOT_Float, IOT_READ ) );

// 湿度 读取湿度
#define defSpecAddr_Humidity_Read( pCtrl ) \
	pCtrl->AddSpecAddr( DeviceAddress( 2+1, "湿度", IOT_DEVICE_Humidity, IOT_Float, IOT_READ ) );

// 风速 读取风速
#define defSpecAddr_Wind_Read( pCtrl, AddrSpec ) \
	pCtrl->AddSpecAddr( DeviceAddress( (AddrSpec)?(AddrSpec):3, "风速", IOT_DEVICE_Wind, IOT_Float, IOT_READ ) );

// 土壤湿度
#define defSpecAddr_HumidityTS_Read( pCtrl, AddrSpec ) \
	pCtrl->AddSpecAddr( DeviceAddress( (AddrSpec)?(AddrSpec):2+1, "土壤湿度", IOT_DEVICE_Humidity, IOT_Float, IOT_READ ) );

using namespace gloox;

class RS485DevControl : public ControlBase, public AddressManager, public EditAttrMgr
{
public:
	RS485DevControl( uint32_t deviceID, uint32_t command, uint32_t protocol=defProtocol_Unknown, const std::string &ver=c_DefaultVer );
	RS485DevControl( const Tag* tag = 0 );
	~RS485DevControl(void);

public:
	static bool IsReadCmd( uint32_t cmd );
	static bool IsWriteCmd( uint32_t cmd );
	static bool IsReadCmd_Ret( uint32_t cmd );
	static bool IsUseRead04( IOTDeviceType type, const std::string ver );

	// 创建特定地址的RS485设备
	static RS485DevControl* CreateRS485DevControl_ModifyDevAddr( IOTDeviceType DeviceType, uint32_t oldDevID, uint32_t newDevID, const std::string &ver=c_DefaultVer );
	bool AddSpecAddr( DeviceAddress &Addrobj, uint16_t addressStart=0, uint16_t addressEnd=0, uint16_t SNoStart=0 );
	static bool IsValueOverRange( IOTDeviceType DeviceType, float val );
	static float ValueRangeFix( IOTDeviceType DeviceType, float val );
	
	bool doEditAttrFromAttrMgr( const EditAttrMgr &attrMgr, defCfgOprt_ oprt = defCfgOprt_Modify );

public:

	Tag* tag(const struTagParam &TagParam) const;	
	virtual ControlBase* clone( bool CreateLock=true ) const;
	std::string Print( const char *info=NULL, bool doPrint=true, DeviceAddress *const pSpecAddr=NULL ) const;

	IOTDeviceType GetType() const
	{
		return IOT_DEVICE_RS485;
	}

	// 基本可用状态
	defUseable set_NetUseable( defUseable NetUseable, bool *isChanged=NULL );
	defUseable get_NetUseable();
	virtual void check_NetUseable_RecvFailed( bool *isChanged=NULL );

	void setVer( const std::string &ver )
	{
		m_ver = ver;
	}

	std::string getVer() const
	{
		return this->m_ver;
	}

	uint32_t GetDeviceid() const
	{
		return m_deviceID;
	}
	
	void SetDeviceid( uint32_t deviceID )
	{
		m_deviceID = deviceID;
	}

	const std::string& GetName() const
	{
		return m_Name;
	}
	void SetName( const std::string &name )
	{
		m_Name = name;
	}

	uint32_t GetCommand() const
	{
		return m_command;
	}
	
	void SetCommand( uint32_t command )
	{
		m_command = command;
	}

	uint32_t GetProtocol() const
	{
		return m_protocol;
	}

	void SetReturnErrCode( uint32_t ReturnErrCode )
	{
		m_returnErrCode = ReturnErrCode;
	}

	uint32_t GetReturnErrCode() const
	{
		return m_returnErrCode;
	}

	void Reset();
	
	bool IsCanBtachRead() const
	{
		uint16_t addressStart = 0;
		uint16_t addressCount = 0;
		uint16_t addressEnableCount = 0;
		return GetBtachReadParam( addressStart, addressCount, addressEnableCount );
	}
	bool GetBtachReadParam( uint16_t &addressStart, uint16_t &addressCount, uint16_t &addressEnableCount ) const;
	bool Encode( defRS485MsgQue &que, DeviceAddress *const pSpecAddr, uint32_t SpecCmd=0 );
	static bool Decode( uint8_t* const originalBuf, const uint32_t buflen, RS485DevControl **pctrl );

private:
	void InitNewMutex( bool CreateLock=true );
	defUseable set_NetUseable_nolock( defUseable NetUseable, bool *isChanged );

private:
	uint32_t m_deviceID; // 例如：modbus 设备地址0-255
	std::string m_Name;
	uint32_t m_command;
	uint32_t m_protocol;
	std::string m_ver;
	
	uint32_t m_returnErrCode;
	defUseable m_NetUseable;
	gloox::util::Mutex *m_pmutex_RS485DevCtl;

	uint32_t m_failedCount;
	uint32_t m_lastSuccessTS;
};

#endif
