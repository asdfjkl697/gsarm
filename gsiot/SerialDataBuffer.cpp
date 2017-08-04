#include "SerialDataBuffer.h"
#include "GSIOTClient.h"


_sdatabuffer::_sdatabuffer()
{
	LinkID = 0;
	data = NULL;
	size = 0;
	DevType = IOT_DEVICE_Unknown;
	DevID = 0;
	ctl = NULL;
	address = NULL;
	m_time = timeGetTime();
	overtime = defNormSendCtlOvertime;
	QueueOverTime = defNormMsgOvertime;
	nextInterval = 0;
}

_sdatabuffer::~_sdatabuffer()
{
	macCheckAndDel_Array(this->data);

	if( this->ctl ) delete this->ctl;
	if( this->address ) delete this->address;

	this->size = 0;
	this->ctl = NULL;
	this->address = NULL;
}

void _sdatabuffer::SetNowTime()
{
	m_time = timeGetTime();
}

bool _sdatabuffer::IsOverTime() const
{
	return ( timeGetTime()-m_time > overtime );
}

bool _sdatabuffer::IsQueueOverTime() const
{
	return ( timeGetTime()-m_time > QueueOverTime );
}

uint32_t _sdatabuffer::get_nextInterval() const
{
	return ( (nextInterval>5000) ? 5000:nextInterval );
}

void _sdatabuffer::Print( const char *info ) const
{
	LOGMSG( "(Link%d) %s ctl(%s)=%s, addr(%s)=%d\r\n", 
		LinkID,
		info?info:"",
		ctl?ControlBase::TypeToSimpleStr(ctl->GetType()).c_str():"null",
		ctl?ctl->Print(false).c_str():"null",
		address?ControlBase::TypeToSimpleStr(address->GetType()).c_str():"null",
		address?address->GetAddress():0 );
}

bool _sdatabuffer::IsSameOnlyDev( const _sdatabuffer &other )
{
	return IsSameOnlyDev( other.LinkID, other.DevType, other.DevID, other.ctl );
}

// 相同设备
bool _sdatabuffer::IsSameOnlyDev(
	const defLinkID other_LinkID, const IOTDeviceType other_DevType, const uint32_t other_DevID,
	const ControlBase *other_ctl
	)
{
	if( !this->ctl
		|| !other_ctl
		)
	{
		return false;
	}

	if( other_DevID != DevID )
		return false;

	if( other_LinkID != LinkID )
		return false;

	if( other_DevType != DevType )
		return false;
	if( !GSIOTClient::Compare_Control( other_ctl, ctl ) )
		return false;

	return true;
}

bool _sdatabuffer::IsSame( const _sdatabuffer &other )
{
	return IsSame( other.LinkID, other.data, other.size, other.DevType, other.DevID, other.ctl, other.address, other.overtime, other.QueueOverTime, other.nextInterval );
}

// 完全相同
bool _sdatabuffer::IsSame(
	const defLinkID other_LinkID, const uint8_t *other_data, const uint32_t other_size, const IOTDeviceType other_DevType, const uint32_t other_DevID,
	const ControlBase *other_ctl, const DeviceAddress *other_address,
	const uint32_t other_overtime, const uint32_t other_QueueOverTime, const uint32_t other_nextInterval
	)
{
	if( !this->ctl
		|| !other_ctl
		)
	{
		return false;
	}

	if( other_size != size)
		return false;

	if( other_DevID != DevID )
		return false;

	if( other_LinkID != LinkID )
		return false;

	if( other_DevType != DevType )
		return false;

	if( other_nextInterval != nextInterval )
		return false;

	if( other_QueueOverTime != QueueOverTime )
		return false;

	if( other_overtime != overtime )
		return false;

	if( address || other_address ) // 只要有一个address有值则进行address比较
	{
		if( !GSIOTClient::Compare_ControlAndAddress( other_ctl, other_address, ctl, address ) )
			return false;
	}
	else // address empty
	{
		if( !GSIOTClient::Compare_Control( other_ctl, ctl ) )
			return false;

		switch( ctl->GetType() )
		{
		case IOT_DEVICE_RS485:
			{
				const RS485DevControl *rsctlA = (RS485DevControl*)ctl;
				const RS485DevControl *rsctlB = (RS485DevControl*)other_ctl;

				if( !rsctlA->IsSameAllAddress( *rsctlB ) )
				{
					return false;
				}
			}
			break;

		default:
			return false;
		}
	}

	if( other_data && data )
	{
		if( 0!=memcmp( other_data, data, size ) )
		{
			return false;
		}
	}
	else if( !other_data && !data )
	{
	}
	else
	{
		return false;
	}

	return true;
}


//////////////////////////////////////////////////////////////////////////


