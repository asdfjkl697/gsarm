#include "DeviceConnection.h"
#include "rs232.h"
//#include <process.h>
#include <pthread.h>
#include "RunCode.h"
#include <utility>

#include <memory> //jyc20160901 for auto_ptr

//#define __stdcall  //20160607
//#define LPVOID void *

#define defTEST_ModbusMod // modbus͸������ģʽ

uint8_t *CommandToByte(uint8_t *buf,uint16_t val)
{
	buf[0] = val >> 8;
	buf[1] = val & 0xFF;
	return buf+2;
}
uint16_t ByteToCommand(uint8_t *buf)
{
	return (buf[1]  & 0xFF) | (buf[0] << 8);
}


//unsigned __stdcall SerialPortThread(LPVOID lpPara)
void *SerialPort_Thread(LPVOID lpPara)
{
	int n;
	int cport_nr = 0;
	unsigned char buf[4096];
	buf[4096-1] = 0;
	DeviceConnection *device = (DeviceConnection *)lpPara;

	uint32_t lastPrintTime = ::timeGetTime()-3600000;
	
	cport_nr = device->GetPort();
	CHeartbeatGuard hbGuard( "Serial" );
	CHeartbeatGuard *phbGuard = &hbGuard;

	while(device->GetRunning())
	{
		hbGuard.alive();

		if( ::timeGetTime()-lastPrintTime > 180000 )
		{
			LOGMSG( "Heartbeat: SerialPortThread(port=%d, br=%d)\r\n", device->m_usecfg_port, device->m_usecfg_baudrate );
			lastPrintTime = ::timeGetTime();
		}
		//jyc20160824
		//device->GetGSM().OnWork(NULL, true);

		if( !device->GetRunning() ) { break; }

		device->OnTimer( phbGuard, NULL, 1 );
		
		if( !device->GetRunning() ) { break; }

		usleep(100000); 

		if( !device->GetRunning() ) { break; }

	    //n = RS232_PollComport(cport_nr, buf+defSerialBufPreLen, 4095-defSerialBufPreLen);
		n = device->RecvData( NULL, buf+defSerialBufPreLen, 4095-defSerialBufPreLen );
		if(n > 0)
        {
			device->OnDataReceived( NULL, NULL, buf+defSerialBufPreLen, n );

			if( !device->GetRunning() ) { break; }

#ifdef _WIN32
			Sleep(50);
#else
			usleep(100000);
#endif

		}
	}

	device->OnThreadExit();
	//return 0;
}


//unsigned __stdcall CommLinkThread(LPVOID lpPara) 
void *CommLink_Thread(LPVOID lpPara) //jyc20160826 test
{
	int n;
	//int cport_nr = 0;
	unsigned char buf[4096];
	buf[4096-1] = 0;
	DeviceConnection *device = (DeviceConnection *)lpPara;
	uint32_t lastPrintTime = ::timeGetTime()-3600000;

	//jyc20160824
	//const int thisThreadId = ::GetCurrentThreadId();
	const int thisThreadId = ::pthread_self(); //jyc20160825add get threadid must test later
	
	const int this_CommLinkThread_No = device->New_CommLinkThread_id();
	int CommLinkThread_allowConnectNum = device->Get_CommLinkThread_allowConnectNum();

	const uint32_t CommLinkThreadMax = RUNCODE_Get(defCodeIndex_CommLinkThreadMax,defRunCodeValIndex_1);
	const uint32_t CommLinkThreadMaxAllow = RUNCODE_Get(defCodeIndex_CommLinkThreadMax,defRunCodeValIndex_2);

	const bool allowConnect = ( CommLinkThread_allowConnectNum<CommLinkThreadMaxAllow || CommLinkThread_allowConnectNum<1 ) && this_CommLinkThread_No%2; // ����һ�����������?ż�����?����ȫ������������
	if( allowConnect )
	{
		CommLinkThread_allowConnectNum = device->New_CommLinkThread_allowConnectNum();
	}

	LOGMSG( "run LinkThread(%d)(max=%d), allowConnect=(%s)(created %d/%d)-ThId%d \n", this_CommLinkThread_No, CommLinkThreadMax, allowConnect?"Y":"N", CommLinkThread_allowConnectNum, CommLinkThreadMaxAllow, thisThreadId );
	snprintf( (char*)buf, sizeof(buf), "LinkTh%d%s-ThId%d", this_CommLinkThread_No, allowConnect?"a":"", thisThreadId );
	CHeartbeatGuard hbGuard( (char*)buf );
	CHeartbeatGuard *phbGuard = &hbGuard;
	CCommLinkRun *pCommLink = NULL;

	while(device->GetRunning())
	{
		hbGuard.alive();
		macHeartbeatGuard_step(1);
				
		CCommLinkAuto_Run_Proc_Get AutoCommLink( device->m_CommLinkMgr, allowConnect );
		CCommLinkRun *pCommLink = AutoCommLink.p();
		
		if( !pCommLink ) 
		{
			macHeartbeatGuard_step(100);			
			usleep(100000);
			continue;
		}
	

		const bool cfgchanged = pCommLink->pop_CfgChanged_For_check_cfg();
		if( cfgchanged )
		{
			macHeartbeatGuard_step(200);
			pCommLink->set_hb_device( NULL, 0 ); //jyc20160824
		}

		if( allowConnect )
		{
			macHeartbeatGuard_step(300);
			
			if( !pCommLink->get_cfg().enable )
			{
				macHeartbeatGuard_step(400);
				pCommLink->Close();
				device->OnCommLinkNotify( ICommLinkNotifyHandler::CLNType_Refresh, pCommLink->get_cfg().id );
				macHeartbeatGuard_step(500);
				continue;
			}

			macHeartbeatGuard_step(600);

			const bool doReconnect = pCommLink->get_doReconnect();
			if( doReconnect )
			{
				macHeartbeatGuard_step(700);

				pCommLink->set_doReconnect(0);

				LOGMSG( "Link%d(%s) doReconnect-ThId%d\r\n", pCommLink->get_cfg().id, pCommLink->get_cfg().name.c_str(), thisThreadId );

				pCommLink->Close();
				device->OnCommLinkNotify( ICommLinkNotifyHandler::CLNType_Refresh, pCommLink->get_cfg().id );
			}

			macHeartbeatGuard_step(800);
			
			const bool isRecvTimeover = pCommLink->Heartbeat_isRecvTimeover();

			if( !pCommLink->IsOpen() || isRecvTimeover ) //jyc20160912 overtimer
			{
				const bool IsOKReconnectionInterval = pCommLink->IsOKReconnectionInterval();

				if( isRecvTimeover && IsOKReconnectionInterval )
				{
					LOGMSG( "Link%d(%s) Heartbeat_isRecvTimeover-ThId%d\r\n", pCommLink->get_cfg().id, pCommLink->get_cfg().name.c_str(), thisThreadId );
				}

				if( doReconnect || IsOKReconnectionInterval )
				{
					macHeartbeatGuard_step(1000);

					if( pCommLink->IsOpen() )
					{
						pCommLink->Close();
						device->OnCommLinkNotify( ICommLinkNotifyHandler::CLNType_Refresh, pCommLink->get_cfg().id );
					}
					else
					{
						pCommLink->resetTsState();
						device->OnCommLinkNotify( ICommLinkNotifyHandler::CLNType_Refresh, pCommLink->get_cfg().id );
					}

					pCommLink->set_ConnectState(defConnectState_Connecting);
					device->OnCommLinkNotify( ICommLinkNotifyHandler::CLNType_Refresh, pCommLink->get_cfg().id );

					const bool openret = pCommLink->Open();
					device->OnCommLinkNotify( ICommLinkNotifyHandler::CLNType_Refresh, pCommLink->get_cfg().id );

					if( !openret )
					{
						macHeartbeatGuard_step(1100);

						device->OnTimer( phbGuard, pCommLink, 1 );
						continue;
					}
				}
			}
		}
		
		macHeartbeatGuard_step(1200);

		if( ::timeGetTime()-lastPrintTime > 180000 )
		{
			LOGMSG( "Heartbeat: SerialPortThread(port=%d, br=%d)\r\n", device->m_usecfg_port, device->m_usecfg_baudrate );
			lastPrintTime = ::timeGetTime();
		}
		

		macHeartbeatGuard_step(1300);

		if( !device->GetRunning() )	{ 
			break; 
		}

		device->OnTimer( phbGuard, pCommLink, 1 );  //onTimer

		if( !device->GetRunning() ) { break; }

		usleep(100000);  /* sleep for 100 milliSeconds */

		if( !device->GetRunning() ) { break; }

		macHeartbeatGuard_step(1400);

		n = device->RecvData( pCommLink, buf+defSerialBufPreLen, 4095-defSerialBufPreLen );
		if(n > 0)
		{
			macHeartbeatGuard_step(1500);

			device->OnDataReceived( phbGuard, pCommLink, buf+defSerialBufPreLen, n );

			if( !device->GetRunning() ) { break; }

			usleep(100000);  /* sleep for 100 milliSeconds */

		}

		macHeartbeatGuard_step(1600);

		// �Ƿ���Ҫ��������
		if( pCommLink->IsOpen() )
		{
			const uint32_t CommLinkHb_SendInterval = RUNCODE_Get(defCodeIndex_CommLinkHb_SendInterval);
			if( pCommLink->Heartbeat_isNeedSend() ) //jyc20160914 test
			{
				macHeartbeatGuard_step(1700);

				switch( pCommLink->get_cfg().heartbeat_type )
				{
				case defCommLinkHeartbeatType_GSIOT:
					{
						device->ReadGSIOTBoardVer( pCommLink->get_cfg().id, (CommLinkHb_SendInterval*1000)+defNormMsgOvertime );
						pCommLink->set_lastSendHeartbeat();

						LOGMSG( "Link%d(%s) Send Heartbeat GSIOT -ThId%d", pCommLink->get_cfg().id, pCommLink->get_cfg().name.c_str(), thisThreadId );
					}
					break;

				case defCommLinkHeartbeatType_SpecRdAddr:
					{
						pCommLink->set_lastSendHeartbeat();

						const IOTDeviceType hb_deviceType = pCommLink->get_cfg().heartbeat_param.hb_deviceType;
						if( IOT_DEVICE_RS485 != hb_deviceType )// Ŀǰ֧�����������豸������
						{
							break;
						}

						const uint32_t hb_deviceId = pCommLink->get_cfg().heartbeat_param.hb_deviceId;
						const uint32_t hb_address = pCommLink->get_cfg().heartbeat_param.hb_address;
						
						// ��ȡ֮ǰ�������豸���ж��Ƿ�������һ�£��Ƿ���Ҫ�ؽ������豸
						GSIOTDevice *hb_device = pCommLink->get_hb_device();

						bool doCreate_hb_device = !hb_device;
						if( hb_device )
						{
							doCreate_hb_device = !hb_device->isDevSelfAndIncludeAddr( hb_deviceType, hb_deviceId, hb_address );
						}

						

						// ��ȡ�Ѵ����������豸
						hb_device = pCommLink->get_hb_device();

						if( !hb_device )
						{
							LOGMSG( "Link%d(%s) do Heartbeat, no hb_dev ptr, type=%d, id=%d -ThId%d\r\n", pCommLink->get_cfg().id, pCommLink->get_cfg().name.c_str(), hb_deviceType, hb_deviceId, thisThreadId );
							break;
						}

						// �����豸������·�뵱ǰ��·��һ��ʱ�����������쳣
						if( hb_device->GetLinkID() != pCommLink->get_cfg().id )
						{
							LOGMSG( "Link%d(%s) do Heartbeat, Hb'LinkID Cfg Err -ThId%d\r\n", pCommLink->get_cfg().id, pCommLink->get_cfg().name.c_str(), thisThreadId );
							break;
						}

						switch( hb_deviceType )
						{
						case IOT_DEVICE_RS485:
							{
								// rs485 proccess

								RS485DevControl *rsCtl = (RS485DevControl*)hb_device->getControl();
								if( !rsCtl )
								{
									LOGMSG( "Link%d(%s) do Heartbeat, ctl err, type=%d, id=%d -ThId%d\r\n", pCommLink->get_cfg().id, pCommLink->get_cfg().name.c_str(), hb_deviceType, hb_deviceId, thisThreadId );
									break;
								}

								DeviceAddress *address = NULL;
								if( 0==hb_address )
								{
									// �����ַ
									address = rsCtl->GetFristAddress();
								}
								else
								{
									// ָ����ַ
									address = rsCtl->GetAddress( hb_address );
								}

								if( !address )
								{
									LOGMSG( "Link%d(%s) do Heartbeat, addr not found, type=%d, id=%d, addr=%d -ThId%d\r\n", pCommLink->get_cfg().id, pCommLink->get_cfg().name.c_str(), hb_deviceType, hb_deviceId, hb_address, thisThreadId );
									break;
								}

								rsCtl->SetCommand( defModbusCmd_Read );
								device->SendControl( hb_deviceType, hb_device, address, defNormSendCtlOvertime, (CommLinkHb_SendInterval*1000)+defNormMsgOvertime );

								LOGMSG( "Link%d(%s) Send Heartbeat SpecRdAddr(%d,%d,%d)-ThId%d", pCommLink->get_cfg().id, pCommLink->get_cfg().name.c_str(), hb_device->getType(), hb_device->getId(), hb_address, thisThreadId );
							}
							break;

						default:
							{
								LOGMSG( "Link%d(%s) Send Heartbeat nosupport devtype=%d -ThId%d", pCommLink->get_cfg().id, pCommLink->get_cfg().name.c_str(), hb_deviceType, thisThreadId );
							}
							break;
						}
					}
					break;

				default:
					break;
				}
			}
		}

		macHeartbeatGuard_step(999998);
	}

	macHeartbeatGuard_step(999999);

	device->OnThreadExit();
	pthread_detach(pthread_self()); //jyc20170722 add
	//return 0;
}

