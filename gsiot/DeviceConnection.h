#ifndef DEVICECONNECTION_H
#define DEVICECONNECTION_H

#include "typedef.h"
#include "IDeviceHandler.h"
#include "IRFReadHandler.h"
#include "common.h"
#include "IRFDeviceHandler.h"
#include "ICANDeviceHandler.h"
#include "ISerialPortHandler.h"
#include "SerialMessage.h"
#include "ITimerHandler.h"
#include "DeviceManager.h"
//#include "GSMProcess.h"
#include "CommLinkManager.h"
#include "IGSMessageHandler.h"
#include "HeartbeatMon.h"

enum defMODSysSet
{
	defMODSysSet_Null = 0,

	defMODSysSet_QueryAll,					// 查询所有

	defMODSysSet_IR_QueryAll,				// IR 查询所有红外相关

	defMODSysSet_IR_TXCtl_Query,			// IR 收发控制 - 查询
	defMODSysSet_IR_TXCtl_RX,				// IR 收发控制 - 设为收
	defMODSysSet_IR_TXCtl_TX,				// IR 收发控制 - 设为发

	defMODSysSet_IR_RXMod_Query,			// IR 收模式查询
	defMODSysSet_IR_RXMod_original,			// IR 收原始数据
	defMODSysSet_IR_RXMod_code,				// IR 收编码数据

	defMODSysSet_RF_RX_freq_Query,			// RF 频率设置查询
	defMODSysSet_RF_RX_freq_315,			// RF 频率设置315
	defMODSysSet_RF_RX_freq_433,			// RF 频率设置433
};

class DeviceConnection:public ISerialPortHandler,public ITimerHandler
{
private:
	IDeviceHandler *m_handler;
	ICommLinkNotifyHandler *m_ICommLinkNotifyHandler;
	bool isRunning;
	int m_port;

	DeviceManager *devManager;
	//20160607 GSMProcess m_GSM;

	uint32_t m_RFCode;
	int m_RFReadCount;
	std::list<IRFReadHandler *> m_RFHandlerQueue; 
	std::list<IRFDeviceHandler *> m_RFDeviceHandlerQueue; 
	std::list<ICANDeviceHandler *> m_CANDeviceHandlerQueue; 
	SerialMessage *m_serial_msg; 
	int m_threadRunning;

	gloox::util::Mutex m_mutex_devcon;
	int m_CommLinkThread_id;	// 链路线程ID
	int m_CommLinkThread_allowConnectNum; // 链路线程运行中已经允许进行连接的数量

protected:
	void DecodeRxb8Data( CHeartbeatGuard *phbGuard, CCommLinkRun *CommLink, defFreq freq, uint8_t* const srcbuf, const uint32_t srcbufsize );
	void DecodeCC1101Data(CCommLinkRun *CommLink, uint8_t *buf,uint32_t size);
	void DecodeCANData(CCommLinkRun *CommLink, uint8_t *buf,uint32_t size);
	void Decode_RS485Data( CHeartbeatGuard *phbGuard, CCommLinkRun *CommLink, uint8_t* const srcbuf, const uint32_t srcbufsize );
	void Decode_Rxb8Data_original( CHeartbeatGuard *phbGuard, CCommLinkRun *CommLink, const defFreq freq, uint8_t* const srcbuf, const uint32_t srcbufsize );
	void Decode_MOD_IR_RX_code( CHeartbeatGuard *phbGuard, CCommLinkRun *CommLink, uint8_t* const srcbuf, const uint32_t srcbufsize );
	void Decode_MOD_IR_RX_original( CHeartbeatGuard *phbGuard, CCommLinkRun *CommLink, uint8_t* const srcbuf, const uint32_t srcbufsize );
	void Decode_Read_IOT_Ver( CHeartbeatGuard *phbGuard, CCommLinkRun *CommLink, uint8_t* const srcbuf, const uint32_t srcbufsize );
	void Decode_MOD_SYS_get( CHeartbeatGuard *phbGuard, CCommLinkRun *CommLink, uint8_t* const srcbuf, const uint32_t srcbufsize );

	void OnDeviceData_RFSignal( CHeartbeatGuard *phbGuard, CCommLinkRun *CommLink, const RFSignal &signal );
	
public:
	DeviceConnection(IDeviceHandler *handler);
	~DeviceConnection(void);

	CCommLinkManager m_CommLinkMgr;

	int m_usecfg_port;
	int m_usecfg_baudrate;

	void OnCommLinkNotify( ICommLinkNotifyHandler::CLNType type, defLinkID LinkID )
	{
		if( m_ICommLinkNotifyHandler )
		{
			m_ICommLinkNotifyHandler->OnCommLinkNotify( type, LinkID );
		}
	}

	void set_ICommLinkNotifyHandler( ICommLinkNotifyHandler *pICommLinkNotifyHandler )
	{
		m_ICommLinkNotifyHandler = pICommLinkNotifyHandler;
	}

	ICommLinkNotifyHandler* get_ICommLinkNotifyHandler() const
	{
		return m_ICommLinkNotifyHandler;
	}

	DeviceManager* GetDeviceManager()
	{
		return devManager;
	}
	/*20160607
	GSMProcess& GetGSM()
	{
		return m_GSM;
	}*/

	int GetPort()
	{
		return this->m_port;
	}

	int Get_CommLinkThread_id();
	int New_CommLinkThread_id();
	int Get_CommLinkThread_allowConnectNum();
	int New_CommLinkThread_allowConnectNum();