CMsgCurCmd::CMsgCurCmd()
	:m_CurCmd(NULL)
{
	m_GSIOTBoardVer_ts = 0;
	m_ts_lastSend = timeGetTime();
	m_prevInterval = 0;

	m_MODSysSetInfo_TXCtl_ts = 0;
	m_MODSysSetInfo_RXMod_ts = 0;
	m_MODSysSetInfo_RF_RX_freq_ts = 0;

	m_MODSysSetInfo_TXCtl = -1;
	m_MODSysSetInfo_RXMod = -1;
	m_MODSysSetInfo_RF_RX_freq = -1;
}


CMsgCurCmd::~CMsgCurCmd(void)
{
	Delete_CurCmd_Content(true);
}

void CMsgCurCmd::Delete_CurCmd_Content_spec( SERIALDATABUFFER *CurCmd )
{
	if( CurCmd ) delete CurCmd;
}

std::string CMsgCurCmd::FormatShowForTS( const std::string &strver, const uint32_t *last_ts )
{
	std::string strts;
	if( last_ts && !strver.empty() )
	{
		const uint32_t tsspan = (timeGetTime()-(*last_ts))/1000;
		if( tsspan > 99 )
		{
			return ( strver + std::string(" (>99s)") );
		}
		else if( tsspan > 1 )
		{
			char buf[64] = {0};
			snprintf( buf, sizeof(buf), " (<%ds)", tsspan );
			return ( strver + std::string(buf) );
		}
		else
		{
			return ( strver + std::string(" (<1s)") );
		}
	}

	return strver;
}

void CMsgCurCmd::SetGSIOTBoardVer( const std::string &ver )
{
	gloox::util::MutexGuard mutexguard( m_mutex_CurCmd );

	m_GSIOTBoardVer = ver;
	m_GSIOTBoardVer_ts = timeGetTime();
}

std::string CMsgCurCmd::GetGSIOTBoardVer( uint32_t *last_ts )
{
	gloox::util::MutexGuard mutexguard( m_mutex_CurCmd );

	if( last_ts )
	{
		*last_ts = m_GSIOTBoardVer_ts;
	}

	return m_GSIOTBoardVer;
}

void CMsgCurCmd::Set_MODSysSetInfo( const defMODSysSetInfo MODSysSetInfo, const int val )
{
	gloox::util::MutexGuard mutexguard( m_mutex_CurCmd );

	switch( MODSysSetInfo )
	{
	case defMODSysSetInfo_TXCtl:
		m_MODSysSetInfo_TXCtl = val;
		m_MODSysSetInfo_TXCtl_ts = timeGetTime();
		break;

	case defMODSysSetInfo_RXMod:
		m_MODSysSetInfo_RXMod = val;
		m_MODSysSetInfo_RXMod_ts = timeGetTime();
		break;
		
	case defMODSysSetInfo_RF_RX_freq:
		m_MODSysSetInfo_RF_RX_freq = val;
		m_MODSysSetInfo_RF_RX_freq_ts = timeGetTime();
		break;

	default:
		break;
	}
}

std::string CMsgCurCmd::Get_MODSysSetInfo( const defMODSysSetInfo MODSysSetInfo, uint32_t *last_ts, bool commboget, int *getval )
{
	gloox::util::MutexGuard mutexguard( m_mutex_CurCmd );

	std::string str;

	switch( MODSysSetInfo )
	{
	case defMODSysSetInfo_TXCtl:
		if( last_ts )
		{
			*last_ts = m_MODSysSetInfo_TXCtl_ts;
		}

		if( getval )
		{
			*getval = m_MODSysSetInfo_TXCtl;
		}

		if( m_MODSysSetInfo_TXCtl>=0 )
		{
			str = m_MODSysSetInfo_TXCtl ? "红外发送模式":"红外学习模式";

			if( m_MODSysSetInfo_RXMod>=0 && 0==m_MODSysSetInfo_TXCtl )
			{
				str += ", ";
				str += m_MODSysSetInfo_RXMod ? "原始数据格式":"编码数据格式";
			}
		}
		break;

	case defMODSysSetInfo_RXMod:
		if( last_ts )
		{
			*last_ts = m_MODSysSetInfo_RXMod_ts;
		}

		if( getval )
		{
			*getval = m_MODSysSetInfo_RXMod;
		}

		if( m_MODSysSetInfo_RXMod>=0 && 0==m_MODSysSetInfo_TXCtl )
		{
			str += m_MODSysSetInfo_RXMod ? "原始数据格式":"编码数据格式";
		}
		break;

	case defMODSysSetInfo_RF_RX_freq:
		if( last_ts )
		{
			*last_ts = m_MODSysSetInfo_RF_RX_freq_ts;
		}

		if( getval )
		{
			*getval = m_MODSysSetInfo_RF_RX_freq;
		}

		if( m_MODSysSetInfo_RF_RX_freq>=0 )
		{
			str += defFreq_315==m_MODSysSetInfo_RF_RX_freq ? "无线接收315MHz":(defFreq_433==m_MODSysSetInfo_RF_RX_freq ? "无线接收433MHz":"");
		}
		break;

	default:
		break;
	}

	return str;
}