DeviceConnection::DeviceConnection(IDeviceHandler *handler)
	:m_port(-1),m_RFReadCount(0),m_threadRunning(0), m_usecfg_port(-1), m_usecfg_baudrate(-1), m_ICommLinkNotifyHandler(NULL)
{
	m_CommLinkThread_id = 0;
	m_CommLinkThread_allowConnectNum = 0;

	m_curGSDevViewMod = GSDevViewMod_Default;
	
	this->m_handler = handler;
    isRunning = false;

	CCommLinkRun::Module_Init(); //jyc20160830 win diff linux   donot remove

	devManager = new DeviceManager();

	m_CommLinkMgr.Init();

	const defmapCommLinkRelation &CommLinkRelationList = m_CommLinkMgr.Relation_GetList();
	for( defmapCommLinkRelation::const_iterator it = CommLinkRelationList.begin(); it!= CommLinkRelationList.end(); ++it )
	{
		GSIOTDevice *dev = devManager->GetIOTDevice( it->second.device_type, it->second.device_id );
		if( dev )
		{
			dev->SetLinkID( it->second.link_id, false );
		}
	}

	m_serial_msg = new SerialMessage(this);
}

DeviceConnection::~DeviceConnection(void)
{
	LOGMSG("~DeviceConnection\r\n");

	DWORD dwStart = ::timeGetTime();
	while(m_threadRunning)
	{
		if( ::timeGetTime()-dwStart > 20*1000 )
		{
			break;
		}
		usleep(100000);
	}

	if(this->m_port>-1){
	    RS232_CloseComport(this->m_port);
	}
	//delete(devXml);
	delete(devManager);
	//delete(m_serial_msg);//...testFinalDelete �����˳������ͷ�
	//m_CommLinkMgr.UnInit();//...testFinalDelete �����˳������ͷ�

	CCommLinkRun::Module_UnInit();
	LOGMSG("~DeviceConnection end\r\n");
}

// ��ȡ�ۺ�ͨѶ״̬��
//   ����������·ͨѶ״̬
//   Զ����·ͨѶ״̬

// return >0��, 0�쳣, <0��
int DeviceConnection::GetAllCommunicationState( const bool getCommLink )
{
	int enableCount = 0;
	bool allConnectState = true;
	
	const int PortState = this->GetPortState();

	if( !getCommLink )
	{
		return PortState;
	}

	this->m_CommLinkMgr.Run_GetAllStateInfo( enableCount, allConnectState );

	//no remotelink
	if( 0==enableCount )
	{
		return PortState;
	}

	// no locallink
	if( PortState<0 )
	{
		return ( allConnectState?1:0 );
	}

	//locallink or remotelink
	//return ( (allConnectState && 1==PortState) ? 1:0 );
	return ( allConnectState ? 1:0 ); //jyc20170223 modify must connect
}

int DeviceConnection::Get_CommLinkThread_id()
{
	gloox::util::MutexGuard mutexguard( this->m_mutex_devcon );

	return m_CommLinkThread_id;
}

//new linkthread id
int DeviceConnection::New_CommLinkThread_id()
{
	gloox::util::MutexGuard mutexguard( this->m_mutex_devcon );

	return ( ++m_CommLinkThread_id );
}

// get linkthread number
int DeviceConnection::Get_CommLinkThread_allowConnectNum()
{
	gloox::util::MutexGuard mutexguard( this->m_mutex_devcon );

	return m_CommLinkThread_allowConnectNum;
}

//new allow linkthread  jyc20160824
int DeviceConnection::New_CommLinkThread_allowConnectNum()
{
	gloox::util::MutexGuard mutexguard( this->m_mutex_devcon );

	return ( ++m_CommLinkThread_allowConnectNum );
}

void DeviceConnection::Check()
{
	m_serial_msg->Check();
}

void DeviceConnection::OnTimer( CHeartbeatGuard *phbGuard, CCommLinkRun *CommLink, int TimerID )
{
	m_serial_msg->onTimer( phbGuard, CommLink );
}

void DeviceConnection::SendTransparentData( defLinkID LinkID, uint8_t *data, uint32_t size, uint32_t overtime, uint32_t QueueOverTime )
{
	m_serial_msg->doMessage( LinkID, data, size, IOT_DEVICE_Unknown, 0, NULL, NULL, overtime, QueueOverTime );
}

void DeviceConnection::SendTransparentData( defLinkID LinkID, const std::string strdata, uint32_t overtime, uint32_t QueueOverTime )
{
	uint8_t data[1024];
	uint32_t size = g_StringToBuffer( strdata, data, sizeof(data) );

	SendTransparentData( LinkID, data, size, overtime, QueueOverTime );
}

bool DeviceConnection::ReadGSIOTBoardVer( defLinkID LinkID, uint32_t QueueOverTime )
{
	uint8_t sendBuf[MIAN_PORT_BUF_MAX] = {0xBC, 0xAC, 0x02, 0x65, Read_IOT_Ver, 0xAC};
	
	m_serial_msg->doMessage( LinkID, sendBuf, 6, IOT_DEVICE_Unknown, 0, NULL, NULL, defNormSendCtlOvertime, QueueOverTime );

	return true;
}

std::string DeviceConnection::GetGSIOTBoardVer( defLinkID LinkID, uint32_t *last_ts )
{
	if( defLinkID_Local == LinkID )
	{
		return m_serial_msg->GetMsgCurCmdObj().GetGSIOTBoardVer(last_ts);
	}
	else
	{
		CCommLinkAuto_Run_Info_Get AutoCommLink( m_CommLinkMgr, LinkID );
		CCommLinkRun *pCommLink = AutoCommLink.p();
		if( pCommLink )
		{
			return pCommLink->GetMsgCurCmdObj().GetGSIOTBoardVer( last_ts );
		}
	}

	if( last_ts )
	{
		*last_ts = 0;
	}

	return std::string("");
}

bool DeviceConnection::SetRFMod( defLinkID LinkID, defFreq freq, uint8_t enable_pass, uint8_t check_count, uint8_t enable_timecheck )
{
	if( LinkID>=0 )
	{
		return SetRFMod_EncodeSend( LinkID, freq, enable_pass, check_count, enable_timecheck );
	}
	else if( defLinkID_All != LinkID )
	{
		return false;
	}

	bool hassend = false;
	bool ret = true;

	ret &= SetRFMod_EncodeSend( defLinkID_Local, freq, enable_pass, check_count, enable_timecheck );

	const defmapCommLinkCfg &cfglist = m_CommLinkMgr.Cfg_GetList();
	for( defmapCommLinkCfg::const_iterator it=cfglist.begin(); it!=cfglist.end(); ++it )
	{
		if( !it->second->enable )
			continue;

		hassend = true;
		ret &= SetRFMod_EncodeSend( it->second->id, freq, enable_pass, check_count, enable_timecheck );
	}

	return (ret && hassend);
}

bool DeviceConnection::SetRFMod_EncodeSend( defLinkID LinkID, defFreq freq, uint8_t enable_pass, uint8_t check_count, uint8_t enable_timecheck )
{

	// Э���ܸ�ʽ����
	uint8_t sendBuf[MIAN_PORT_BUF_MAX] = {0};
	uint8_t *sendBuf_end = sendBuf + sizeof(sendBuf);

	sendBuf[0] = 0xBC;
	sendBuf[1] = 0xAC;
	uint8_t *pPacketLen = sendBuf+2;		// ��� λ��
	uint8_t *pPacketVer = pPacketLen+1;		// ��汾 λ��
	uint8_t *pModuleIndex = pPacketVer+1;	// ģ������ λ��
	uint8_t *enc = pModuleIndex+1;			// ��ݶα�����
	uint8_t *pPacketEnd = NULL;				// ����ȷ��λ λ��

	*pPacketVer = 0x65;

	// ��ݶ�
	*enc = enable_pass;
	enc++;

	*enc = check_count;
	enc++;

	*enc = enable_timecheck;
	enc++;


	*pModuleIndex = defFreq_433==freq ? RXB8_433 : RXB8_315; //module

	pPacketEnd = enc;
	enc++;

	*pPacketLen = (enc - sendBuf) - MIAN_PORT_UNPkt_LEN;
	*pPacketEnd = 0xAC;

	m_serial_msg->doMessage( LinkID, sendBuf, pPacketEnd + 1 - sendBuf, IOT_DEVICE_Unknown, 0, NULL, NULL, defNormSendCtlOvertime, defNormMsgOvertime );

	return true;
}

bool DeviceConnection::SendMOD_set( const defMODSysSet cmd, const defLinkID LinkID, const uint32_t QueueOverTime )
{
	if( LinkID>=0 )
	{
		return SendMOD_set_EncodeSend( cmd, LinkID, QueueOverTime );
	}
	else if( defLinkID_All != LinkID )
	{
		return false;
	}

	bool hassend = false;
	bool ret = true;

	ret &= SendMOD_set_EncodeSend( cmd, defLinkID_Local, QueueOverTime );

	const defmapCommLinkCfg &cfglist = m_CommLinkMgr.Cfg_GetList();
	for( defmapCommLinkCfg::const_iterator it=cfglist.begin(); it!=cfglist.end(); ++it )
	{
		if( !it->second->enable )
			continue;

		hassend = true;
		ret &= SendMOD_set_EncodeSend( cmd, it->second->id, QueueOverTime );
	}

	return ( ret && hassend );
}

