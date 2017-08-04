#ifndef SERIALMESSAGE_H_
#define SERIALMESSAGE_H_

#include "typedef.h"
#include "ISerialPortHandler.h"
#include "gloox/mutexguard.h"
#include "DeviceAddress.h"
#include "ControlBase.h"
#include "common.h"
#include "HeartbeatMon.h"

class SerialMessage
{
private:
	//bool m_queue_state;
	gloox::util::Mutex m_queue_mutex;
	std::list<SERIALDATABUFFER *> m_buffer_queue;
	ISerialPortHandler *m_handler;

	//// 当前正在执行的RS485读命令
	uint32_t m_lastPrintIs_CurCmd;
	CMsgCurCmd m_MsgCurCmd;

public:
	SerialMessage(ISerialPortHandler *handler);
	~SerialMessage(void);

	CMsgCurCmd &GetMsgCurCmdObj()
	{
		return m_MsgCurCmd;
	}

	void doMessage( defLinkID LinkID, uint8_t *data, uint32_t size, const IOTDeviceType DevType, const uint32_t DevID, ControlBase *ctl, DeviceAddress *address, uint32_t overtime, uint32_t QueueOverTime, uint32_t nextInterval=1 );	
	void Check();
	void onTimer( CHeartbeatGuard *phbGuard, CCommLinkRun *CommLink );
};

#endif