bool CMsgCurCmd::Is_CurCmd( ControlBase *DoCtrl )
{
	if( !DoCtrl )
	{
		return false;
	}

	if( IOT_DEVICE_RS485 != DoCtrl->GetType() ) // 目前只处理RS485
		return false;

	bool isDo = false;

	uint32_t command = ((RS485DevControl*)DoCtrl)->GetCommand();

	// 目前只有RS485读命令才需要控制当前只有一个在执行
	if( !RS485DevControl::IsReadCmd(command) )
	{
		return false;
	}

	//
	m_mutex_CurCmd.lock();

	if( !m_CurCmd )
	{
		m_mutex_CurCmd.unlock();
		return false;
	}

	const IOTDeviceType device_type = m_CurCmd->DevType;
	const uint32_t device_id = m_CurCmd->DevID;
	const defLinkID LinkID = m_CurCmd->LinkID;
	const uint32_t addr = m_CurCmd->address?m_CurCmd->address->GetAddress():0;  
	//jyc20170302 addr=0 have trouble
	SERIALDATABUFFER *delbuf = NULL;

	// 时间超时判断
	const bool curIsOverTime = m_CurCmd->IsOverTime();
	if( curIsOverTime )
	{
		uint32_t DevID = m_CurCmd->ctl?((RS485DevControl*)m_CurCmd->ctl)->GetDeviceid():0;
		uint32_t cmd = m_CurCmd->ctl?((RS485DevControl*)m_CurCmd->ctl)->GetCommand():0;

		LOGMSG( "SerialMessage::Is_CurCmd overtime, Link%d dev(%d,%d), ProtDevID=%d, cmd=%d, addr=%d\r\n", LinkID, device_type, device_id, DevID, cmd, addr );

		delbuf = m_CurCmd;
		m_CurCmd = NULL;
	}

	if( m_CurCmd ) isDo = true;

	m_mutex_CurCmd.unlock();

	Delete_CurCmd_Content_spec( delbuf );

	IGSClientExFunc *client = g_SYS_GetGSIOTClient();
	if( client && curIsOverTime )
	{
		client->OnTimeOverForCmdRecv( LinkID, device_type, device_id, addr );
	}

	return isDo;
}

bool CMsgCurCmd::Set_CurCmd( ControlBase *DoCtrl, SERIALDATABUFFER *CurCmd )
{
	if( !DoCtrl )
	{
		return false;
	}

	if( IOT_DEVICE_RS485 != DoCtrl->GetType() ) // 目前只处理RS485
		return false;

	bool doSet = false;

	uint32_t command = ((RS485DevControl*)DoCtrl)->GetCommand();

	// 目前只有RS485读命令才需要控制当前只有一个在执行
	if( RS485DevControl::IsReadCmd(command) )
	{
		doSet = true;
	}

	if( !doSet )
		return false;

	//
	m_mutex_CurCmd.lock();

	SERIALDATABUFFER *delbuf = m_CurCmd;

	m_CurCmd = CurCmd;
	if( m_CurCmd ) m_CurCmd->SetNowTime();
	m_mutex_CurCmd.unlock();

	Delete_CurCmd_Content_spec( delbuf );

	return true;
}

void CMsgCurCmd::Get_CurCmd( ControlBase *DoCtrl, SERIALDATABUFFER **CurCmd )
{
	*CurCmd = NULL;

	if( !DoCtrl )
	{
		return ;
	}

	if( IOT_DEVICE_RS485 != DoCtrl->GetType() ) // 目前只处理RS485
		return;

	bool doGet = false;

	uint32_t command = ((RS485DevControl*)DoCtrl)->GetCommand();

	// 目前只有RS485读命令才需要控制当前只有一个在执行
	// 包括读正常返回和读异常返回
	if( RS485DevControl::IsReadCmd(command)
		|| RS485DevControl::IsReadCmd_Ret(command) )
	{
		doGet = true;
	}

	if( !doGet )
		return ;

	// 
	m_mutex_CurCmd.lock();

	if( !m_CurCmd )
	{
		m_mutex_CurCmd.unlock();
		return ;
	}

	*CurCmd = m_CurCmd;
	m_CurCmd = NULL;  
	m_mutex_CurCmd.unlock();
}

void CMsgCurCmd::Delete_CurCmd_Content( bool lock=true )
{
	if( lock ) m_mutex_CurCmd.lock();

	Delete_CurCmd_Content_spec( m_CurCmd );
	m_CurCmd = NULL;

	if( lock ) m_mutex_CurCmd.unlock();
}