bool DeviceConnection::SendMOD_set_EncodeSend( const defMODSysSet cmd, const defLinkID LinkID, const uint32_t QueueOverTime )
{
	switch( cmd )
	{
	case defMODSysSet_IR_TXCtl_Query:		// IR �շ����� - ��ѯ
		{
			uint8_t sendBuf[MIAN_PORT_BUF_MAX] = { 0xBC, 0xAC, 0x04, 0x65, MOD_SYS_set, 0x30, 0x0A, 0xAC };
			m_serial_msg->doMessage( LinkID, sendBuf, 8, IOT_DEVICE_Unknown, 0, NULL, NULL, defNormSendCtlOvertime, QueueOverTime );
		}
		break;

	case defMODSysSet_IR_TXCtl_RX:			// IR �շ����� - ��Ϊ��
		{
			uint8_t sendBuf[MIAN_PORT_BUF_MAX] = {0xBC, 0xAC, 0x05, 0x65, MOD_SYS_set, 0x30, 0x1A, 0x00, 0xAC};
			m_serial_msg->doMessage( LinkID, sendBuf, 9, IOT_DEVICE_Unknown, 0, NULL, NULL, defNormSendCtlOvertime, QueueOverTime );
		}
		break;

	case defMODSysSet_IR_TXCtl_TX:			// IR �շ����� - ��Ϊ��
		{
			uint8_t sendBuf[MIAN_PORT_BUF_MAX] = {0xBC, 0xAC, 0x05, 0x65, MOD_SYS_set, 0x30, 0x1A, 0x01, 0xAC};
			m_serial_msg->doMessage( LinkID, sendBuf, 9, IOT_DEVICE_Unknown, 0, NULL, NULL, defNormSendCtlOvertime, QueueOverTime );
		}
		break;

	case defMODSysSet_IR_RXMod_Query:		// IR ��ģʽ��ѯ
		{
			uint8_t sendBuf[MIAN_PORT_BUF_MAX] = {0xBC, 0xAC, 0x04, 0x65, MOD_SYS_set, 0x30, 0x0D, 0xAC};
			m_serial_msg->doMessage( LinkID, sendBuf, 8, IOT_DEVICE_Unknown, 0, NULL, NULL, defNormSendCtlOvertime, QueueOverTime );
		}
		break;

	case defMODSysSet_IR_RXMod_original:	// IR ��ԭʼ���
		{
			//uint8_t sendBuf[MIAN_PORT_BUF_MAX] = {0xBC, 0xAC, 0x0B, 0x65, MOD_SYS_set, 0x30, 0x1D, 0x01, 0x00, 0x00, 0x70, 0x94, 0x00, 0x00, 0xAC};
			uint8_t sendBuf[MIAN_PORT_BUF_MAX] = {0xBC, 0xAC, 0x0B, 0x65, MOD_SYS_set, 0x30, 0x1D, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xAC};
			m_serial_msg->doMessage( LinkID, sendBuf, 15, IOT_DEVICE_Unknown, 0, NULL, NULL, defNormSendCtlOvertime, QueueOverTime );
		}
		break;

	case defMODSysSet_IR_RXMod_code:		// IR �ձ������
		{
			//uint8_t sendBuf[MIAN_PORT_BUF_MAX] = {0xBC, 0xAC, 0x0B, 0x65, MOD_SYS_set, 0x30, 0x1D, 0x00, 0x00, 0x00, 0x70, 0x94, 0x00, 0x00, 0xAC};
			uint8_t sendBuf[MIAN_PORT_BUF_MAX] = {0xBC, 0xAC, 0x0B, 0x65, MOD_SYS_set, 0x30, 0x1D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xAC};
			m_serial_msg->doMessage( LinkID, sendBuf, 15, IOT_DEVICE_Unknown, 0, NULL, NULL, defNormSendCtlOvertime, QueueOverTime );
		}
		break;
		
	case defMODSysSet_RF_RX_freq_Query:		// RF Ƶ�����ò�ѯ
		{
			uint8_t sendBuf[MIAN_PORT_BUF_MAX] = {0xBC, 0xAC, 0x04, 0x65, MOD_SYS_set, 0x10, 0x0D, 0xAC};
			m_serial_msg->doMessage( LinkID, sendBuf, 8, IOT_DEVICE_Unknown, 0, NULL, NULL, defNormSendCtlOvertime, QueueOverTime );
		}
		break;

	case defMODSysSet_RF_RX_freq_315:		// RF Ƶ������315
		{
			uint8_t sendBuf[MIAN_PORT_BUF_MAX] = {0xBC, 0xAC, 0x08, 0x65, MOD_SYS_set, 0x10, 0x1D, 0xC0, 0x84, 0xC6, 0x12, 0xAC};
			m_serial_msg->doMessage( LinkID, sendBuf, 15, IOT_DEVICE_Unknown, 0, NULL, NULL, defNormSendCtlOvertime, QueueOverTime );
		}
		break;

	case defMODSysSet_RF_RX_freq_433:		// RF Ƶ������433
		{
			uint8_t sendBuf[MIAN_PORT_BUF_MAX] = {0xBC, 0xAC, 0x08, 0x65, MOD_SYS_set, 0x10, 0x1D, 0x00, 0x18, 0xDD, 0x19, 0xAC};
			m_serial_msg->doMessage( LinkID, sendBuf, 15, IOT_DEVICE_Unknown, 0, NULL, NULL, defNormSendCtlOvertime, QueueOverTime );
		}
		break;
		
	case defMODSysSet_IR_QueryAll:
		{
			SendMOD_set( defMODSysSet_IR_TXCtl_Query, LinkID, QueueOverTime );
			SendMOD_set( defMODSysSet_IR_RXMod_Query, LinkID, QueueOverTime );
		}
		break;

	case defMODSysSet_QueryAll:
		{
			SendMOD_set( defMODSysSet_RF_RX_freq_Query, LinkID, QueueOverTime );
			SendMOD_set( defMODSysSet_IR_TXCtl_Query, LinkID, QueueOverTime );
			SendMOD_set( defMODSysSet_IR_RXMod_Query, LinkID, QueueOverTime );
		}
		break;

	default:
		return false;
	}

	return true;
}

bool DeviceConnection::Encode_RFRemote_code( const RFSignal &signal, uint8_t *buf, uint16_t &buflen )
{
	uint8_t *enc = buf;

	// ָ��ͷβ����
	*enc = 0;
	if( signal.headlen )
	{
		*enc |= 0x01; // �Ƿ�������

		uint8_t headlen = signal.headlen;
		headlen &= 0x0E;
		headlen <<= 1;
		*enc |= headlen; // ���볤��
	}

	if( signal.taillen )
	{
		*enc |= 0x10; // �Ƿ�������

		uint8_t taillen = signal.taillen;
		taillen &= 0xE0;
		taillen <<= 5;
		*enc |= taillen; // ���볤��
	}
	enc++;

	// ָ���
	*enc = signal.codeValidLen; enc++;

	// �ظ����ʹ���
	uint8_t signal_count = signal.signal_count<RUNCODE_Get(defCodeIndex_RF_Send_CountMin) ? RUNCODE_Get(defCodeIndex_RF_Send_CountDefault):signal.signal_count;
	*enc = signal_count; enc++;

	// ����
	memcpy( enc, signal.headcode, sizeof(uint16_t)*signal.headlen );
	enc += sizeof(uint16_t)*signal.headlen;

	// ��ݶ�BIT 1������ʱ��
	memcpy( enc, &signal.one_high_time, sizeof(uint16_t) );
	enc += sizeof(uint16_t);

	// ��ݶ�BIT 1�½���ʱ��
	memcpy( enc, &signal.one_low_time, sizeof(uint16_t) );
	enc += sizeof(uint16_t);

	// ��ݶ�BIT 0������ʱ��
	memcpy( enc, &signal.zero_high_time, sizeof(uint16_t) );
	enc += sizeof(uint16_t);

	// ��ݶ�BIT 0�½���ʱ��
	memcpy( enc, &signal.zero_low_time, sizeof(uint16_t) );
	enc += sizeof(uint16_t);

	// �����ʱ��code
	memcpy( enc, &signal.code, 4 );
	enc += 4;

	// silent���ʱ��
	memcpy( enc, &signal.silent_interval, sizeof(uint16_t) );
	enc += sizeof(uint16_t);

	// ������
	memcpy( enc, signal.tailcode, sizeof(uint16_t)*signal.taillen );
	enc += sizeof(uint16_t)*signal.taillen;

	buflen = enc-buf;
	return true;
}

bool DeviceConnection::Encode_RFRemote_original( const RFSignal &signal, uint8_t *buf, uint16_t &buflen )
{
	
	//if( signal.freq <= 0 )
	//	return false;

	uint8_t *enc = buf;

	uint8_t signal_count = signal.signal_count<RUNCODE_Get(defCodeIndex_RF_Send_CountMin) ? RUNCODE_Get(defCodeIndex_RF_Send_CountDefault):signal.signal_count;

	*enc = signal_count; enc++;

	const int originalbytelen = sizeof(uint16_t)*signal.original_len <= sizeof(signal.original) ? sizeof(uint16_t)*signal.original_len : sizeof(signal.original);

	// ѧϰ������ݻ��ϣ�����Ϊ������ʱ����ǰ�油������ʱ��
	if( 0 != signal.original_headflag )
	{
		memcpy( enc, &signal.original_headtime, sizeof(uint16_t) );
		enc += sizeof(uint16_t);
	}

	memcpy( enc, &signal.original, originalbytelen );
	enc += originalbytelen;

	// ѧϰ������ݻ��ϣ�����Ϊ0x00������ʱ������貹������ʱ��
	if( 0 == signal.original_headflag )
	{
		memcpy( enc, &signal.original_headtime, sizeof(uint16_t) );
		enc += sizeof(uint16_t);
	}

	buflen = enc-buf;
	return true;
}

bool DeviceConnection::Encode_RFRemote_IR_code( const RFSignal &signal, uint8_t *buf, uint16_t &buflen )
{
	
	uint8_t *enc = buf;

	uint8_t signal_count = signal.signal_count<RUNCODE_Get(defCodeIndex_RF_Send_CountMin) ? RUNCODE_Get(defCodeIndex_RF_Send_CountDefault):signal.signal_count;

	*enc = signal_count; enc++;

	const int originalbytelen = signal.original_len <= sizeof(signal.original) ? signal.original_len : sizeof(signal.original); // original_len=�ֽ�
	
	*enc = originalbytelen; enc++;

	memcpy( enc, &signal.original, originalbytelen );
	enc += originalbytelen;

	buflen = enc-buf;
	return true;
}

bool DeviceConnection::Encode_RFRemote( const RFSignal &signal, uint8_t *buf, uint16_t &buflen )
{
	if( !signal.isValid() )
	{
		return false;
	}

	switch( signal.signal_type )
	{
	case defRFSignalType_code:
		return Encode_RFRemote_code( signal, buf, buflen );

	case defRFSignalType_original:
	case defRFSignalType_IR_original:
		return Encode_RFRemote_original( signal, buf, buflen );

	case defRFSignalType_IR_code:
		return Encode_RFRemote_IR_code( signal, buf, buflen );
	}

	return false;
}

