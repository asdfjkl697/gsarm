#include "CommLink.h"
#include "NetModule.h"
#include "RS485DevControl.h"

//=====================================================


// 设置字符串格式的参数
void CommLinkCfg_heartbeat_param::setstrfmt( const std::string &str_heartbeat_param )
{
	if( str_heartbeat_param.empty() )
	{
		return;
	}

	std::vector<std::string> paramlist;
	split( str_heartbeat_param, paramlist );

	if( paramlist.empty() )
	{
		return;
	}

	if( paramlist[0] == "SPECADDR" )
	{
		if( paramlist.size() < 4 )
		{
			return;
		}

		this->hb_deviceType = (IOTDeviceType)atoi( paramlist[1].c_str() );
		this->hb_deviceId = atoi( paramlist[2].c_str() );
		this->hb_address = atoi( paramlist[3].c_str() );
	}
}

// 获取字符串格式的参数
std::string CommLinkCfg_heartbeat_param::getstrfmt( const defCommLinkHeartbeatType heartbeat_type ) const
{
	char buf[256] = {0};

	// fmt: 英文逗号隔开：
	// 格式类型字符,内容
	// SPECADDR,设备类型,设备ID,地址单元

	if( defCommLinkHeartbeatType_SpecRdAddr == heartbeat_type )
	{
		snprintf( buf, sizeof(buf), "SPECADDR,%d,%d,%d", this->hb_deviceType, this->hb_deviceId, this->hb_address );
	}
	
	return std::string(buf);
}

void CommLinkCfg::set_default()
{
	lastCfgTs = 0;
	id = 0;
	name = "link"; //jyc name
	enable = 1;
	link_type = defCommLinkType_TCP;
	trans_mod = defTransMod_GSIOT;
	heartbeat_type = defCommLinkHeartbeatType_GSIOT;
	heartbeat_param.set_default();

	memset( &param, 0, sizeof(param) );

	param_str[0] = "";
	param_str[1] = "";

	switch(link_type)
	{
	case defCommLinkType_TCP:
		{
			CCommLinkRun_TCP::setparam_ip( this, "192.168.0.201" );
			CCommLinkRun_TCP::setparam_port( this, defCommLink_DefaultTCPPort );
		}
		break;

	default:
		break;
	}
}

// 心跳配置是否正常
bool CommLinkCfg::isHeartbeatCfgOK( const GSIOTDevice *dev ) const
{
	if( CommLinkCfg::isHeartbeatUserDefine( this->heartbeat_type ) )
	{
		if( !dev )
		{
			return false;
		}

		if( !dev->GetEnable() )
		{
			return false;
		}

		if( dev->GetLinkID() != this->id )
		{
			return false;
		}

		if( !this->heartbeat_param.isCfged() )
		{
			return false;
		}
	}

	return true;
}

bool CommLinkCfg::operator == ( const CommLinkCfg &other ) const
{
	if( this->link_type != other.link_type )
		return false;

	switch(this->link_type)
	{
	case defCommLinkType_TCP:
		{
			if( CCommLinkRun_TCP::getparam_ip(this) == CCommLinkRun_TCP::getparam_ip(&other)
				&& CCommLinkRun_TCP::getparam_port(this) == CCommLinkRun_TCP::getparam_port(&other)
				)
			{
				return true;
			}
		}
		break;

	case defCommLinkType_COM:
	case defCommLinkType_UDP:
	default:
		break;
	}

	return false;
}

std::string CommLinkCfg::getstr_link_type( const defCommLinkType link_type )
{
	switch(link_type)
	{
	case defCommLinkType_COM:
		return std::string("COM");

	case defCommLinkType_TCP:
		return std::string("TCPClient");

	case defCommLinkType_UDP:
		return std::string("UDPClient");

	default:
		break;
	}

	return std::string("");
}

std::string CommLinkCfg::getstr_trans_mod( const defTransMod trans_mod )
{
	switch(trans_mod)
	{
	case defTransMod_Transparent:
		return std::string("数据透传");

	case defTransMod_GSIOT:
		return std::string("主控协议");

	default:
		break;
	}

	return std::string("");
}

