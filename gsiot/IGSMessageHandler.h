#pragma once

#include "typedef.h"
#include "gloox/gloox.h"
#include "gloox/jid.h"
#include "gloox/tag.h"
#include "DeviceAddress.h"

#include "GSIOTDevice.h" //jyc20170224 add 

using namespace gloox;

enum defGSMsgType_
{
	defGSMsgType_Unknown = 0,
	defGSMsgType_Notify,

	defGSMsgType_FastRestart = 0x995A99,
};

enum defCtrlOprt
{
	defCtrlOprt_Null = 0,
	defCtrlOprt_CheckHeartbeat,			// 心跳检测
	defCtrlOprt_CheckDevTime,			// 校时

	defCtrlOprt_SendControl = 101,		// 命令
	defCtrlOprt_DoPrePic	= 102,		// 生成预览图
	defCtrlOprt_DoRealPic	= 103,		// 生成实时图
	
	defCtrlOprt_DestroyObj = 901,		// 销毁实例
};


/*
============
xmpp消息处理

============
*/
class GSMessage
{
public:
	GSMessage( const defGSMsgType_ MsgType, const JID& From, const std::string& id, StanzaExtension *const pEx );
	~GSMessage(void);

	defGSMsgType_ getMsgType() const
	{
		return m_MsgType;
	}

	uint32_t get_ts() const
	{
		return m_ts;
	}

	bool isOverTime( uint32_t overtime=20000 ) const;

	const JID& getFrom() const
	{
		return m_From;
	}

	const std::string& getId() const
	{
		return m_id;
	}

	const StanzaExtension* getpEx() const
	{
		return m_pEx;
	}

private:
	defGSMsgType_ m_MsgType;
	uint32_t m_ts; // 时间戳
	JID m_From;
	std::string m_id;
	StanzaExtension *m_pEx;
};

class IGSMessageHandler
{
public:
	IGSMessageHandler(void){};
	~IGSMessageHandler(void){};

public:
	virtual void OnGSMessage( defGSMsgType_ MsgType, uint32_t Param ){};
	virtual defGSReturn OnControlOperate( const defCtrlOprt CtrlOprt, const IOTDeviceType DevType,
	                                     const GSIOTDevice *device, 
	                                     const GSIOTObjBase *obj, 
	                                     const uint32_t overtime=defNormSendCtlOvertime, 
	                                     const uint32_t QueueOverTime=defNormMsgOvertime, 
	                                     const void *inParam=NULL, void *outParam=NULL ){ 
		return defGSReturn_Err; }; //jyc20170224 unnote
};

class ITriggerDebugHandler
{
public:
	ITriggerDebugHandler(void){};
	~ITriggerDebugHandler(void){};

public:
	virtual void OnTriggerDebug(
		defLinkID LinkID, const IOTDeviceType devtype, const std::string &devname,
		const bool AGRunState, const int AlarmGuardGlobalFlag, const bool IsValidCurTime,
		const struGSTime &dt, const std::string &strAlmBody, const std::string &strAlmSubject ){};
};

class ICommLinkNotifyHandler
{
public:
	ICommLinkNotifyHandler(void){};
	~ICommLinkNotifyHandler(void){};

	enum CLNType
	{
		CLNType_Refresh = 1,
	};

public:
	virtual void OnCommLinkNotify( CLNType type, defLinkID LinkID ){};
};