defGSReturn DeviceConnection::SendControl( const IOTDeviceType DevType, const GSIOTDevice *device, const GSIOTObjBase *obj, const uint32_t overtime, const uint32_t QueueOverTime, const uint32_t nextInterval )
{
	if( obj )
	{
		if( GSIOTObjType_DeviceAddress != obj->GetObjType() )
		{
			assert(false);
			return defGSReturn_ErrParam;
		}
	}

	if( !device )
		return defGSReturn_ErrParam;

	if( !device->getControl() )
		return defGSReturn_ErrParam;

	const int DevID = device->getId();
	ControlBase *ctl = device->getControl();
	DeviceAddress *address =  (DeviceAddress*)obj;

	defGSReturn isSuccess = defGSReturn_Err;

	if( !GetRunning() )
	{
		return isSuccess;
	}

	const defLinkID LinkID = ctl->GetLinkID();
	if( LinkID<0 )
	{
		//LOGMSGEX( defLOGNAME, defLOG_WORN, "SendControl:LinkID=%d error!!!\r\n", LinkID );
		return isSuccess;
	}

	// Э���ܸ�ʽ����
	uint8_t sendBuf[MIAN_PORT_BUF_MAX] = {0};
	uint8_t *sendBuf_end = sendBuf + sizeof(sendBuf);

	sendBuf[0] = 0xBC;
	sendBuf[1] = 0xAC;
	uint8_t *pPacketLen = sendBuf+2;		// ��� λ��
	uint8_t *pPacketVer = pPacketLen+1;		// ��汾 λ��
	uint8_t *pModuleIndex = pPacketVer+1;	// ģ������ λ��
	uint8_t *enc = pModuleIndex+1;			// ��ݶα�����
	uint8_t *pPacketEnd = NULL;				// ����ȷ��λ λ��

	*pPacketVer = 0x65;

	switch(ctl->GetType()){
		case IOT_DEVICE_RFDevice:
			{
				RFDeviceControl *rfCtl = (RFDeviceControl *)ctl;
				if(!rfCtl)return defGSReturn_Success;
				RFDevice *dev = rfCtl->GetDevice();

				*pModuleIndex = CC1101_433; //module
				enc = CommandToByte(enc,rfCtl->GetCommand());//cmd
				enc = Int32ToByte(enc,dev->GetProductid());
				enc = Int32ToByte(enc,dev->GetPasscode());
				
				if(address){
				    uint32_t addr = address->GetAddress();
					switch(rfCtl->GetCommand()){
					case Device_Address_Request:
						{
							*enc ++= dev->GetAddressType();
							switch(dev->GetAddressType()){
								case IOT_Byte:
									*enc ++= addr;
									break;
								case IOT_Int16:
									enc = Int16ToByte(enc,addr);
									break;
								case IOT_Integer:
									enc = Int32ToByte(enc,addr);
									break;
							}
							break;
						}
					}
				}

				pPacketEnd = enc;
				enc++;

				*pPacketLen = (enc - sendBuf) - MIAN_PORT_UNPkt_LEN;
				*pPacketEnd = 0xAC;
				m_serial_msg->doMessage( LinkID, sendBuf, pPacketEnd + 1 - sendBuf, DevType, DevID, NULL, NULL, overtime, QueueOverTime, nextInterval );
				isSuccess = defGSReturn_Success;
			    break;
			}
		case IOT_DEVICE_CANDevice:
			{
				*pModuleIndex = CAN_Chip; //module
				
				CANDeviceControl *cctl = (CANDeviceControl *)ctl;
				if(!cctl)return defGSReturn_Success;
				enc = Int32ToByte(enc, cctl->GetDeviceid());
				*enc ++= cctl->GetCommand();
				if(address){
					*enc ++= address->GetAddress();

					switch(cctl->GetCommand())
					{
					case CAN_Address_Write_Request:
						{
							if(address->GetDataType() != IOT_Object){
							    enc = address->GetCurToByte(enc);
							}else{
							    enc = address->GetObjectData(enc);
							}
							break;
						}
					case CAN_Address_Setting_Request:
						{
							enc = address->GetObjectData(enc);
							break;
						}
					}
				}

				pPacketEnd = enc;
				enc++;

				*pPacketLen = (enc - sendBuf) - MIAN_PORT_UNPkt_LEN;
				*pPacketEnd = 0xAC;
				m_serial_msg->doMessage( LinkID, sendBuf, pPacketEnd + 1 - sendBuf, DevType, DevID, NULL, NULL, overtime, QueueOverTime, nextInterval );
				isSuccess = defGSReturn_Success;
			    break;
			}
			
		case IOT_DEVICE_RS485:
			{
				*pModuleIndex = Module_RS485; //module

				// data 
				//enc
				RS485DevControl *rsCtl = (RS485DevControl *)ctl;

				defRS485MsgQue que;
				if( rsCtl->Encode( que, address ) )
				{
					uint8_t *ptemp_enc = enc;

					defRS485MsgQue::const_iterator it = que.begin();
					defRS485MsgQue::const_iterator itEnd = que.end();
					for( ; it!=itEnd; ++it )
					{
						struRS485Msg *pMsg = (*it);
						
						memcpy( enc, pMsg->buf, pMsg->buflen );
						enc += pMsg->buflen;

						pPacketEnd = enc;
						enc++;

						*pPacketLen = (enc - sendBuf) - MIAN_PORT_UNPkt_LEN;
						*pPacketEnd = 0xAC;
						m_serial_msg->doMessage( LinkID, sendBuf, pPacketEnd + 1 - sendBuf, DevType, DevID, rsCtl, address, overtime, QueueOverTime, nextInterval );
						isSuccess = defGSReturn_Success;

						enc = ptemp_enc;
					}
				}

				g_DeleteRS485MsgQue( que );
				break;
			}

		case IOT_DEVICE_Remote:
			{
				RFRemoteControl *rfCtl = (RFRemoteControl*)ctl;
				const defButtonQueue &que = rfCtl->GetButtonList();

				uint8_t *ptemp_enc = enc;

				defButtonQueue::const_iterator it = que.begin();
				defButtonQueue::const_iterator itEnd = que.end();
				for( ; it!=itEnd; ++it )
				{
					RemoteButton *pCurButton = *it;

					uint16_t buflen = sizeof(sendBuf)-(enc -sendBuf);
					if( Encode_RFRemote( pCurButton->GetSignal(), enc, buflen ) )
					{
						enc += buflen;

						pPacketEnd = enc;
						enc++;

						*pPacketLen = (enc - sendBuf) - MIAN_PORT_UNPkt_LEN;
						*pPacketEnd = 0xAC;
						*pModuleIndex = pCurButton->GetSignal().GetModuleIndex(); //module

						// ���ʹ�����ڱ�����
						LOGMSG( "RFRemoteCtrl(%s) Send module=%d, %s", pCurButton->GetObjName().c_str(), \
						       pCurButton->GetSignal().GetModuleIndex(), pCurButton->GetSignal().Print( "", false ).c_str() );

						m_serial_msg->doMessage( LinkID, sendBuf, pPacketEnd + 1 - sendBuf, \
						       DevType, DevID, rfCtl, address, overtime, QueueOverTime, \
						       nextInterval>1 ? nextInterval : pCurButton->GetSignal().GetSendNeedTime() );

						isSuccess = defGSReturn_Success;
					}
					else
					{
						//LOGMSGEX( defLOGNAME, defLOG_ERROR, "RFRemoteCtrl(%s) Send err, %s", pCurButton->GetObjName().c_str(), pCurButton->GetSignal().Print( "", false ).c_str() );
					}

					enc = ptemp_enc;
				}
			}
			break;
	}

	return isSuccess;
}

// ������С
#define defSendDataMaxSize 255

// �ְ����ݴ�С
#define defSendDataClipSize	(255-6)  // ����ȥͷβ

// ���ڷ��Ͷ���ӿ�
int DeviceConnection::SendData(CCommLinkRun *CommLink, unsigned char *sendbuf,int sendsize)
{
	// ���̫�����зְ���
	if( sendsize <= defSendDataMaxSize )
	{
		return SendData_clip( CommLink, sendbuf, sendsize );
	}

	g_PrintfByte( sendbuf, sendsize, "send (dev SendData) largesize", CommLink?CommLink->get_cfg().id:defPrintfByteDefaultLinkID, CommLink?CommLink->get_cfg().name.c_str():"" );

	// ÿ�ηְ�洢
	unsigned char clipbuf[1024] = {0};
	int clipsize = 0;

	memcpy( clipbuf, sendbuf, 5 ); // ͷ����
	uint8_t *const clipbuf_PacketLen = clipbuf+2;	// ��� λ��
	uint8_t *const clipbuf_data = clipbuf+5;		// data λ��

	unsigned char *buf_temp = sendbuf+5; // ��ǰ��ݶ�̬��ַ
	int buf_temp_size = sendsize-6; // ��ǰ��ݶ�̬ʣ���С
	const unsigned char *const buf_temp_end = sendbuf+sendsize-1; // ��ݽ�β
	//int buf_temp_size = buf_temp_end-buf_temp; // ��ǰ��ݶ�̬ʣ���С

	// �ְ�
	int sendsizecount = 0;
	for( ; buf_temp<buf_temp_end; )
	{
		if( !this->isRunning )
		{
			break;
		}

		int cur_copy_size = buf_temp_size>defSendDataClipSize ? defSendDataClipSize:buf_temp_size;

		memcpy( clipbuf_data, buf_temp, cur_copy_size );
		*clipbuf_PacketLen = cur_copy_size+6-MIAN_PORT_UNPkt_LEN;// ��С
		clipbuf_data[cur_copy_size] = 0xAC; // ��β

		sendsizecount += this->SendData_clip( CommLink, clipbuf, cur_copy_size+6 );

		buf_temp += cur_copy_size;
		buf_temp_size -= cur_copy_size;
		
		usleep(100000);
	}

	return sendsizecount;
}

// ���ڷ��ͷְ�
int DeviceConnection::SendData_clip(CCommLinkRun *CommLink, unsigned char *buf,int size)
{
	if( !this->isRunning )
	{
		return 0;
	}

	defTransMod trans_mod = defTransMod_GSIOT;
	if( CommLink )
	{
		trans_mod = CommLink->get_cfg().trans_mod;
	}

#if defined(defTEST_ModbusMod)
	if( IsRUNCODEEnable(defCodeIndex_TEST_ModbusMod) )
	{
		// modbus����ģʽ��ȥ��ͷβ
		trans_mod = defTransMod_Transparent;

		if( IsRUNCODEEnable(defCodeIndex_PrintSend_MainPort) )
		{
			g_PrintfByte( buf, size, "__TEST__: <TEST_ModbusMod>Dev SendData", CommLink?CommLink->get_cfg().id:defPrintfByteDefaultLinkID, CommLink?CommLink->get_cfg().name.c_str():"" );
		}
	}
#endif

	if( !IsRUNCODEEnable(defCodeIndex_Dis_CommLinkTransparent) )
	{
		if( defTransMod_Transparent == trans_mod )
		{
			buf += 5;
			size -= 6;
		}
	}

	if( IsRUNCODEEnable(defCodeIndex_PrintSend_MainPort) )
	{
		g_PrintfByte( buf, size, "send (dev SendData)", CommLink?CommLink->get_cfg().id:defPrintfByteDefaultLinkID, CommLink?CommLink->get_cfg().name.c_str():"" );
	}

	int sendsize = 0;
	if( CommLink )
	{
		sendsize = CommLink->Write( buf, size );
		if( sendsize<0 )
		{
			LOGMSG( "DeviceConnection::SendData_clip err, CommLink%d\r\n", CommLink->get_cfg().id );
			CommLink->Close();
			sendsize = 0;
		}
	}
	else
	{
		sendsize = RS232_SendBuf(this->m_port,buf,size);
	}

	if( IsRUNCODEEnable(defCodeIndex_PrintSend_MainPort) )
	{
		//LOGMSGEX( defLOGNAME, sendsize!=size?defLOG_ERROR:defLOG_INFO, "Link%d(%s) %s(Send ret=%d)", CommLink?CommLink->get_cfg().id:defPrintfByteDefaultLinkID, CommLink?CommLink->get_cfg().name.c_str():"", sendsize!=size?"(send err!!!)":"", sendsize );
	}

	return sendsize;
}

int DeviceConnection::RecvData(CCommLinkRun *CommLink, unsigned char *buf,int size)
{
	int recvsize = 0;
	if( CommLink )
	{
		recvsize = CommLink->Read( buf, size );
		if( recvsize<0 )
		{
			LOGMSG( "DeviceConnection::RecvData err, CommLink%d\r\n", CommLink->get_cfg().id );
			CommLink->Close();
			recvsize = 0;
		}
	}
	else
	{
		//recvsize = RS232_PollComport( this->GetPort(), buf, size ); //jyc20160901
	}
	if( IsRUNCODEEnable(defCodeIndex_PrintRecv_MainPort) )
	{
		g_PrintfByte( buf, recvsize, "recv (dev RecvData)", CommLink?CommLink->get_cfg().id:defPrintfByteDefaultLinkID, CommLink?CommLink->get_cfg().name.c_str():"" );
	}

	return recvsize;
}

void DeviceConnection::Run(int port)
{	
#if defined(defTEST_ModbusMod)
	if( IsRUNCODEEnable(defCodeIndex_TEST_ModbusMod) )
	{
		//LOGMSGEX( defLOGNAME, defLOG_WORN, ">>>>> DeviceConnection::Run <TEST_ModbusMod> <<<<<\r\n\r\n" );
	}
#endif

	//load device
	this->LoadDevice();
	//jyc20160824
	//m_GSM.setSerialPortHandler( this );

	isRunning = true;

	// CommLinkThread
	if( m_CommLinkMgr.Cfg_GetList().empty() )
	{
		AddCommLinkThread( 99 );
	}
	else
	{
		const uint32_t CommLinkThreadMax = RUNCODE_Get(defCodeIndex_CommLinkThreadMax);
		const int CommLinkThreadNum = m_CommLinkMgr.Cfg_GetList().size();
		for( int i=1; i<=CommLinkThreadNum && i<=CommLinkThreadMax; ++i )
		{
			AddCommLinkThread( CommLinkThreadNum ); 
		}
	}

	/* jyc20160906 delete
	pthread_t id_2;  
	int i,ret;  
	//this->isRunning = true; //jyc20160826 add just test

	ret=pthread_create(&id_2,NULL,SerialPort_Thread,this);  
	if(ret!=0)  
	{  
   		printf("Create timemanager_pthread error!\n");  
		//return -1;  
		return; 
	}//*/

	if(port>=1)
	{
		m_usecfg_port = port;
		port--; //���������ֵ������ҪС
		//if(isRunning && port!=m_port){
		//	this->Stoped();
		//}
		//if(!isRunning)
		{
			//isRunning = true;
			const int baudrate = RUNCODE_Get(defCodeIndex_SYS_COM_baudrate);

			if(RS232_OpenComport(port,baudrate)){
				//isRunning = false;
				//LOGMSGEX( defLOGNAME, defLOG_WORN, "OpenCom Faild!!! comindex=%d, baudrate=%d\r\n", port, baudrate );
				return;
			}

			//LOGMSGEX( defLOGNAME, defLOG_SYS, "OpenCom success. comindex=%d, baudrate=%d\r\n", port, baudrate );
			m_usecfg_baudrate = baudrate;

			m_port = port;

			RS232_disableDTR( m_port );
			RS232_disableRTS( m_port );

			/*jyc20160824
			//����COM�����߳�		
			HANDLE   hth1;
			unsigned  uiThread1ID;
			m_threadRunning++;
			hth1 = (HANDLE)_beginthreadex(NULL, 0, SerialPortThread, this, 0, &uiThread1ID);
			CloseHandle(hth1);
			*/	
		}
	}
}

