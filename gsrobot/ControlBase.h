#ifndef CONTROLBASE_H
#define CONTROLBASE_H

#include "typedef.h"
#include "gloox/tag.h"

using namespace gloox;

class ControlBase
{
public:
	friend class GSIOTDevice;
	ControlBase(const Tag* tag=0) : LinkID(0), m_Cmd(defCmd_Null) {};
	virtual ~ControlBase(void){};
	void SetLinkID( const defLinkID in_LinkID ) { LinkID = in_LinkID; }
	defLinkID GetLinkID() const { return LinkID; }
	virtual const std::string& GetName() const = 0;//  { return gloox::EmptyString;};
	virtual void SetName( const std::string &name ) = 0;//{};
	virtual IOTDeviceType GetType() const =0;
	virtual IOTDeviceType GetExType() const { return GetType(); }
	virtual Tag* tag(const struTagParam &TagParam) const = 0;
	virtual ControlBase* clone( bool CreateLock=true ) const = 0;
	virtual std::string Print( bool doPrint=true ) const { return std::string(""); };

	virtual bool SetCmd( const defCmd Cmd )
	{
		m_Cmd = Cmd;
		return true;
	}
	virtual defCmd GetCmd() const
	{
		return m_Cmd;
	}

protected:
	defLinkID LinkID;
	defCmd m_Cmd;

public:
	static std::string TypeToString( const IOTDeviceType type, const IOTDeviceType ExType=IOT_DEVICE_Unknown )
	{
		switch( IOT_DEVICE_Unknown==ExType?type:ExType )
		{
		case IOT_DEVICE_Unknown:
			return std::string("未知");

		case IOT_DEVICE_Video:
			return std::string("视频设备");

		case IOT_DEVICE_Audio:
			return std::string("音频设备");

		case IOT_DEVICE_Switch:
			return std::string("开关");

		case IOT_DEVICE_Temperature:
			return std::string("温度");

		case IOT_DEVICE_Humidity:
			return std::string("湿度");

		case IOT_DEVICE_CO2:
			return std::string("CO2");  //jyc20170304 add
		case IOT_DEVICE_HCHO:
			return std::string("HCHO");  //jyc20170304 add
		case IOT_DEVICE_PM25:
			return std::string("PM2.5");  //jyc20170304 add

		case IOT_DEVICE_Trigger:
			return std::string("触发器");

		case IOT_DEVICE_Camera:
			return std::string("摄像机");

		case IOT_DEVICE_Remote:
			return std::string("遥控");

		case IOT_DEVICE_RFDevice:
			return std::string("无线设备");

		case IOT_DEVICE_CANDevice:
			return std::string("总线设备");

		case IOT_DEVICE_RS485:
			return std::string("RS485设备");

		case IOT_DEVICE_Wind:
			return std::string("风速");

		case IOTDevice_AC_Ctl:
			return std::string( "门禁" );

		case IOTDevice_AC_Door:
			return std::string( "门" );

		case IOTDevice_Combo_Ctl:
			return std::string( "组合控制" );

		case IOT_DEVICE_All:
			return std::string("所有设备");
		}

		return std::string("未知类型");
	}
	
	static std::string TypeToSimpleStr( const IOTDeviceType type )
	{
		switch( type )
		{
		case IOT_DEVICE_Unknown:
			return std::string("Unkwn");

		case IOT_DEVICE_Video:
			return std::string("Video");

		case IOT_DEVICE_Audio:
			return std::string("Audio");

		case IOT_DEVICE_Switch:
			return std::string("Switch");

		case IOT_DEVICE_Temperature:
			return std::string("Temp");

		case IOT_DEVICE_Humidity:
			return std::string("Humi");

		case IOT_DEVICE_CO2:
			return std::string("Co2");
		case IOT_DEVICE_HCHO:
			return std::string("Hcho");
		case IOT_DEVICE_PM25:
			return std::string("Pm2.5");

		case IOT_DEVICE_Trigger:
			return std::string("Trig");

		case IOT_DEVICE_Camera:
			return std::string("Cam");

		case IOT_DEVICE_Remote:
			return std::string("Remote");

		case IOT_DEVICE_RFDevice:
			return std::string("RFDev");

		case IOT_DEVICE_CANDevice:
			return std::string("CANDev");

		case IOT_DEVICE_RS485:
			return std::string("RS485");

		case IOT_DEVICE_Wind:
			return std::string("Wind");

		case IOTDevice_AC_Ctl:
			return std::string( "ACCtl" );

		case IOTDevice_AC_Door:
			return std::string( "Door" );
			
		case IOTDevice_Combo_Ctl:
			return std::string( "CmbCtl" );

		case IOT_DEVICE_All:
			return std::string("All");
		}

		return std::string("UnDef");
	}
};

enum defAGRunStateFinalST
{
	defAGRunStateFinalST_UnAg		= 0,	// 撤防
	defAGRunStateFinalST_AgFinish	= 1,	// 布防已启用，并且“布防完成”
	defAGRunStateFinalST_AgFailed	= 2,	// 布防已启用，但“布防失败”，一般会每隔段时间自动尝试布防
};

class AGRunStateBase
{
public:
	AGRunStateBase( bool AGRunState=false ) : m_AGRunState(AGRunState) {};
	virtual ~AGRunStateBase(void){};

	// 设置布防标志配置
	virtual bool SetAGRunState( bool AGRunState, bool onlyvalue=false )
	{
		m_AGRunState = AGRunState;
		return true;
	}

	// 获取布防标志配置
	virtual bool GetAGRunState() const
	{
		return m_AGRunState;
	}

	// 布防是否完成
	virtual bool IsAGRunStateFinished() const
	{
		return GetAGRunState();
	}

	// 返回最终布防状态
	virtual defAGRunStateFinalST GetAGRunStateFinalST() const
	{
		if( !IsSupportAGRunState() )
		{
			return defAGRunStateFinalST_UnAg;
		}

		return ( GetAGRunState() ? defAGRunStateFinalST_AgFinish:defAGRunStateFinalST_UnAg );
	}

	virtual bool IsSupportAGRunState() const = 0;

protected:
	bool m_AGRunState;
};

#endif
