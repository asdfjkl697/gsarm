#pragma once
#include "common.h"
#include "SerialDataBuffer.h"
#include "gsiot/GSIOTDevice.h"

#define defCommLink_param_max_int 6 // int���͵Ĳ�������
#define defCommLink_param_max_str 2 // str���͵Ĳ�������

#define defCommLink_DefaultTCPPort 8089

// ��·����
enum defCommLinkType
{
	defCommLinkType_Null		= 0, // ��
	defCommLinkType_COM			= 1, // ����
	defCommLinkType_TCP			= 2, // TCP�ͻ�������
	defCommLinkType_UDP			= 3, // UDP�ͻ�������
};

enum defCommLinkHeartbeatType
{
	defCommLinkHeartbeatType_Null		= 0, // ��
	defCommLinkHeartbeatType_GSIOT		= 1, // ����Э������
	defCommLinkHeartbeatType_SpecRdAddr	= 2, // ָ��һ����ַ����ȡ�����ݣ��õ������Ӧ�ķ�ʽ��������
};

// �������
struct CommLinkCfg_heartbeat_param
{
	IOTDeviceType hb_deviceType;
	uint32_t hb_deviceId;
	uint32_t hb_address;

	CommLinkCfg_heartbeat_param()
	{
		set_null();
	}

	void set_null()
	{
		hb_deviceType = IOT_DEVICE_Unknown;
		hb_deviceId = 0;
		hb_address = 0;
	}

	void set_default()
	{
		set_null();
	}

	// �Ƿ�������
	bool isCfged() const
	{
		return ( IOT_DEVICE_Unknown != hb_deviceType && 0!=hb_deviceId );
	}

	void setstrfmt( const std::string &str_heartbeat_param );
	std::string getstrfmt( const defCommLinkHeartbeatType heartbeat_type ) const;
};

// ��·����
struct CommLinkCfg
{
	uint32_t lastCfgTs;				// ���ø��±�־
	defLinkID id;					// ��·ID
	std::string name;				// ��·���
	int enable;						// ���ñ�־
	defCommLinkType link_type;		// ��·����
	defTransMod trans_mod;			// ����ģʽ
	defCommLinkHeartbeatType heartbeat_type;				// ��������
	CommLinkCfg_heartbeat_param heartbeat_param;			// �������
	int param[defCommLink_param_max_int];					// ԭʼ�������飬�����ڼ̳�����
	std::string param_str[defCommLink_param_max_str];		// ԭʼ�������飬�����ڼ̳�����

public:
	CommLinkCfg()
	{
		lastCfgTs = 0;
		id = 0;
		enable = 0;
		link_type = defCommLinkType_Null;
		trans_mod = defTransMod_Transparent;
		heartbeat_type = defCommLinkHeartbeatType_Null;
		memset( &param, 0, sizeof(param) );
	}

	// �Ƿ�Ϊ�Զ���������������
	static bool isHeartbeatUserDefine( const defCommLinkHeartbeatType heartbeat_type )
	{
		return ( defCommLinkHeartbeatType_SpecRdAddr == heartbeat_type );
	}

	void set_default();
	bool isHeartbeatCfgOK( const GSIOTDevice *dev ) const;

	bool operator == ( const CommLinkCfg &other ) const;
	static std::string getstr_link_type( const defCommLinkType link_type );
	static std::string getstr_trans_mod( const defTransMod trans_mod );
	static std::string getstr_heartbeat_type( const defCommLinkHeartbeatType heartbeat_type );
	static std::string getstr_cfgdesc( const CommLinkCfg *cfg );

	void refreshCfgTs()
	{
		lastCfgTs++;
	}
};

// ��·����
struct CommLinkRelation
{
	IOTDeviceType device_type;		// ����·�������豸����
	int device_id;					// ����·�������豸ID
	defLinkID link_id;				// ���豸��������·ID
	defLinkID link_id_bak;			// ���豸�����ı�����·ID, Ԥ��

	// ����Ԥ������param

	CommLinkRelation()
	{
		device_type = IOT_DEVICE_Unknown;
		device_id = 0;
		link_id = 0;
		link_id_bak = 0;
	}

	CommLinkRelation( IOTDeviceType in_device_type, int in_device_id, defLinkID in_link_id )
		: device_type(in_device_type), device_id(in_device_id), link_id(in_link_id), link_id_bak(0)
	{
	}
};

// ��·���в�����
class CCommLinkRun
{
public:
	friend class CCommLinkManager;

	CCommLinkRun( CommLinkCfg *refcfg );
	virtual ~CCommLinkRun(void);

public:
	static void Module_Init();
	static void Module_UnInit();
	virtual bool IsNeedReconnectForCfgChanged( CommLinkCfg *oldcfg, CommLinkCfg *newcfg ) const = 0;
	virtual bool IsOpen() const = 0;
	virtual bool Open() = 0;
	virtual bool Close() = 0;
	virtual int	Write(void* buffer,unsigned long buflen) = 0;
	virtual int	Read(void* buffer,unsigned long buflen) = 0;
	virtual int get_linkrun_type() const = 0;
	virtual std::string getstr_enable() const;
	virtual std::string getstr_ConnectState() const;