void DeviceConnection::AddCommLinkThread( int CommLinkCount )
{
	const int CommLinkThread_id = this->Get_CommLinkThread_id();

	if( CommLinkThread_id >= CommLinkCount )
	{
		return;
	}

	const uint32_t CommLinkThreadMax = RUNCODE_Get(defCodeIndex_CommLinkThreadMax);

	if( CommLinkThread_id >= CommLinkThreadMax )
	{
		return;
	}
	/*jyc20160824
	//�����߳�
	HANDLE   hth1;
	unsigned  uiThread1ID;
	m_threadRunning++;
	hth1 = (HANDLE)_beginthreadex(NULL, 0, CommLinkThread, this, 0, &uiThread1ID);
	CloseHandle(hth1);
	*/

	pthread_t id_1;  
    int i,ret;  
	//this->isRunning = true; //jyc20160826 add just test
	//printf("creat pthread print...\n");

    ret=pthread_create(&id_1,NULL,CommLink_Thread,this);  
    if(ret!=0)  
    {  
        printf("Create timemanager_pthread error!\n");  
		//return -1;  
		return; 
    } 
}

void DeviceConnection::Stoped()
{
	this->isRunning = false;
}

void DeviceConnection::OnDataReceived( CHeartbeatGuard *phbGuard, CCommLinkRun *CommLink, uint8_t* const srcbuf, const uint32_t srcbufsize )
{
	defTransMod trans_mod = defTransMod_GSIOT;
	if( CommLink )
	{
		trans_mod = CommLink->get_cfg().trans_mod;
	}

	unsigned char *buf = srcbuf;
	uint32_t size = srcbufsize;

#if defined(defTEST_ModbusMod)
if( IsRUNCODEEnable(defCodeIndex_TEST_ModbusMod) )
	{
		// modbus����ģʽ�¼���ͷβ
		trans_mod = defTransMod_Transparent;

		if( IsRUNCODEEnable(defCodeIndex_PrintRecv_MainPort) )
		{
			g_PrintfByte( buf, size, "__TEST__: <TEST_ModbusMod> recv (dev OnDataReceived)", CommLink?CommLink->get_cfg().id:defPrintfByteDefaultLinkID, CommLink?CommLink->get_cfg().name.c_str():"" );
		}
	}
#endif

	if( !IsRUNCODEEnable(defCodeIndex_Dis_CommLinkTransparent) )
	{
		if( defTransMod_Transparent == trans_mod )
		{
			*(buf+size) = 0xAC;
			*(--buf) = Module_RS485;
			*(--buf) = 0x65;
			*(--buf) = size+2;
			*(--buf) = 0xAC;
			*(--buf) = 0xBC;
			size += 6;
		}
	}

	unsigned char *buffer = buf;

	while(size>5)
	{
		uint8_t PacketHead = *buffer++; size--;			// ��ͷ
		uint8_t PacketHeadCheck = *buffer++; size--;	// ��ͷ ȷ��λ
		uint8_t PacketLen = *buffer++; size--;			// ��� λ��
		uint8_t PacketVer = *buffer++; size--;			// ��汾 λ��
		uint8_t ModuleIndex = *buffer++; size--;		// ģ������ λ��

		// ��ͷ
		if( 0xBC != PacketHead
			|| 0xAC != PacketHeadCheck
			)
		{
			continue;
		}

		if( PacketLen>MIAN_PORT_Pkt_LEN
			|| 0x65 != PacketVer
			)
		{
			g_PrintfByte( srcbuf, srcbufsize, "Device DataReceived head error! src=", CommLink?CommLink->get_cfg().id:defPrintfByteDefaultLinkID, CommLink?CommLink->get_cfg().name.c_str():"" );
			continue;
		}

		uint8_t DataLen = PacketLen-2;			// ��ݶγ���
		if( size<(uint32_t)(DataLen+1) ) // ��ݶκͽ���ȷ��λ �ĳ���
		{
			g_PrintfByte( srcbuf, srcbufsize, "Device DataReceived len no enough! src=", CommLink?CommLink->get_cfg().id:defPrintfByteDefaultLinkID, CommLink?CommLink->get_cfg().name.c_str():"" );
			continue;
		}

		uint8_t PacketEnd = *(buffer+DataLen);	// ����ȷ��λ λ��
		if( 0xAC != PacketEnd )
		{
			g_PrintfByte( srcbuf, srcbufsize, "Device DataReceived end error! src=", CommLink?CommLink->get_cfg().id:defPrintfByteDefaultLinkID, CommLink?CommLink->get_cfg().name.c_str():"" );
			continue;
		}

		//������
		switch(ModuleIndex)
		{
			case RXB8_315:
				{
					this->DecodeRxb8Data(phbGuard, CommLink, defFreq_315, buffer, DataLen);
					break;
				}
			case RXB8_433:
				{
					this->DecodeRxb8Data(phbGuard, CommLink, defFreq_433, buffer, DataLen);
					break;
				}
			case CC1101_433:
				{
					//������
					this->DecodeCC1101Data(CommLink, buffer, DataLen);
				    break;
				}
			case CAN_Chip:
				{	   
					this->DecodeCANData(CommLink, buffer, DataLen);
					break;
				}
			case RXB8_315_TX:
				//ң�ط���
				break;
			case RXB8_433_TX:
				//ң�ط���
				break;
			case Module_RS485:
				{
					this->Decode_RS485Data( phbGuard, CommLink, buffer, DataLen );
					break;
				}

			case RXB8_315_original:
				{
					this->Decode_Rxb8Data_original(phbGuard, CommLink, defFreq_315, buffer, DataLen);
					break;
				}

			case RXB8_433_original:
				{
					this->Decode_Rxb8Data_original(phbGuard, CommLink, defFreq_433, buffer, DataLen);
					break;
				}

			case Module_GSM:
				{
					LOGMSG( "DataReceived GSM: %s", buffer );

					if( IsRUNCODEEnable(defCodeIndex_Disable_Recv_GSM) )
					{
						//LOGMSGEX( defLOGNAME, defLOG_WORN, "__TEST__: Disable_Recv_GSM!" );
						return;
					}

					break;
				}

			case Read_IOT_Ver:
				{
					this->Decode_Read_IOT_Ver( phbGuard, CommLink, buffer, DataLen );
					break;
				}
				
			case MOD_IR_RX_original:
				{
					this->Decode_MOD_IR_RX_original( phbGuard, CommLink, buffer, DataLen );
					break;
				}

			case MOD_IR_RX_code:
				{
					this->Decode_MOD_IR_RX_code( phbGuard, CommLink, buffer, DataLen );
					break;
				}

			case MOD_SYS_get:
				{
					this->Decode_MOD_SYS_get( phbGuard, CommLink, buffer, DataLen );
					break;
				}

			default:
				{
					g_PrintfByte( srcbuf, srcbufsize, "Device DataReceived ModuleIndex error! src=", CommLink?CommLink->get_cfg().id:defPrintfByteDefaultLinkID, CommLink?CommLink->get_cfg().name.c_str():"" );
					break;
				}
		}

		buffer+= DataLen;
		size-= DataLen;
		PacketEnd = *buffer++; size--;			// ����ȷ��λ λ��
	}
}

void DeviceConnection::LoadDevice()
{
	if(devManager){
		std::list<GSIOTDevice *> deviceList = devManager->GetDeviceList();
		if(deviceList.size()>0){
			std::list<GSIOTDevice *>::iterator it = deviceList.begin();
			for(;it!=deviceList.end();it++){
				this->m_handler->OnDeviceConnect(*it);
			}
		}
	}
}

bool DeviceConnection::DeleteDevice( GSIOTDevice *iotdevice )
{
	if(devManager)
	{
		if( devManager->DeleteDevice( iotdevice ) )
		{
			m_handler->OnDeviceDisconnect(iotdevice);
			delete iotdevice;
			return true;
		}
	}

	return false;
}

bool DeviceConnection::ModifyDevice( GSIOTDevice *iotdevice, uint32_t ModifySubFlag )
{
	if(devManager)
	{
		if( devManager->DB_ModifyDevice( iotdevice, ModifySubFlag ) )
		{
			if( m_handler )
			{
				m_handler->OnDeviceNotify( defDeviceNotify_Modify, iotdevice, NULL );
			}

			return true;
		}
	}

	return false;
}

bool DeviceConnection::ModifyAddress( GSIOTDevice *iotdevice, DeviceAddress *addr )
{
	if(devManager)
	{
		if( devManager->DB_ModifyAddress( iotdevice, addr ) )
		{
			if( m_handler )
			{
				m_handler->OnDeviceNotify( defDeviceNotify_Modify, iotdevice, addr );
			}

			return true;
		}
	}

	return false;
}

int DeviceConnection::AddController(ControlBase *ctl, const std::string &ver, uint32_t enable, GSIOTDevice **outNewDev)
{
	int newid = 0;
	if(devManager)
	{
		newid = devManager->AddController(ctl, ver);
	}
	GSIOTDevice *dev = devManager->GetIOTDevice( ctl->GetType(), newid );

	if( !dev )
		return 0;

	if( outNewDev )
	{
		*outNewDev = dev;
	}

	m_handler->OnDeviceConnect(dev);

	return newid;
}

ControlBase *DeviceConnection::GetController( IOTDeviceType deviceType, uint32_t deviceId ) const
{
	std::list<GSIOTDevice *> deviceList = devManager->GetDeviceList();
	if(deviceList.size()>0)
	{
		std::list<GSIOTDevice *>::iterator it = deviceList.begin();
		for(;it!=deviceList.end();++it)
		{
			if( (*it)->getType() == deviceType
				&& (*it)->getId() == deviceId )
			{
				return (*it)->getControl();
			}
		}
    }
	return NULL;
}

GSIOTDevice* DeviceConnection::GetIOTDevice( IOTDeviceType deviceType, uint32_t deviceId ) const
{
	return devManager->GetIOTDevice( deviceType, deviceId );
}

void DeviceConnection::DeviceSaveToFile()
{
	 devManager->SaveToFile();
}

void DeviceConnection::AddRFHandler(IRFReadHandler *handler)
{
	if(handler)
	   m_RFHandlerQueue.push_back(handler);
}

void DeviceConnection::RemoveRFHandler(IRFReadHandler *handler)
{
	if(handler)
		m_RFHandlerQueue.remove(handler);
}

void DeviceConnection::AddRFDeviceHandler(IRFDeviceHandler *handler)
{	
	if(handler)
	   m_RFDeviceHandlerQueue.push_back(handler);
}

void DeviceConnection::RemoveRFDeviceHandler(IRFDeviceHandler *handler)
{	
	if(handler)
		m_RFDeviceHandlerQueue.remove(handler);
}

void DeviceConnection::AddCANDeviceHandler(ICANDeviceHandler *handler)
{
	if(handler)
	   m_CANDeviceHandlerQueue.push_back(handler);
}

void DeviceConnection::RemoveCANDeviceHandler(ICANDeviceHandler *handler)
{
	if(handler)
		m_CANDeviceHandlerQueue.remove(handler);
}

void DeviceConnection::GetRFDeviceInfo(uint32_t productID,uint32_t passCode)
{

}

#ifndef OS_UBUNTU_FLAG
uint8_t *Reversebuffer16(uint8_t *buffer, unsigned int len){
	uint8_t *src;
	unsigned int i;
	for(i=0;i<len;i+=2){
		src[i]=buffer[i+1];
		src[i+1]=buffer[i];
	}
	return src;
}

void Reversememcpy16(uint16_t *src, uint8_t *buffer, unsigned int len){
	unsigned int i;
	for(i=0;i<len;i+=2){
		src[i/2]=buffer[i+1]<<8|buffer[i];
	}
}

void Reversememcpy32(uint32_t *src, uint8_t *buffer, unsigned int len){
	unsigned int i;
	for(i=0;i<len;i+=4){
		src[i/4]=buffer[i+3]<<24|buffer[i+2]<<16|buffer[i+1]<<8|buffer[i];
	}
}
#endif