std::string CommLinkCfg::getstr_heartbeat_type( const defCommLinkHeartbeatType heartbeat_type )
{
	switch(heartbeat_type)
	{
	case defCommLinkHeartbeatType_Null:
		return std::string("无");

	case defCommLinkHeartbeatType_GSIOT:
		return std::string("主控心跳");
		
	case defCommLinkHeartbeatType_SpecRdAddr:
		return std::string("自定义");

	default:
		break;
	}

	return std::string("");
}

std::string CommLinkCfg::getstr_cfgdesc( const CommLinkCfg *cfg )
{
	if( cfg )
	{
		char buf[256];
		memset(buf,0,256);

		switch(cfg->link_type)
		{
		case defCommLinkType_TCP:
			{
				snprintf( buf, sizeof(buf), "%s:%d", CCommLinkRun_TCP::getparam_ip( cfg ).c_str(), CCommLinkRun_TCP::getparam_port(cfg) );

				return std::string(buf);
			}
			break;

		default:
			break;
		}
	}

	return std::string("");
}


//=====================================================


CCommLinkRun::CCommLinkRun( CommLinkCfg *refcfg )
	: m_refcfg(refcfg), m_cfg(*refcfg), m_doReconnect(0), m_ConnectState(defConnectState_Null), m_lockProc(false), m_lockInfoUse(false), m_lastWork(0)
{
	m_lastSend = timeGetTime();
	m_lastSendHeartbeat = m_lastSend;
	m_lastRecv = m_lastSend;
	m_lastConnect = 0;
	m_isCfgChanged = false;

	m_hb_device = NULL;
}

CCommLinkRun::~CCommLinkRun(void)
{
	this->set_hb_device( NULL, 0 );
}

void CCommLinkRun::Module_Init()
{
	NetModule_Init();
}

void CCommLinkRun::Module_UnInit()
{
	NetModule_UnInit();
}

std::string CCommLinkRun::getstr_enable() const
{
	if( !m_refcfg )
		return std::string("配置异常");

	std::string str = m_refcfg->enable ? "启用":"禁用";

	if( m_refcfg->lastCfgTs != m_cfg.lastCfgTs && m_refcfg->enable != m_cfg.enable )
	{
		str += "...";
	}

	return str;
}

std::string CCommLinkRun::getstr_ConnectState() const
{
	if( m_cfg.enable )
	{
		std::string str = g_GetConnectStateStr( get_ConnectState() );

		if( IsOpen() && Heartbeat_isRecvTimeover() )
		{
			str += "(心跳超时)";
		}

		if( get_doReconnect() )
		{
			str += "(重连)";
		}

		return str;
	}

	return std::string("");
}

void CCommLinkRun::Get_CurCmd( ControlBase *DoCtrl, SERIALDATABUFFER **CurCmd )
{
	this->m_MsgCurCmd.Get_CurCmd( DoCtrl, CurCmd );
}

bool CCommLinkRun::IsOKReconnectionInterval() const
{
	const uint32_t CommLinkConnectInterval = RUNCODE_Get(defCodeIndex_CommLinkConnectInterval);

	return ( 0==m_lastConnect || (timeGetTime()-m_lastConnect)>(CommLinkConnectInterval*1000) );
}

void CCommLinkRun::resetConnectTime()
{
	m_lastConnect = timeGetTime();
}

// 链路状态转换成链路上设备的可用状态
defUseable CCommLinkRun::get_all_useable_state_ForDevice() const
{
	if( !m_cfg.enable )
		return defUseable_Err;

	switch( get_ConnectState() )
	{
	case defConnectState_Null:
		return defUseable_OK;//defUseable_Null;

	case defConnectState_Connecting:
		return defUseable_ErrNet;

	case defConnectState_Connected:
		{
			if( Heartbeat_isRecvTimeover() )
			{
				return defUseable_ErrNoData;
			}

			return defUseable_OK;
		}

	case defConnectState_Disconnected:
		return defUseable_ErrNet;
	}

	return defUseable_Err;
}

void CCommLinkRun::resetTsState()
{
	m_lastSend = timeGetTime();
	m_lastSendHeartbeat = m_lastSend;
	m_lastRecv = m_lastSend;
}

// 只要有发送就可以
void CCommLinkRun::set_lastSend()
{
	this->m_lastSend = timeGetTime();
}