	void set_doReconnect( uint32_t doReconnect = 1, const char *callinfo=NULL )
	{
		if( doReconnect ) LOGMSG( "CommLink%d set_doReconnect %s\r\n", m_cfg.id, callinfo?callinfo:"" );

		m_doReconnect = doReconnect;
	}
	uint32_t get_doReconnect() const
	{
		return m_doReconnect;
	}

	bool IsOKReconnectionInterval() const;
	void resetConnectTime();

	void set_ConnectState( defConnectState state )
	{
		m_ConnectState = state;
	}
	defConnectState get_ConnectState() const
	{
		if( IsOpen() )
			return defConnectState_Connected;

		return ( defConnectState_Connected==m_ConnectState ? defConnectState_Disconnected:m_ConnectState );
	}
	defUseable get_all_useable_state_ForDevice() const;
	void resetTsState();

	void set_lastSend();
	void set_lastSendHeartbeat();
	bool Heartbeat_isNeedSend() const;
	bool Heartbeat_isRecvTimeover() const;

	const CommLinkCfg& get_cfg()
	{
		return m_cfg;
	}

	bool pop_CfgChanged_For_check_cfg()
	{
		if( m_isCfgChanged )
		{
			m_isCfgChanged = false;

			return true;
		}

		return false;
	}

	void check_cfg()
	{
		try
		{
			if( m_refcfg && m_refcfg->lastCfgTs!=m_cfg.lastCfgTs )
			{
				resetTsState();

				if( IsNeedReconnectForCfgChanged( m_refcfg, &m_cfg ) )
				{
					set_doReconnect( 1, "cfg changed" );
				}

				m_cfg = *m_refcfg;

				m_isCfgChanged = true;
			}
		}
		catch(...)
		{
		}
	}

	void Get_CurCmd( ControlBase *DoCtrl, SERIALDATABUFFER **CurCmd );

	CMsgCurCmd &GetMsgCurCmdObj()
	{
		return m_MsgCurCmd;
	}

	GSIOTDevice* get_hb_device() const
	{
		return m_hb_device;
	}
	void set_hb_device( const GSIOTDevice *const hb_device_src, const uint32_t hb_address );

protected:
	// lock
	bool m_lockProc;				// ����ʹ����
	bool m_lockInfoUse;				// ����Ϣʹ����
	uint32_t m_lastWork;			// ���һ�δ���ʱ���

	// state
	uint32_t m_doReconnect;			// �Ƿ��������
	defConnectState m_ConnectState;	// ����״̬
	uint32_t m_lastConnect;
	uint32_t m_lastSend;			// ���һ�η������ʱ���
	uint32_t m_lastSendHeartbeat;	// ���һ�η����������ʱ���
	uint32_t m_lastRecv;			// ���һ�ν������ʱ���
	bool m_isCfgChanged;			// ����check_cfg�������Ƿ��з���仯���´�check_cfg�ָ�λ
	CommLinkCfg m_cfg;
	CommLinkCfg *m_refcfg;
	CMsgCurCmd m_MsgCurCmd;

	// Heartbeat
	GSIOTDevice *m_hb_device;		// ͸��ʱ������������豸
};


// TCP��·
class CCommLinkRun_TCP : public CCommLinkRun
{
public:
	CCommLinkRun_TCP( CommLinkCfg *cfg );
	virtual ~CCommLinkRun_TCP(void);

	virtual bool IsNeedReconnectForCfgChanged( CommLinkCfg *oldcfg, CommLinkCfg *newcfg ) const;
	virtual bool IsOpen() const;
	virtual bool Open();
	virtual bool Close();
	virtual int	Write(void* buffer,unsigned long buflen);
	virtual int	Read(void* buffer,unsigned long buflen);
	virtual int get_linkrun_type() const
	{
		return defCommLinkType_TCP;
	}

public:
	static void setparam_ip( CommLinkCfg *cfg, const std::string &ip )
	{
		if( !cfg ) return;
		cfg->param_str[index_ip] = ip;
	}
	static std::string getparam_ip( const CommLinkCfg *cfg )
	{
		if( !cfg ) return std::string("");
		return cfg->param_str[index_ip];
	}

	static void setparam_port( CommLinkCfg *cfg, int port )
	{
		if( !cfg ) return;
		cfg->param[index_port] = 0==port?defCommLink_DefaultTCPPort:port;
	}
	static int getparam_port( const CommLinkCfg *cfg )
	{
		if( !cfg ) return defCommLink_DefaultTCPPort;
		return cfg->param[index_port];
	}

protected:
	void* m_linkDevice;

protected:
	enum CommLink_IndexDef_TCP
	{
		// int
		index_port	= 0, // port���ڲ���λ������

		// str
		index_ip	= 0, // ip���ڲ���λ������
	};
};