void DeviceConnection::DecodeRxb8Data( CHeartbeatGuard *phbGuard, CCommLinkRun *CommLink, defFreq freq, uint8_t* const srcbuf, const uint32_t srcbufsize )
{
	if( IsRUNCODEEnable(defCodeIndex_PrintRecv_RF) )
	{
		g_PrintfByte( srcbuf, srcbufsize, "recv DecodeRxb8Data RF" );
	}

	if( IsRUNCODEEnable(defCodeIndex_Disable_Recv_RF) )
	{
		if( IsRUNCODEEnable(defCodeIndex_PrintRecv_RF) )
		{
			//LOGMSGEX( defLOGNAME, defLOG_WORN, "__TEST__: Disable_Recv_RF! recv DecodeRxb8Data" );
		}

		return;
	}

	unsigned char *buf = srcbuf;
	uint32_t size = srcbufsize;

	// �����ֽ���
	if( srcbufsize < 3 )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "DeviceConnection::DecodeRxb8Data size=%d err!\r\n", srcbufsize );
		return;
	}

	RFSignal signal;
	memset( &signal, 0, sizeof(signal) );

	signal.signal_type = defRFSignalType_code;
	signal.freq = freq;

	// ���ָ��ͷβ���壬ȡ����Ӧλ��ֵ
	// ���볤��
	if( buf[0] & 0x01 ) // �Ƿ�������
	{
		signal.headlen = (buf[0] & 0x0E)>>1;
	}

	// �����볤��
	if( buf[0] & 0x10 ) // �Ƿ��н�����
	{
		signal.taillen = (buf[0] & 0xE0)>>5;
	}

	// �ֽ���
	const uint32_t needsize = (1+1+sizeof(uint16_t)*signal.headlen+2*4+4+2+sizeof(uint16_t)*signal.taillen);
	if( size < needsize )
	{
		//LOGMSGEX( defLOGNAME, defLOG_INFO, "DeviceConnection::DecodeRxb8Data size=%d err! needsize=%d, headlen=%d, taillen=%d\r\n", srcbufsize, needsize, signal.headlen, signal.taillen );
		return;
	}

	uint8_t *buffer = buf;

	buffer++; // ָ��ͷβ����

	// ָ���					(1 byte)		ָ����Чλ����
	signal.codeValidLen = *buffer++;

	if( signal.headlen>0 )
	{
		memcpy( &signal.headcode, buffer, sizeof(uint16_t)*signal.headlen );
		buffer += sizeof(uint16_t)*signal.headlen;
	}

	memcpy( &signal.one_high_time, buffer, sizeof(uint16_t) );
	buffer += sizeof(uint16_t);

	memcpy( &signal.one_low_time, buffer, sizeof(uint16_t) );
	buffer += sizeof(uint16_t);

	memcpy( &signal.zero_high_time, buffer, sizeof(uint16_t) );
	buffer += sizeof(uint16_t);

	memcpy( &signal.zero_low_time, buffer, sizeof(uint16_t) );
	buffer += sizeof(uint16_t);


	uint32_t codemask = ~0;
	if( signal.codeValidLen<32 )
	{
#ifndef OS_OPENWRT //jyc20170505 modify OS_UBUNTU_FLAG  //jyc20170301 add for notice message
		codemask >>= 32-signal.codeValidLen;
#else
		codemask <<= 32-signal.codeValidLen;
#endif
	}

	memcpy( &signal.code, buffer, 4 );
	buffer += 4;
	signal.code &= codemask;

	memcpy( &signal.silent_interval, buffer, sizeof(uint16_t) );
	buffer += sizeof(uint16_t);

	if( signal.taillen>0 )
	{
		memcpy( &signal.tailcode, buffer, sizeof(uint16_t)*signal.taillen );
		buffer += sizeof(uint16_t)*signal.taillen;
	}

	if( IsRUNCODEEnable(defCodeIndex_PrintRecv_val_RF) )
	{
		//jyc20160824 debug all off
		//signal.Print( "recv DecodeRxb8Data" );
	}
	OnDeviceData_RFSignal( phbGuard, CommLink, signal );
}

void DeviceConnection::OnDeviceData_RFSignal( CHeartbeatGuard *phbGuard, CCommLinkRun *CommLink, const RFSignal &signal )
{
	macHeartbeatGuard_step(151100);

	const defLinkID LinkID = CommLink ? CommLink->get_cfg().id : defLinkID_Local;

	bool isAdd = false;
	std::string strAddedName;
	std::list<GSIOTDevice *> deviceList = devManager->GetDeviceList();
	if( !deviceList.empty() )
	{
		std::list<GSIOTDevice *>::const_iterator it = deviceList.begin();
		for(;it!=deviceList.end();it++)
		{
			if((*it)->getType() == IOT_DEVICE_Trigger)
			{
				TriggerControl *tc = (TriggerControl *)(*it)->getControl();
				if(tc)
				{
					if( tc->GetSignal().IsNear( signal ) )
					{
						if( (*it)->GetEnable() )
						{
							this->m_handler->OnDeviceData( LinkID, *it, tc, NULL );
						}

						isAdd = true;
						strAddedName = (*it)->getName();
					}
				}
			}
		}
	}

	macHeartbeatGuard_step(151800);
	std::list<IRFReadHandler *>::const_iterator it = m_RFHandlerQueue.begin();
	for(;it!=m_RFHandlerQueue.end();it++){
		(*it)->OnRead(LinkID, signal, isAdd, strAddedName);
	}

	macHeartbeatGuard_step(151900);
}

void DeviceConnection::DecodeCC1101Data(CCommLinkRun *CommLink, uint8_t *buf,uint32_t size)
{
	return;

	const defLinkID LinkID = CommLink ? CommLink->get_cfg().id : defLinkID_Local;

	uint8_t *buffer = buf;
	uint16_t cmd = ByteToCommand(buffer);
	buffer+=2;
	uint32_t pid = ByteToInt32(buffer);
	buffer+=4;

	if(pid==0){
		return;
	}

	switch(cmd){
	case Heartbeat_Response:
		{
			break;
		}
	case Device_Address_Response:
		{
			std::list<GSIOTDevice *> deviceList = devManager->GetDeviceList();
			if(deviceList.size()>0){
				std::list<GSIOTDevice *>::const_iterator it = deviceList.begin();
				for(;it!=deviceList.end();it++){
					if((*it)->getType()==IOT_DEVICE_RFDevice){
						RFDeviceControl *rfControl = (RFDeviceControl *)(*it)->getControl();
						if(!rfControl) continue;
						RFDevice *dev = rfControl->GetDevice();
						if(dev->GetProductid() == pid){
							//���µ�ַ��Ϣ
							DeviceAddress *addr = dev->DecodeAddressData(buffer,size - (buffer - buf));
							if(addr){
								std::list<IRFDeviceHandler *>::const_iterator ith = m_RFDeviceHandlerQueue.begin();
								for(;ith!=m_RFDeviceHandlerQueue.end();ith++){
									(*ith)->OnDeviceAddress(addr);
								}
								//�豸����						
								this->m_handler->OnDeviceData(LinkID, *it,rfControl,addr);
							}
							break;
						}
					}
				}
			}
			break;
		}
	case Device_Broadcast:
		{
			std::list<IRFDeviceHandler *>::const_iterator it = m_RFDeviceHandlerQueue.begin();
			for(;it!=m_RFDeviceHandlerQueue.end();it++){
				(*it)->OnDeviceBroadcast(pid);
			}
			break;
		}
	case Device_Info_Response:
		{
			RFDevice *dev = new RFDevice();
			dev->SetProductid(pid);
			if(dev->DecodeData(buffer,size - (buffer - buf))){
				RFDeviceControl *ctl = new RFDeviceControl(dev);
				devManager->AddTempController(ctl);
				std::list<IRFDeviceHandler *>::const_iterator it = m_RFDeviceHandlerQueue.begin();
				for(;it!=m_RFDeviceHandlerQueue.end();it++){
					(*it)->OnDeviceInfo(ctl);
				}
				return;
			}
			//û��handler�ͷŶ���
			delete(dev);
			break;
		}
	case Device_Error:
		{
			std::list<IRFDeviceHandler *>::const_iterator it = m_RFDeviceHandlerQueue.begin();
			for(;it!=m_RFDeviceHandlerQueue.end();it++){
				(*it)->OnDevicePassError();
			}
			break;
		}
	}
}

void DeviceConnection::DecodeCANData(CCommLinkRun *CommLink, uint8_t *buf,uint32_t size)
{
	return;

	const defLinkID LinkID = CommLink ? CommLink->get_cfg().id : defLinkID_Local;

	uint8_t *buffer = buf;
	uint32_t deviceId = ByteToInt32(buffer);
	buffer+=4;
	uint8_t cmd = *buffer++;

	if(deviceId==0){
		return;
	}

	switch(cmd)
	{
		case CAN_Heartbeat_Response:
			break;
		case CAN_Info_Response:
			{
				uint32_t pid = ByteToInt32(buffer);
				buffer+=4;
				uint8_t addressCount = *buffer++;

				std::list<GSIOTDevice *> deviceList = devManager->GetDeviceList();
				if(deviceList.size()>0){
					std::list<GSIOTDevice *>::const_iterator it = deviceList.begin();
					for(;it!=deviceList.end();it++){
						if((*it)->getType()==IOT_DEVICE_CANDevice){
							CANDeviceControl *ctl = (CANDeviceControl *)(*it)->getControl();
							if(!ctl) continue;
							if(ctl->GetDeviceid() == deviceId &&
								ctl->GetProductid() == pid){
								return;
							}
						}
					}
				}
								       								   							
				if(m_CANDeviceHandlerQueue.size()>0){
					CANDeviceControl *ctl = new CANDeviceControl(deviceId,pid,addressCount);
					devManager->AddTempController(ctl);

					std::list<ICANDeviceHandler *>::const_iterator it = m_CANDeviceHandlerQueue.begin();
					for(;it!=m_CANDeviceHandlerQueue.end();it++){
						(*it)->OnDeviceInfo(ctl);
					}
				}
				break;
			}
		case CAN_Address_Response:
			{
				std::list<GSIOTDevice *> deviceList = devManager->GetDeviceList();
				if(deviceList.size()>0){
					std::list<GSIOTDevice *>::const_iterator it = deviceList.begin();
					for(;it!=deviceList.end();it++){
						if((*it)->getType()==IOT_DEVICE_CANDevice){
							CANDeviceControl *ctl = (CANDeviceControl *)(*it)->getControl();
							if(!ctl) continue;
							if(ctl->GetDeviceid() == deviceId){
								if(ctl->DecodeAddressData(buffer,size - (buffer - buf))){
									std::list<ICANDeviceHandler *>::const_iterator ith = m_CANDeviceHandlerQueue.begin();
									for(;ith!=m_CANDeviceHandlerQueue.end();ith++){
										(*ith)->OnAddressInfo(ctl);
									}
								}
								return;
							}
						}
					}
				}
				break;
			}
		case CAN_Address_Read_Response:
			{
				std::list<GSIOTDevice *> deviceList = devManager->GetDeviceList();
				if(deviceList.size()>0){
					std::list<GSIOTDevice *>::const_iterator it = deviceList.begin();
					for(;it!=deviceList.end();it++){
						if((*it)->getType()==IOT_DEVICE_CANDevice){
							CANDeviceControl *ctl = (CANDeviceControl *)(*it)->getControl();
							if(!ctl) continue;
							if(ctl->GetDeviceid() == deviceId){
								DeviceAddress *addr =ctl->DecodeAddressInfoData(buffer,size - (buffer - buf));
								//�豸����
								if(addr){
									std::list<ICANDeviceHandler *>::const_iterator ith = m_CANDeviceHandlerQueue.begin();
									for(;ith!=m_CANDeviceHandlerQueue.end();ith++){
										(*ith)->OnDeviceAddress(addr);
									}
									this->m_handler->OnDeviceData(LinkID, *it,ctl,addr);
								}
								return;
							}
						}
					}
				}
				break;
			}
		case CAN_Address_Write_Response:
			{
				break;
			}
		case CAN_Address_Setting_Response:
			{
				std::list<GSIOTDevice *> deviceList = devManager->GetDeviceList();
				if(deviceList.size()>0){
					std::list<GSIOTDevice *>::const_iterator it = deviceList.begin();
					for(;it!=deviceList.end();it++){
						if((*it)->getType()==IOT_DEVICE_CANDevice){
							CANDeviceControl *ctl = (CANDeviceControl *)(*it)->getControl();
							if(!ctl) continue;
							if(ctl->GetDeviceid() == deviceId){
								DeviceAddress *addr =ctl->DecodeAddressSettingData(buffer,size - (buffer - buf));
								//�豸����
								if(addr){
									std::list<ICANDeviceHandler *>::const_iterator ith = m_CANDeviceHandlerQueue.begin();
									for(;ith!=m_CANDeviceHandlerQueue.end();ith++){
										(*ith)->OnDeviceAddress(addr);
									}
									this->m_handler->OnDeviceData(LinkID, *it,ctl,addr);
								}
								return;
							}
						}
					}
				}
				break;
			}
		case CAN_Address_Packet_Ext:
			{
				std::list<GSIOTDevice *> deviceList = devManager->GetDeviceList();
				if(deviceList.size()>0){
					std::list<GSIOTDevice *>::const_iterator it = deviceList.begin();
					for(;it!=deviceList.end();it++){
						if((*it)->getType()==IOT_DEVICE_CANDevice){
							CANDeviceControl *ctl = (CANDeviceControl *)(*it)->getControl();
							if(!ctl) continue;
							if(ctl->GetDeviceid() == deviceId){
								ctl->DoPacketExtData(buffer,size - (buffer - buf));
								return;
							}
						}
					}
				}
				break;
			}
		case CAN_Address_Error:
			break;
	}
}