	GSDevViewMod_ GetCurGSDevViewMod() const
	{
		return m_curGSDevViewMod;
	}

	void SetCurGSDevViewMod( GSDevViewMod_ mod )
	{
		m_curGSDevViewMod = mod;
	}

	bool IsCurGSDevViewMod_Debug() const
	{
		return ( GSDevViewMod_Debug == m_curGSDevViewMod );
	}

	bool ReadGSIOTBoardVer( defLinkID LinkID, uint32_t QueueOverTime=defNormMsgOvertime );
	std::string GetGSIOTBoardVer( defLinkID LinkID, uint32_t *last_ts=NULL );

	bool SetRFMod( defLinkID LinkID, defFreq freq, uint8_t enable_pass, uint8_t check_count=3, uint8_t enable_timecheck=0 );
private:
	bool SetRFMod_EncodeSend( defLinkID LinkID, defFreq freq, uint8_t enable_pass, uint8_t check_count=3, uint8_t enable_timecheck=0 );
public:
	bool SendMOD_set( const defMODSysSet cmd, const defLinkID LinkID, const uint32_t QueueOverTime=defNormMsgOvertime );
private:
	bool SendMOD_set_EncodeSend( const defMODSysSet cmd, const defLinkID LinkID, const uint32_t QueueOverTime );
public:
	bool Encode_RFRemote_code( const RFSignal &signal, uint8_t *buf, uint16_t &buflen );
	bool Encode_RFRemote_original( const RFSignal &signal, uint8_t *buf, uint16_t &buflen );
	bool Encode_RFRemote_IR_code( const RFSignal &signal, uint8_t *buf, uint16_t &buflen );
	bool Encode_RFRemote( const RFSignal &signal, uint8_t *buf, uint16_t &buflen );
	defGSReturn SendControl( const IOTDeviceType DevType, const GSIOTDevice *device, const GSIOTObjBase *obj, const uint32_t overtime=defNormSendCtlOvertime, const uint32_t QueueOverTime=defNormMsgOvertime, const uint32_t nextInterval=1 );
	void SendTransparentData( defLinkID LinkID, uint8_t *data, uint32_t size, uint32_t overtime=defNormSendCtlOvertime, uint32_t QueueOverTime=defNormMsgOvertime );
	void SendTransparentData( defLinkID LinkID, const std::string strdata, uint32_t overtime=defNormSendCtlOvertime, uint32_t QueueOverTime=defNormMsgOvertime );
	int SendData(CCommLinkRun *CommLink, unsigned char *sendbuf,int sendsize);
	void OnDataReceived( CHeartbeatGuard *phbGuard, CCommLinkRun *CommLink, uint8_t* const srcbuf, const uint32_t srcbufsize );
	int RecvData(CCommLinkRun *CommLink, unsigned char *buf,int size);
	void Run(int port);
	void AddCommLinkThread( int CommLinkCount );
	void Stoped();
	bool IsStoped();
	bool GetRunning(){
		return this->isRunning;
	}
	int GetPortState() const
	{
		return (this->m_usecfg_port>0) ? ( this->m_port>-1 ? 1: 0 ) : -1;
	}
	std::string GetPortStateStr() const
	{
		const int PortState = this->GetPortState();
		if( PortState>0 )
		{
			return std::string("正常");
		}
		else if( PortState<0 )
		{
			return std::string("无");
		}

		return std::string("异常");
	}
	int GetAllCommunicationState( const bool getCommLink );
	void OnThreadExit();
	bool IsThreadExit()
	{
		return (!m_threadRunning);
	}

	void LoadDevice();
	void LoadDeviceXml();
	int AddController(ControlBase *ctl, const std::string &ver=c_DefaultVer, uint32_t enable=1, GSIOTDevice **outNewDev=NULL);
	void DeviceSaveToFile();
	bool DeleteDevice( GSIOTDevice *iotdevice );
	bool ModifyDevice( GSIOTDevice *iotdevice, uint32_t ModifySubFlag=0 );
	bool ModifyAddress( GSIOTDevice *iotdevice, DeviceAddress *addr );

	ControlBase *GetController( IOTDeviceType deviceType, uint32_t deviceId ) const;
	GSIOTDevice* GetIOTDevice( IOTDeviceType deviceType, uint32_t deviceId ) const;
	
	void Check();
	void OnTimer( CHeartbeatGuard *phbGuard, CCommLinkRun *CommLink, int TimerID );

	void AddRFHandler(IRFReadHandler *handler);
	void RemoveRFHandler(IRFReadHandler *handler);
	void AddRFDeviceHandler(IRFDeviceHandler *handler);
	void RemoveRFDeviceHandler(IRFDeviceHandler *handler);
	void AddCANDeviceHandler(ICANDeviceHandler *handler);
	void RemoveCANDeviceHandler(ICANDeviceHandler *handler);
	void GetRFDeviceInfo(uint32_t productID,uint32_t passCode);
	bool ModifyDevID( GSIOTDevice *iotdevice, uint8_t newID, const std::string &ver, uint8_t SpecAddr=0 );
	bool ModifyDevID_RS485( const defLinkID in_LinkID, IOTDeviceType Type, uint8_t oldID, uint8_t newID, const std::string &ver, uint8_t SpecAddr=0 );
	
	bool OnTest_Trigger( const int devid );

private:
	int SendData_clip(CCommLinkRun *CommLink, unsigned char *buf,int size);

	GSDevViewMod_ m_curGSDevViewMod;
};

#endif