// 只要有发送就可以
void CCommLinkRun::set_lastSendHeartbeat()
{
	this->m_lastSendHeartbeat = timeGetTime();
}

// 是否需要发送心跳
bool CCommLinkRun::Heartbeat_isNeedSend() const
{
	if( !m_cfg.enable )
		return false;

	if( defCommLinkHeartbeatType_Null == m_cfg.heartbeat_type )
		return false;

	// 最近时间内有发送或有收到就不需要发送心跳

	const uint32_t CommLinkHb_SendInterval = RUNCODE_Get(defCodeIndex_CommLinkHb_SendInterval) * 1000;

	if( timeGetTime() - this->m_lastRecv < CommLinkHb_SendInterval )
		return false;

	if( timeGetTime() - this->m_lastSendHeartbeat < CommLinkHb_SendInterval )
		return false;

	return true;
}

// 是否心跳超时
bool CCommLinkRun::Heartbeat_isRecvTimeover() const
{
	if( !m_cfg.enable )
		return false;

	if( defCommLinkHeartbeatType_Null == m_cfg.heartbeat_type )
		return false;

	const uint32_t CommLinkHb_RecvTimeover = RUNCODE_Get(defCodeIndex_CommLinkHb_RecvTimeover) * 1000;

	if( timeGetTime() - this->m_lastRecv > CommLinkHb_RecvTimeover )
	{
		return true;
	}

	return false;
}

void CCommLinkRun::set_hb_device( const GSIOTDevice *const hb_device_src, const uint32_t hb_address  )
{
	if( m_hb_device )
	{
		delete m_hb_device;
		m_hb_device = NULL;
	}

	if( hb_device_src )
	{
		m_hb_device = hb_device_src->clone(false);

		if( m_hb_device )
		{
			switch( hb_device_src->getType() )
			{
			case IOT_DEVICE_RS485:
				{
					RS485DevControl *rsCtl = (RS485DevControl*)m_hb_device->getControl();

					rsCtl->AddressQueueChangeToOneAddr( hb_address );
				}
				break;
			}
		}
	}
}

//=====================================================


CCommLinkRun_TCP::CCommLinkRun_TCP( CommLinkCfg *cfg )
	: CCommLinkRun(cfg), m_linkDevice(NULL)
{
}

CCommLinkRun_TCP::~CCommLinkRun_TCP(void)
{
	Close();
}

bool CCommLinkRun_TCP::IsNeedReconnectForCfgChanged( CommLinkCfg *oldcfg, CommLinkCfg *newcfg ) const
{
	if( !oldcfg || !newcfg )
		return false;

	if( getparam_ip( oldcfg ) != getparam_ip( newcfg ) )
		return true;

	if( getparam_port( oldcfg ) != getparam_port( newcfg ) )
		return true;

	return false;
}

bool CCommLinkRun_TCP::IsOpen() const
{
	return ( m_linkDevice ? true:false );
}

bool CCommLinkRun_TCP::Open()
{
	m_ConnectState = defConnectState_Connecting;
	resetConnectTime();

	m_linkDevice = NetModule_Open( getparam_ip( &m_cfg ).c_str(), getparam_port(&m_cfg) );

	if( m_linkDevice )
	{
		m_doReconnect = 0;
	}
	else
	{
		m_ConnectState = defConnectState_Disconnected;
	}

	return ( m_linkDevice ? true:false );
}

bool CCommLinkRun_TCP::Close()
{
	if( m_linkDevice )
	{
		m_ConnectState = defConnectState_Disconnected;

		NetModule_Close( m_linkDevice );
		m_linkDevice = NULL;
	}

	return true;
}

int	CCommLinkRun_TCP::Write(void* buffer,unsigned long buflen)
{
	//g_PrintfByte( (unsigned char*)buffer, buflen, "CCommLinkRun_TCP Write" );

	const int ret = NetModule_Write( m_linkDevice, buffer, buflen );

	this->set_lastSend(); // 只要有发送就可以，不需要判断返回

	return ret;
}

int	CCommLinkRun_TCP::Read(void* buffer,unsigned long buflen)
{
	const int ret = NetModule_Read( m_linkDevice, buffer, buflen );

	if( ret > 0 )
	{
		this->m_lastRecv = timeGetTime();
	}

	return ret;
}