// ����RS485ģ����չ��������
void DeviceConnection::Decode_RS485Data( CHeartbeatGuard *phbGuard, CCommLinkRun *CommLink, uint8_t* const srcbuf, const uint32_t srcbufsize )
{
	macHeartbeatGuard_step(157100);

	const defLinkID LinkID = CommLink ? CommLink->get_cfg().id : defLinkID_Local;

	if( IsRUNCODEEnable(defCodeIndex_PrintRecv_RS485) )
	{
		g_PrintfByte( srcbuf, srcbufsize, "recv Decode_RS485Data " );
	}

	if( IsRUNCODEEnable(defCodeIndex_Disable_Recv_RS485) )
	{
		if( IsRUNCODEEnable(defCodeIndex_PrintRecv_RS485) )
		{
			//LOGMSGEX( defLOGNAME, defLOG_WORN, "__TEST__: Disable_Recv_RS485! recv Decode_RS485Data" );
		}

		return;
	}

	RS485DevControl *pDecodeCtrl = NULL;
	if( !RS485DevControl::Decode( srcbuf, srcbufsize, &pDecodeCtrl ) )
	{
		if( pDecodeCtrl )
		{
			delete pDecodeCtrl;
		}

		return ;
	}

	if( !pDecodeCtrl )
	{
		return;
	}

	pDecodeCtrl->SetLinkID( LinkID );
	std::auto_ptr<RS485DevControl> autop(pDecodeCtrl);

	if( IsRUNCODEEnable(defCodeIndex_PrintRecv_val_RS485) )
	{
		pDecodeCtrl->Print( "recv DeSrc" );
	}

	macHeartbeatGuard_step(157200);

	SERIALDATABUFFER *CurCmd = NULL;
	CMsgCurCmd *pMsgCurCmd = CommLink ? &CommLink->GetMsgCurCmdObj() : &m_serial_msg->GetMsgCurCmdObj();
	pMsgCurCmd->Get_CurCmd( pDecodeCtrl, &CurCmd );

	macHeartbeatGuard_step(157300);

	if( !CurCmd )
	{
		if( this->IsCurGSDevViewMod_Debug() )
		{
			this->m_handler->OnDeviceData( LinkID, NULL, pDecodeCtrl, NULL ); // ֻ֪ͨ��ַ��ֵ֪ͨ�����ڱ�����
		}

		return;
	}

	RS485DevControl *CurCmd_Ctrl = (RS485DevControl*)CurCmd->ctl;
	DeviceAddress *CurCmd_Addr = CurCmd->address;

	if( !CurCmd_Ctrl )
	{
		CMsgCurCmd::Delete_CurCmd_Content_spec( CurCmd );

		if( this->IsCurGSDevViewMod_Debug() )
		{
			this->m_handler->OnDeviceData( LinkID, NULL, pDecodeCtrl, NULL ); // ֻ֪ͨ��ַ��ֵ֪ͨ�����ڱ�����
		}

		return;
	}

	macHeartbeatGuard_step(157400);

	if( CurCmd_Ctrl->GetDeviceid() != pDecodeCtrl->GetDeviceid() )
	{
		// ��������е�ַ��������ȴû�У����ô���
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "DeviceConnection::Decode_RS485Data %s devid error! send devid=%d, recv devid=%d\r\n", CurCmd_Ctrl->GetName().c_str(), CurCmd_Ctrl->GetDeviceid(), pDecodeCtrl->GetDeviceid() );
		CMsgCurCmd::Delete_CurCmd_Content_spec( CurCmd );

		if( this->IsCurGSDevViewMod_Debug() )
		{
			this->m_handler->OnDeviceData( LinkID, NULL, pDecodeCtrl, NULL ); // ֻ֪ͨ��ַ��ֵ֪ͨ�����ڱ�����
		}

		return;
	}

	macHeartbeatGuard_step(157500);

	CurCmd_Ctrl->SetCommand( pDecodeCtrl->GetCommand() );
	CurCmd_Ctrl->SetReturnErrCode( pDecodeCtrl->GetReturnErrCode() );

	// ĿǰĬ�ϵ�����ַ����
	DeviceAddress *pDecodeCtrlAddr = pDecodeCtrl->GetFristAddress();
	if( pDecodeCtrlAddr
		&& CurCmd_Ctrl->GetCommand()<defModbusCmd_Err_Spec
		&& CurCmd_Ctrl->GetReturnErrCode()==0 )
	{
		if( pDecodeCtrlAddr->GetAddress() > 0 )
		{
			if( CurCmd_Addr && CurCmd_Addr->GetAddress() != pDecodeCtrlAddr->GetAddress() )
			{
				// ��������е�ַ��������ȴû�У����ô���
				//LOGMSGEX( defLOGNAME, defLOG_ERROR, "DeviceConnection::Decode_RS485Data %s addr error! send addr=%d, recv addr=%d\r\n", CurCmd_Ctrl->GetName().c_str(), CurCmd_Addr->GetAddress(), pDecodeCtrlAddr->GetAddress() );
				CMsgCurCmd::Delete_CurCmd_Content_spec( CurCmd );

				if( this->IsCurGSDevViewMod_Debug() )
				{
					this->m_handler->OnDeviceData( LinkID, NULL, pDecodeCtrl, NULL ); // ֻ֪ͨ��ַ��ֵ֪ͨ�����ڱ�����
				}

				return;
			}
		}

		macHeartbeatGuard_step(157600);

		if( CurCmd_Addr && CurCmd_Addr->GetAddress()>0 )
		{
			pDecodeCtrl->EncodeAddressNum( CurCmd_Addr->GetAddress() );
		}
		else
		{
			DeviceAddress *pOneAddr_CurCmdFirst = CurCmd_Ctrl->GetFristAddress();

			if( pOneAddr_CurCmdFirst )
			{
				pDecodeCtrl->EncodeAddressNum( pOneAddr_CurCmdFirst->GetAddress() );
			}
		}

		const defAddressQueue &AddrQue = pDecodeCtrl->GetAddressList();
		defAddressQueue::const_iterator itAddrQue = AddrQue.begin();
		for( ; itAddrQue!=AddrQue.end(); ++itAddrQue )
		{
			DeviceAddress *pOneAddr_Decode = *itAddrQue;

			DeviceAddress *pOneAddr_CurCmd = CurCmd_Ctrl->GetAddress( pOneAddr_Decode->GetAddress() );

			if( pOneAddr_CurCmd )
			{
				switch( pOneAddr_CurCmd->GetType() )
				{
				case IOT_DEVICE_CO2:
				case IOT_DEVICE_HCHO:
				//case IOT_DEVICE_PM25:						
				case IOT_DEVICE_Temperature:
				case IOT_DEVICE_Humidity:
				case IOT_DEVICE_Wind:
					{
						// �⼸����ݶ���100������
						const std::string strCurValue = pOneAddr_Decode->GetCurValue();
						float flVal = (float)atoi( strCurValue.c_str() );

						//LOGMSG( "Decode_RS485Data: src devid=%d, addr=%d, val=%.2f, ver=%s\r\n", CurCmd_Ctrl?CurCmd_Ctrl->GetDeviceid():0, pOneAddr_Decode->GetAddress(), flVal, CurCmd_Ctrl->getVer().c_str() );
						
						if(CurCmd_Ctrl->getVer() == defRS485_Ver_CO2_201706A){	 //jyc20170305 remove
							if(pOneAddr_Decode->GetAddress()>2)flVal /= 10.0f; //jyc20170422 notice remove have bug
						}else
						if(CurCmd_Ctrl->getVer() == defRS485_Ver_HCHO_201706A){
							if(pOneAddr_Decode->GetAddress()<3)flVal /= 1000.0f; //jyc20170422 add
							else flVal /= 10.0f; //jyc20170422 notice some trouble
						}else if( CurCmd_Ctrl->getVer() == defRS485_Ver_Wind_1601_201501A
							|| CurCmd_Ctrl->getVer() == defRS485_Ver_Humidity_1601_201506A ){
							flVal /= 10.0f;
						}else{
							flVal /= 100.0f;
						}

						bool isOverRange = false;
						if( RS485DevControl::IsValueOverRange( pOneAddr_Decode->GetType(), flVal ) )
						{
							isOverRange = true;
							//LOGMSGEX( defLOGNAME, defLOG_ERROR, "ValueOverRange: devid=%d, addr=%d, val=%.2f!!!\r\n", CurCmd_Ctrl?CurCmd_Ctrl->GetDeviceid():0, pOneAddr_Decode->GetAddress(), flVal );
						}

						flVal = RS485DevControl::ValueRangeFix( pOneAddr_Decode->GetType(), flVal );

						pOneAddr_CurCmd->SetDataType( IOT_Float );
						if((CurCmd_Ctrl->getVer() == defRS485_Ver_HCHO_201706A)&&
							pOneAddr_CurCmd-> GetType() == IOT_DEVICE_HCHO)
						{ //jyc20170619 modify
							pOneAddr_CurCmd->SetCurValue_3f( flVal );
						}else{
							pOneAddr_CurCmd->SetCurValue( flVal );
						}
						
						if( isOverRange)
						{
							macHeartbeatGuard_step(157780);
							
							pOneAddr_CurCmd->SetCurValue( "0" );
							CMsgCurCmd::Delete_CurCmd_Content_spec( CurCmd );

							if( this->IsCurGSDevViewMod_Debug() )
							{
								this->m_handler->OnDeviceData( LinkID, NULL, pDecodeCtrl, NULL ); // ֻ֪ͨ��ַ��ֵ֪ͨ�����ڱ�����
							}

							macHeartbeatGuard_step(157790);
							return;
						}

						pOneAddr_Decode->SetCurValue( pOneAddr_CurCmd->GetCurValue() );
					}
					break;

				default:
					{
						pOneAddr_CurCmd->SetCurValue( pOneAddr_Decode->GetCurValue() );
					}
					break;
				}

				if( CurCmd_Addr && pOneAddr_CurCmd->GetAddress() == CurCmd_Addr->GetAddress() )
				{
					CurCmd_Addr->SetCurValue( pOneAddr_CurCmd->GetCurValue() );
				}
			}
		}
	}

	if( IsRUNCODEEnable(defCodeIndex_PrintRecv_val_RS485) )
	{
		pDecodeCtrl->Print( "recv DeCalc" );
	}

	macHeartbeatGuard_step(157800);
	
	this->m_handler->OnDeviceData( LinkID, NULL, CurCmd_Ctrl, CurCmd_Addr );
	CMsgCurCmd::Delete_CurCmd_Content_spec( CurCmd );

	macHeartbeatGuard_step(157900);
}


