#ifndef RFREMOTECONTROM_H_
#define RFREMOTECONTROM_H_

#include "gloox/tag.h"
#include "ControlBase.h"
#include "RemoteButtonManager.h"

using namespace gloox;

class RFRemoteControl : public ControlBase, public RemoteButtonManager
{
public:
	RFRemoteControl(const Tag* tag=0);
	~RFRemoteControl(void);

	virtual IOTDeviceType GetType() const
	{
		return IOT_DEVICE_Remote;
	}

	virtual IOTDeviceType GetExType() const
	{
		return GetType();
	}

	static IOTDeviceType Save_TransFromExType( const std::string& loadstr );	// 将来自保存的字段转换得到扩展类型
	virtual std::string Save_RefreshToSave(){ return c_NullStr; }			// 刷新值到即将保存的字段
	virtual void Save_RefreshFromLoad(const std::string& loadstr){}			// 将来自保存的字段刷新为值
	virtual std::string GetCfgDesc() const{ return c_NullStr; }				// 获取配置描述

	Tag* tag(const struTagParam &TagParam) const;

	virtual ControlBase* clone( bool CreateLock=true ) const{
	    return new RFRemoteControl(*this);
	}
	
	virtual bool isSameRmtCtl( const RFRemoteControl *other ) const{ return false; }

	virtual defUseable get_NetUseable()
	{
		return defUseable_OK;
	}

	const std::string& GetName() const
	{
		return m_Name;
	}
	void SetName( const std::string &name )
	{
		m_Name = name;
	}
	
	std::string Print( const char *info, bool doPrint, RemoteButton *const pSpec ) const;

protected:
	std::string m_Name;
};

class GSRemoteCtl_AC : public RFRemoteControl
{
public:
	GSRemoteCtl_AC( const Tag* tag=0 );
	~GSRemoteCtl_AC();
	
	virtual IOTDeviceType GetExType() const
	{
		return IOTDevice_AC_Ctl;
	}


protected:
	struct CtlParam
	{
		std::string ip;
		std::string pwd;

#pragma pack(push, 1)
		struct SubParam
		{
			USHORT size;
			defFactory factory;
			USHORT port;

			SubParam( void )
				: size(sizeof( SubParam ))
			{
				factory = defFactory_Unknown;
				port = 4370;
			}
		}sParam;
#pragma pack(pop)

		CtlParam( void )
		{
			ip = "192.168.2.17";//test
			pwd = "93280";
		}
	}m_param;

	struct CtlState
	{
		defConnectState ConnectState;

		CtlState( void )
		{
			ConnectState = defConnectState_Null;
		}
	}m_state;

public:
	virtual std::string Save_RefreshToSave();									// 刷新值到即将保存的字段
	virtual void Save_RefreshFromLoad(const std::string& loadstr);				// 将来自保存的字段刷新为值
	virtual std::string GetCfgDesc() const;										// 获取配置描述

	Tag* tag( const struTagParam &TagParam ) const;

	virtual ControlBase* clone( bool CreateLock=true ) const{
		return new GSRemoteCtl_AC( *this );
	}

	virtual bool isSameRmtCtl( const RFRemoteControl *other ) const
	{
		if( !other )
			return false;

		if( this->GetExType() != other->GetExType() )
			return false;

		const GSRemoteCtl_AC *otherac = (const GSRemoteCtl_AC*)other;

		if( this->get_param_safe().ip != otherac->get_param_safe().ip )
			return false;

		return ( this->get_param_safe().sParam.port == otherac->get_param_safe().sParam.port );
	}

	virtual defUseable get_NetUseable()
	{
		return ( defConnectState_Connected==m_state.ConnectState || defConnectState_Null==m_state.ConnectState ? defUseable_OK : defUseable_ErrNet );
	}

	defFactory get_factory() const
	{
		return m_param.sParam.factory;
	}
	
	const CtlParam& get_param_safe() const
	{
		return m_param;
	}

	CtlParam& get_param()
	{
		return m_param;
	}

	const CtlState& get_state_safe() const
	{
		return m_state;
	}

	CtlState& get_state()
	{
		return m_state;
	}

	GSRemoteObj_AC_Door* GetDoor( const long DoorNo ) const;
	defGSReturn UpdateDoorState( const long DoorNo, const long state );
	long GetOneNewDoorNo() const;
};


//////////

class GSRemoteCtl_Combo_Ctl : public RFRemoteControl
{
public:
	GSRemoteCtl_Combo_Ctl( const Tag* tag=0 );
	~GSRemoteCtl_Combo_Ctl();
	
	virtual IOTDeviceType GetExType() const
	{
		return IOTDevice_Combo_Ctl;
	}

public:
	virtual std::string Save_RefreshToSave();									// 刷新值到即将保存的字段
	virtual void Save_RefreshFromLoad(const std::string& loadstr);				// 将来自保存的字段刷新为值
	
	Tag* tag( const struTagParam &TagParam ) const;

	virtual ControlBase* clone( bool CreateLock=true ) const{
		return new GSRemoteCtl_Combo_Ctl( *this );
	}
};

#endif