void DeviceConnection::Decode_Rxb8Data_original( CHeartbeatGuard *phbGuard, CCommLinkRun *CommLink, const defFreq freq, uint8_t* const srcbuf, const uint32_t srcbufsize )
{
	if( IsRUNCODEEnable(defCodeIndex_PrintRecv_RF_original) )
	{
		g_PrintfByte( srcbuf, srcbufsize, "recv Decode_Rxb8Data_original RF" );
	}

	if( IsRUNCODEEnable(defCodeIndex_Disable_Recv_RF_original) )
	{
		if( IsRUNCODEEnable(defCodeIndex_PrintRecv_RF_original) )
		{
			//LOGMSGEX( defLOGNAME, defLOG_WORN, "__TEST__: Disable_Recv_RF_original! recv Decode_Rxb8Data_original" );
		}

		return;
	}

	unsigned char *buf = srcbuf;
	uint32_t size = srcbufsize;

	// �����ֽ���
	if( srcbufsize < 2 )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "DeviceConnection::Decode_Rxb8Data_original size=%d err!\r\n", srcbufsize );
		return;
	}

	RFSignal signal;
	memset( &signal, 0, sizeof(signal) );

	signal.signal_type = defRFSignalType_original;
	signal.freq = freq;

	// ���������������	(1 byte)	0x00�����壬0x01��
	signal.original_headflag = *buf;
	buf++; size--;

	// ����ʱ��	UIN16_T
	memcpy( &signal.original_headtime, buf, sizeof(uint16_t) );
	buf += sizeof(uint16_t); size -= sizeof(uint16_t);

	// ʱ�����	(1 byte)
	const uint8_t timenum = *buf;
	buf++; size--;
	
	signal.original_len = size>sizeof(signal.original) ? sizeof(signal.original)/sizeof(uint16_t) : size/sizeof(uint16_t);

	if( !timenum == signal.original_len )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "Decode_Rxb8Data_original recv err, recv timenum=%d, analyse original_len=%d\r\n", timenum, signal.original_len );
	}

	memcpy( &signal.original, buf, sizeof(uint16_t)*signal.original_len );

	if( IsRUNCODEEnable(defCodeIndex_PrintRecv_val_RF) )
	{
		//signal.Print( "recv Decode_Rxb8Data_original" );
	}

	OnDeviceData_RFSignal( phbGuard, CommLink, signal );
}

void DeviceConnection::Decode_MOD_IR_RX_original( CHeartbeatGuard *phbGuard, CCommLinkRun *CommLink, uint8_t* const srcbuf, const uint32_t srcbufsize )
{
	if( IsRUNCODEEnable(defCodeIndex_PrintRecv_IR_original) )
	{
		g_PrintfByte( srcbuf, srcbufsize, "recv Decode_MOD_IR_RX_original IR" );
	}

	if( IsRUNCODEEnable(defCodeIndex_Disable_Recv_IR_original) )
	{
		if( IsRUNCODEEnable(defCodeIndex_PrintRecv_IR_original) )
		{
			//LOGMSGEX( defLOGNAME, defLOG_WORN, "__TEST__: Disable_Recv_IR_original! recv Decode_MOD_IR_RX_original" );
		}

		return;
	}

	unsigned char *buf = srcbuf;
	uint32_t size = srcbufsize;

	// �����ֽ���
	if( srcbufsize < 2 )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "DeviceConnection::Decode_MOD_IR_RX_original size=%d err!\r\n", srcbufsize );
		return;
	}

	RFSignal signal;
	memset( &signal, 0, sizeof(signal) );

	signal.signal_type = defRFSignalType_IR_original;
	signal.freq = defFreq_Null;

	// ���������������	(1 byte)	0x00�����壬0x01��
	signal.original_headflag = *buf;
	buf++; size--;

	// ����ʱ��	UIN16_T
	memcpy( &signal.original_headtime, buf, sizeof(uint16_t) );
	buf += sizeof(uint16_t); size -= sizeof(uint16_t);

	// ʱ�����	(1 byte)
	const uint8_t timenum = *buf;
	buf++; size--;
	
	signal.original_len = size>sizeof(signal.original) ? sizeof(signal.original)/sizeof(uint16_t) : size/sizeof(uint16_t);

	if( !timenum == signal.original_len )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "Decode_MOD_IR_RX_original recv err, recv timenum=%d, analyse original_len=%d\r\n", timenum, signal.original_len );
	}

	memcpy( &signal.original, buf, sizeof(uint16_t)*signal.original_len );

	if( IsRUNCODEEnable(defCodeIndex_PrintRecv_val_IR) )
	{
		//signal.Print( "recv Decode_MOD_IR_RX_original" );
	}

	OnDeviceData_RFSignal( phbGuard, CommLink, signal );
}

void DeviceConnection::Decode_MOD_IR_RX_code( CHeartbeatGuard *phbGuard, CCommLinkRun *CommLink, uint8_t* const srcbuf, const uint32_t srcbufsize )
{
	if( IsRUNCODEEnable(defCodeIndex_PrintRecv_IR_code) )
	{
		g_PrintfByte( srcbuf, srcbufsize, "recv Decode_MOD_IR_RX_code IR" );
	}

	if( IsRUNCODEEnable(defCodeIndex_Disable_Recv_IR_code) )
	{
		if( IsRUNCODEEnable(defCodeIndex_PrintRecv_IR_code) )
		{
			//LOGMSGEX( defLOGNAME, defLOG_WORN, "__TEST__: Disable_Recv_IR_code! recv Decode_MOD_IR_RX_code" );
		}

		return;
	}

	unsigned char *buf = srcbuf;
	uint32_t size = srcbufsize;

	// �����ֽ���
	if( srcbufsize < 2 )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "DeviceConnection::Decode_MOD_IR_RX_code size=%d err!\r\n", srcbufsize );
		return;
	}

	RFSignal signal;
	memset( &signal, 0, sizeof(signal) );

	signal.signal_type = defRFSignalType_IR_code;
	signal.freq = defFreq_Null;

	// ��ݳ���
	signal.original_len = *buf;
	buf++; size--;

	// �������
	memcpy( &signal.original, buf, signal.original_len );

	if( IsRUNCODEEnable(defCodeIndex_PrintRecv_val_IR) )
	{
		//jyc20160824
		//signal.Print( "recv Decode_MOD_IR_RX_code" );
	}

	OnDeviceData_RFSignal( phbGuard, CommLink, signal );
}

void DeviceConnection::Decode_Read_IOT_Ver( CHeartbeatGuard *phbGuard, CCommLinkRun *CommLink, uint8_t* const srcbuf, const uint32_t srcbufsize )
{
	macHeartbeatGuard_step(150910);

	unsigned char *buf = srcbuf;
	uint32_t size = srcbufsize;


	char chver[256] = {0};
	memcpy( chver, srcbuf, srcbufsize );
	chver[srcbufsize] = 0;

	//LOGMSG( "Decode_Read_IOT_Ver=%s\r\n", chver );

	CMsgCurCmd *pMsgCurCmd = CommLink ? &CommLink->GetMsgCurCmdObj() : &m_serial_msg->GetMsgCurCmdObj();
	pMsgCurCmd->SetGSIOTBoardVer( chver );

	macHeartbeatGuard_step(150980);

	if( CommLink ) this->OnCommLinkNotify( ICommLinkNotifyHandler::CLNType_Refresh, CommLink->get_cfg().id );

	macHeartbeatGuard_step(150990);
}

void DeviceConnection::Decode_MOD_SYS_get( CHeartbeatGuard *phbGuard, CCommLinkRun *CommLink, uint8_t* const srcbuf, const uint32_t srcbufsize )
{
	macHeartbeatGuard_step(150910);

	unsigned char *buf = srcbuf;
	uint32_t size = srcbufsize;

	uint8_t getmod = *buf;
	buf++; size--;

	CMsgCurCmd *pMsgCurCmd = CommLink ? &CommLink->GetMsgCurCmdObj() : &m_serial_msg->GetMsgCurCmdObj();

	switch( getmod )
	{
	case 0x10:
		{
			uint8_t getcmd = *buf;
			buf++; size--;

			switch( getcmd )
			{
				case 0x0D:
				case 0x1D:
				{
					uint32_t freq = 0;
					memcpy( &freq, buf, 4 );

					if( 315000000==freq )
						pMsgCurCmd->Set_MODSysSetInfo( defMODSysSetInfo_RF_RX_freq, defFreq_315 );
					else if( 433920000==freq )
						pMsgCurCmd->Set_MODSysSetInfo( defMODSysSetInfo_RF_RX_freq, defFreq_433 );
				}
				break;

			default:
				return;
			}
		}
		break;

	case 0x30:
		{
			uint8_t getcmd = *buf;
			buf++; size--;

			switch( getcmd )
			{
			case 0x0D:
			case 0x1D:
				pMsgCurCmd->Set_MODSysSetInfo( defMODSysSetInfo_RXMod, *buf );	// ��ݸ�ʽ ԭʼ�����
				break;

			case 0x0A:
			case 0x1A:
				pMsgCurCmd->Set_MODSysSetInfo( defMODSysSetInfo_TXCtl, *buf );	// ����ģʽ ����(ѧϰģʽ)����
				break;

			default:
				return;
			}
		}
		break;

	default:
		return;
	}

	macHeartbeatGuard_step(150980);

	if( CommLink ) this->OnCommLinkNotify( ICommLinkNotifyHandler::CLNType_Refresh, CommLink->get_cfg().id );

	macHeartbeatGuard_step(150990);
}


// �޸��豸��ַ
bool DeviceConnection::ModifyDevID( GSIOTDevice *iotdevice, uint8_t newID, const std::string &ver, uint8_t SpecAddr )
{
	if( !iotdevice )
		return false;

	ControlBase *ctlbase = iotdevice->getControl();
	if( !ctlbase )
	{
		return false;
	}

	bool isSuccess = false;
	switch(ctlbase->GetType())
	{
	case IOT_DEVICE_RS485:
		{
			DeviceAddress *pAddr = ((RS485DevControl*)ctlbase)->GetFristAddress();
			
			if( !pAddr )
			{
				return false;
			}

			isSuccess = ModifyDevID_RS485( iotdevice->GetLinkID(), pAddr->GetType(), ((RS485DevControl*)ctlbase)->GetDeviceid(), newID, ver, SpecAddr );
			if( isSuccess )
			{
				((RS485DevControl*)ctlbase)->SetDeviceid( newID );
			}

		}
	}

	if( isSuccess )
	{
		isSuccess = this->ModifyDevice( iotdevice );
	}

	return isSuccess;
}

// �޸�RS485ģ�������豸���豸��ַ
bool DeviceConnection::ModifyDevID_RS485( const defLinkID in_LinkID, IOTDeviceType Type, uint8_t oldID, uint8_t newID, const std::string &ver, uint8_t SpecAddr )
{
	RS485DevControl *ModifyDevIDCtl = RS485DevControl::CreateRS485DevControl_ModifyDevAddr( Type, oldID, newID, ver );
	
	if( !ModifyDevIDCtl )
		return false;
	
	GSIOTDevice *device = new GSIOTDevice( 0, c_NullStr, IOT_DEVICE_RS485, c_NullStr, c_NullStr, c_NullStr, ModifyDevIDCtl );
	std::auto_ptr<GSIOTDevice> autop(device);

	ModifyDevIDCtl->SetLinkID( in_LinkID );

	if( SpecAddr>0 && ModifyDevIDCtl->GetFristAddress() )
	{
		ModifyDevIDCtl->GetFristAddress()->SetAddress( SpecAddr );
	}

	return this->SendControl( IOT_DEVICE_RS485, device, ModifyDevIDCtl->GetFristAddress() )>0;
}

void DeviceConnection::OnThreadExit()
{
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "%sDeviceConnection::OnThreadExit(%d)", this->isRunning?">>>>>err ":"", m_threadRunning );
	m_threadRunning--;
	if( m_threadRunning < 0 )
	{
		m_threadRunning = 0;
	}
}

bool DeviceConnection::OnTest_Trigger( const int devid )
{
	std::list<GSIOTDevice*> deviceList = devManager->GetDeviceList();
	if( !deviceList.empty() )
	{
		std::list<GSIOTDevice*>::const_iterator it = deviceList.begin();
		for(;it!=deviceList.end();it++)
		{
			if( devid==(*it)->getId() )
			{
				if( (*it)->getType() == IOT_DEVICE_Trigger )
				{
					TriggerControl *tc = (TriggerControl*)(*it)->getControl();
					if(tc)
					{
						//jyc20160824
						OnDeviceData_RFSignal( NULL, NULL, tc->GetSignal() );
						return true;
					}
				}

				return false;
			}
		}
	}

	return false;
}

