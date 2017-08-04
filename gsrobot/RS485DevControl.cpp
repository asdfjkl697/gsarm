#include "RS485DevControl.h"
#include "common.h"
#include "gloox/util.h"
//#include <Windows.h>


#define defValueRange_Temperature_Min	(-40)
#define defValueRange_Temperature_Max	(100)

#define defValueRange_Humidity_Min		(0)
#define defValueRange_Humidity_Max		(100)

#define defValueRange_Wind_Min			(0)
#define defValueRange_Wind_Max			(999)

#define defValueRange_CO2_Min			(0)
#define defValueRange_CO2_Max			(9999)

#define defValueRange_HCHO_Min			(0)
#define defValueRange_HCHO_Max			(999)

#define defValueRange_PM25_Min			(0)
#define defValueRange_PM25_Max			(9999)


void g_DeleteRS485MsgQue( defRS485MsgQue &que )
{
	while( !que.empty() )
	{
		struRS485Msg *addr = que.front();
		delete(addr);
		que.pop_front();
	}
}

RS485DevControl::RS485DevControl( uint32_t deviceID, uint32_t command, uint32_t protocol, const std::string &ver )
	:m_deviceID(deviceID), m_command(command), m_protocol(protocol), m_ver(ver), m_returnErrCode(0), m_NetUseable(defUseable_OK)
{
	InitNewMutex();

	m_failedCount = 0;
	m_lastSuccessTS = timeGetTime();
}

RS485DevControl::RS485DevControl( const Tag* tag)
	:ControlBase(tag), m_NetUseable(defUseable_OK)
{
	InitNewMutex();
	m_failedCount = 0;
	m_lastSuccessTS = timeGetTime();

	Reset();
	if( !tag || tag->name() != defDeviceTypeTag_rs485device)
		return;

	if(tag->hasAttribute("device_id"))
		this->m_deviceID = atoi(tag->findAttribute("device_id").c_str());
	if(tag->hasAttribute("command"))
		this->m_command = atoi(tag->findAttribute("command").c_str());
	if(tag->hasAttribute("protocol"))
		this->m_protocol = atoi(tag->findAttribute("protocol").c_str());
	if(tag->hasAttribute("ver"))
		this->m_ver = tag->findAttribute("ver");

	const TagList& l = tag->children();
	TagList::const_iterator it = l.begin();
	TagList::const_iterator itEnd = l.end();
	for( ; it != itEnd; ++it )
	{
		const std::string& name = (*it)->name();
		if( name == "address" )
		{
			this->m_AddressQueue.push_back(new DeviceAddress(*it));
		}
	}

	this->UntagEditAttr( tag );
}

RS485DevControl::~RS485DevControl(void)
{
	if( m_pmutex_RS485DevCtl )
	{
		delete m_pmutex_RS485DevCtl;
		m_pmutex_RS485DevCtl = NULL;
	}
}

void RS485DevControl::InitNewMutex( bool CreateLock )
{
	if( !CreateLock )
	{
		m_pmutex_RS485DevCtl = NULL;
		return;
	}

	// new dev use
	m_pmutex_RS485DevCtl = new gloox::util::Mutex();
}

bool RS485DevControl::IsReadCmd( uint32_t cmd )
{
	switch(cmd)
	{
	case defModbusCmd_Read:
	case defModbusCmd_Read04:
		{
			return true;
		}
	}

	return false;
}

bool RS485DevControl::IsWriteCmd( uint32_t cmd )
{
	switch(cmd)
	{
	case defModbusCmd_Write:
		{
			return true;
		}
	}

	return false;
}

bool RS485DevControl::IsReadCmd_Ret( uint32_t cmd )
{
	switch(cmd)
	{
	case defModbusCmd_Err_Read:
	case defModbusCmd_Err_Read04:
		{
			return true;
		}
	}

	return false;
}

bool RS485DevControl::IsUseRead04( IOTDeviceType type, const std::string ver )
{
	switch(type)
	{
	case IOT_DEVICE_Wind:
		{
			if( ver == defRS485_Ver_Wind_04_201312A )
			{
				return true;
			}
		}
		break;
	}

	return false;
}

bool RS485DevControl::doEditAttrFromAttrMgr( const EditAttrMgr &attrMgr, defCfgOprt_ oprt )
{
	if( attrMgr.GetEditAttrMap().empty() )
		return false;

	bool doUpdate = false;
	std::string outAttrValue;

	if( attrMgr.FindEditAttr( "device_id", outAttrValue ) )
	{
		doUpdate = true;
		this->SetDeviceid( atoi(outAttrValue.c_str()) );
	}

	return doUpdate;
}

void RS485DevControl::Reset()
{
	m_deviceID = 0;
	m_command = 0;
	m_protocol = defProtocol_Unknown;
	m_ver = c_DefaultVer;
	m_returnErrCode = 0;
}

RS485DevControl* RS485DevControl::CreateRS485DevControl_ModifyDevAddr( IOTDeviceType DeviceType, uint32_t oldDevID, uint32_t newDevID, const std::string &ver )
{
	uint32_t address = 0;

	switch( DeviceType )
	{
	case IOT_DEVICE_Switch:
		address = 1+1;
		break;

	case IOT_DEVICE_CO2:
		address = 115;
		break;
	case IOT_DEVICE_HCHO:
		address = 119;
		break;
	case IOT_DEVICE_PM25:
		address = 0x10+1;
		break;
			
	case IOT_DEVICE_Temperature:
		address = 0x10+1;
		break;

	case IOT_DEVICE_Humidity:
		{
			if( ver == defRS485_Ver_Humidity_1601_201506A )
			{
				address = 2;
			}
			else
			{
				address = 0x10+1;
			}
		}
		break;

	case IOT_DEVICE_Wind:
		{
			if( ver == defRS485_Ver_Wind_1601_201501A )
			{
				address = 2;
			}
			else
			{
				address = 10;
			}
		}
		break;

	default:
		{
			return NULL;
		}
		break;
	}

	if( address <= 0 )
		return NULL;

	char chID[32] = {0};
	snprintf( chID, sizeof(chID), "%d", newDevID );

	RS485DevControl *prs485 = new RS485DevControl( oldDevID, defModbusCmd_Write, defProtocol_Unknown, ver );
	//jyc20170306 notice 
	//prs485->AddSpecAddr( DeviceAddress( address, "", DeviceType, IOT_Byte, IOT_WRITE, chID ) ); 

	return prs485;
}

bool RS485DevControl::AddSpecAddr( DeviceAddress &Addrobj, uint16_t addressStart, uint16_t addressEnd, uint16_t SNoStart )
{
	if( addressEnd<addressStart || addressStart<=0 || addressEnd<=0 )
	{
		addressStart = 0;
		addressEnd = 0;
	}

	for( uint16_t i=addressStart; i<=addressEnd; ++i )
	{
		DeviceAddress *paddr = (DeviceAddress*)Addrobj.clone();

		if( i>0 )
		{
			paddr->SetAddress( i );
		}

		if( SNoStart>0 )
		{
			char chname[256] = {0};
			snprintf( chname, sizeof(chname), "%s-%d", Addrobj.GetName().c_str(), SNoStart++ );
			paddr->SetName( std::string(chname) );
		}

		this->AddAddress( paddr );
	}

	return true;
}

bool RS485DevControl::IsValueOverRange( IOTDeviceType DeviceType, float val )
{
	switch( DeviceType )
	{
	case IOT_DEVICE_CO2:
		{
			if( macIsValueOverRange( val, defValueRange_CO2_Min, defValueRange_CO2_Max ) )
			{
				return true;
			}
		}
		break;
	case IOT_DEVICE_HCHO:
		{
			if( macIsValueOverRange( val, defValueRange_HCHO_Min, defValueRange_HCHO_Max ) )
			{
				return true;
			}
		}
		break;
	case IOT_DEVICE_PM25:
		{
			if( macIsValueOverRange( val, defValueRange_PM25_Min, defValueRange_PM25_Max ) )
			{
				return true;
			}
		}
		break;
			
	case IOT_DEVICE_Temperature:
		{
			if( macIsValueOverRange( val, defValueRange_Temperature_Min, defValueRange_Temperature_Max ) )
			{
				return true;
			}
		}
		break;

	case IOT_DEVICE_Humidity:
		{
			if( macIsValueOverRange( val, defValueRange_Humidity_Min, defValueRange_Humidity_Max ) )
			{
				return true;
			}
		}
		break;

	case IOT_DEVICE_Wind:
		{
			if( macIsValueOverRange( val, defValueRange_Wind_Min, defValueRange_Wind_Max ) )
			{
				return true;
			}
		}
		break;
	}

	return false;
}

float RS485DevControl::ValueRangeFix( IOTDeviceType DeviceType, float val )
{
	switch( DeviceType )
	{
	case IOT_DEVICE_CO2:
		{
			macValueRangeFix_Min( val, defValueRange_CO2_Min );
			macValueRangeFix_Max( val, defValueRange_CO2_Max );
		}
		break;
	case IOT_DEVICE_HCHO:
		{
			macValueRangeFix_Min( val, defValueRange_HCHO_Min );
			macValueRangeFix_Max( val, defValueRange_HCHO_Max );
		}
		break;
	case IOT_DEVICE_PM25:
		{
			macValueRangeFix_Min( val, defValueRange_PM25_Min );
			macValueRangeFix_Max( val, defValueRange_PM25_Max );
		}
		break;
	case IOT_DEVICE_Temperature:
		{
			macValueRangeFix_Min( val, defValueRange_Temperature_Min );
			macValueRangeFix_Max( val, defValueRange_Temperature_Max );
		}
		break;

	case IOT_DEVICE_Humidity:
		{
			macValueRangeFix_Min( val, defValueRange_Humidity_Min );
			macValueRangeFix_Max( val, defValueRange_Humidity_Max );
		}
		break;

	case IOT_DEVICE_Wind:
		{
			macValueRangeFix_Min( val, defValueRange_Wind_Min );
			macValueRangeFix_Max( val, defValueRange_Wind_Max );
		}
		break;
	}

	return val;
}

Tag* RS485DevControl::tag(const struTagParam &TagParam) const
{
	Tag* i = new Tag( defDeviceTypeTag_rs485device );
	
	if( TagParam.isValid && TagParam.isResult )
	{
	}
	else
	{
		i->addAttribute("device_id",util::int2string(m_deviceID));
		i->addAttribute("command",util::int2string(m_command));
		i->addAttribute("protocol",util::int2string(m_protocol));
		i->addAttribute("ver",m_ver);
	}

	if( !m_AddressQueue.empty() )
	{
		defAddressQueue::const_iterator it = m_AddressQueue.begin();
		defAddressQueue::const_iterator itEnd = m_AddressQueue.end();
		for( ; it!=itEnd; ++it )
		{
			if( (*it)->GetEnable() )
			{
				i->addChild( (*it)->tag(TagParam) );
			}
		}
	}

	return i;
}

ControlBase* RS485DevControl::clone( bool CreateLock ) const
{
	RS485DevControl *dev = new RS485DevControl( *this );
	dev->InitNewMutex( CreateLock );
	return dev;
}

std::string RS485DevControl::Print( const char *info, bool doPrint, DeviceAddress *const pSpecAddr ) const
{
	//jyc20160824
	//const int thisThreadId = ::GetCurrentThreadId();
	//pthread_t pthread_self();
	const int thisThreadId = ::pthread_self(); //jyc20160825add get threadid must test later

	char buf[256] = {0};
	DeviceAddress *addr = NULL;
	
	std::string strvalues;

	if( pSpecAddr )
	{
		addr = GetAddress( pSpecAddr->GetAddress() );
		if( addr )
		{
			strvalues = addr->GetCurValue().c_str();
		}
	}
	else
	{
		const defAddressQueue &AddrQue = this->GetAddressList();
		defAddressQueue::const_iterator itAddrQue = AddrQue.begin();
		for( ; itAddrQue!=AddrQue.end(); ++itAddrQue )
		{
			DeviceAddress *pOneAddr = *itAddrQue;
			if( !strvalues.empty() )
			{
				strvalues += ",";
			}
			strvalues += pOneAddr->GetCurValue();
		}
	}
	
	snprintf( buf, sizeof(buf), "RS485Ctl(%s) cnt=%d, 485id=%d, cmd=%d, addr=%d, val=(%s) -ThId%d\n", info?info:"", 
	         this->GetAddressList().size(), this->m_deviceID, this->m_command, addr?addr->GetAddress():0, 
	         strvalues.c_str(), thisThreadId );
	
	if( doPrint )
	{
		LOGMSG( buf );
	}

	return std::string(buf);
}

bool RS485DevControl::GetBtachReadParam( uint16_t &addressStart, uint16_t &addressCount, uint16_t &addressEnableCount ) const
{
	addressStart = 0;
	addressCount = m_AddressQueue.size();
	addressEnableCount = addressCount;

	if( addressCount<2 )
	{
		addressStart = 0;
		addressCount = 0;
		return false;
	}

	uint16_t addressPrev = 0;

	defAddressQueue::const_iterator itChkAddr = m_AddressQueue.begin();
	for( ; itChkAddr!=m_AddressQueue.end(); ++itChkAddr )
	{
		const uint16_t addressCur = (*itChkAddr)->GetAddress();

		if( !(*itChkAddr)->GetEnable() )
		{
			addressEnableCount--;
		}
		
		if( 0==addressCur )
		{
			addressStart = 0;
			addressCount = 0;
			return false;
		}

		if( 0==addressPrev )
		{
			addressStart = addressCur;
			addressPrev = addressStart;
		}
		else
		{
			if( addressStart >= addressCur )
			{
				addressStart = 0;
				addressCount = 0;
				return false;
			}

			addressPrev = addressCur;
		}
	}

	return ( addressCount>1 && addressEnableCount>1 );
}

bool RS485DevControl::Encode( defRS485MsgQue &que, DeviceAddress *const pSpecAddr, uint32_t SpecCmd )
{
	defProtocol_ protocol = (defProtocol_)m_protocol;
	if( defProtocol_Unknown == protocol )
	{
		protocol = defProtocol_Modbus;
	}
	
	if( defProtocol_Modbus != protocol )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "RS485DevControl::Encode protocol failed!!! protocol=%d\r\n", m_protocol );
		return false;
	}

	struRS485Msg msg;
	
	defAddressQueue::const_iterator it = m_AddressQueue.begin();
	defAddressQueue::const_iterator itEnd = m_AddressQueue.end();
	for( ; it!=itEnd; ++it )
	{
		DeviceAddress *pAddr = (*it);

		if( pSpecAddr )
		{
			pAddr = pSpecAddr;
		}

		bool isSuccess = false;
		std::string strErr("");
		msg.Reset();

		if( RS485DevControl::IsReadCmd(m_command) )
		{
			if( IOT_WRITE == pAddr->GetReadType() )
				continue;

			switch( pAddr->GetType() )
			{
			case IOT_DEVICE_Switch:
			case IOT_DEVICE_CO2:
			case IOT_DEVICE_HCHO:
			case IOT_DEVICE_PM25:
			case IOT_DEVICE_Temperature:
			case IOT_DEVICE_Humidity:
			case IOT_DEVICE_Wind:
				{
					uint32_t useModCmd = defModbusCmd_Read;
					if( SpecCmd )
					{
						useModCmd = SpecCmd;
					}
					else
					{
						if( IsUseRead04( pAddr->GetType(), this->m_ver ) )
						{
							useModCmd = defModbusCmd_Read04;
						}
					}

					uint16_t addressStart = 0;
					uint16_t addressCount = 0;
					uint16_t addressEnableCount = 0;
					if( !pSpecAddr )
					{
						GetBtachReadParam( addressStart, addressCount, addressEnableCount );
					}

					if( 0==addressStart || 0==addressCount )
					{
						addressStart = pAddr->GetAddress();
						addressCount = 1;
					}

					assert(addressStart);
					assert(addressCount);

					if( defModbusCmd_Read04==useModCmd )
					{
						isSuccess = ModbusProc::Encode_Read04_Request( msg.buf, msg.buflen, this->m_deviceID, addressStart, addressCount );
					}
					else
					{
						//default
						isSuccess = ModbusProc::Encode_Read03_Request( msg.buf, msg.buflen, this->m_deviceID, addressStart, addressCount );
					}
				}
				break;

			default:
				strErr = "type unsupport";
				break;
			}
		}
		else if( defModbusCmd_Write==m_command )
		{
			if( IOT_READ == pAddr->GetReadType() )
				continue;

			switch( pAddr->GetType() )
			{
			case IOT_DEVICE_CO2:
			case IOT_DEVICE_HCHO:
			//case IOT_DEVICE_PM25:
				//break;
			case IOT_DEVICE_Switch:
			case IOT_DEVICE_Temperature:
			case IOT_DEVICE_Humidity:
				if( this->m_ver == defRS485_Ver_Humidity_1601_201506A )
				{
					isSuccess = ModbusProc::Encode_Write1601_Request( msg.buf, msg.buflen, this->m_deviceID, pAddr->GetAddress(), atoi(pAddr->GetCurValue().c_str()) );
				}
				else
				{
					isSuccess = ModbusProc::Encode_Write06_Request( msg.buf, msg.buflen, this->m_deviceID, pAddr->GetAddress(), atoi(pAddr->GetCurValue().c_str()) );
				}
				break;

			case IOT_DEVICE_Wind:
				{
					if( this->m_ver == defRS485_Ver_Wind_1601_201501A )
					{
						isSuccess = ModbusProc::Encode_Write1601_Request( msg.buf, msg.buflen, this->m_deviceID, pAddr->GetAddress(), atoi(pAddr->GetCurValue().c_str()) );
					}
					else
					{
						isSuccess = ModbusProc::Encode_Write06_Request( msg.buf, msg.buflen, this->m_deviceID, pAddr->GetAddress(), atoi(pAddr->GetCurValue().c_str()) );
					}
				}
				break;

			default:
				strErr = "type unsupport";
				break;
			}
		}
		else
		{
			//LOGMSGEX( defLOGNAME, defLOG_ERROR, "RS485DevControl::Encode cmd failed!!! protocol=%d, cmd=%d\r\n", m_protocol, m_command );
			return false;
		}

		if( isSuccess )
		{
			struRS485Msg *pMsg = new struRS485Msg;
			memcpy( pMsg, &msg, sizeof(struRS485Msg) );
			que.push_back( pMsg );
		}
		else
		{
			//LOGMSGEX( defLOGNAME, defLOG_ERROR, "RS485Ctrl addr encode failed(%s)! devid=%d, cmd=%d, addr=%d, type=%d\r\n", strErr.c_str(), this->m_deviceID, this->m_command, pAddr->GetAddress(), pAddr->GetType() );
		}

		break; // only one
	}

	return true;
}

bool RS485DevControl::Decode( uint8_t* const originalBuf, const uint32_t buflen, RS485DevControl **pctrl )
{
	uint8_t devid = 0;
	uint8_t cmdcode = 0;

	uint32_t protocol = defProtocol_Modbus;
	std::string ver = c_DefaultVer;

	*pctrl = NULL;

	if( !ModbusProc::Decode_dev_cmd( originalBuf, buflen, devid, cmdcode ) )
	{
		return false;
	}

	switch( cmdcode )
	{
	case defModbusCmd_Read:
	case defModbusCmd_Read04:
		{
			std::vector<uint16_t> vecAddressData;
			if( defModbusCmd_Read04==cmdcode )
			{
				if( !ModbusProc::Decode_Read04_Response( originalBuf, buflen, vecAddressData ) )
				{
					return false;
				}
			}
			else
			{
				//default
				if( !ModbusProc::Decode_Read03_Response( originalBuf, buflen, vecAddressData ) )
				{
					return false;
				}
			}

			RS485DevControl *pnewCtrl = new RS485DevControl( devid, cmdcode, protocol, ver );

			std::vector<uint16_t>::const_iterator it = vecAddressData.begin();
			std::vector<uint16_t>::const_iterator itEnd = vecAddressData.end();
			for( ; it!=itEnd; ++it )
			{
				DeviceAddress *pnewAddr = new DeviceAddress( uint32_t(0) );
				pnewAddr->SetCurValue( (*it) );
				pnewCtrl->AddAddress( pnewAddr );
			}
			*pctrl = pnewCtrl;
		}
		break;

	case defModbusCmd_Write:
		{
			uint16_t address = 0;
			uint16_t addressValue = 0;
			if( !ModbusProc::Decode_Write06_Response( originalBuf, buflen, address, addressValue ) )
			{
				return false;
			}

			RS485DevControl *pnewCtrl = new RS485DevControl( devid, cmdcode, protocol, ver );

			DeviceAddress *pnewAddr = new DeviceAddress( address );
			pnewAddr->SetCurValue( addressValue );
			pnewCtrl->AddAddress( pnewAddr );

			*pctrl = pnewCtrl;
		}
		break;

	case defModbusCmd_Err_Read:
	case defModbusCmd_Err_Read04:
		{
			uint8_t ErrCode = 0;
			if( defModbusCmd_Err_Read04==cmdcode )
			{
				if( !ModbusProc::Decode_Read04_Err_Response( originalBuf, buflen, ErrCode ) )
				{
					return false;
				}
			}
			else
			{
				//default
				if( !ModbusProc::Decode_Read03_Err_Response( originalBuf, buflen, ErrCode ) )
				{
					return false;
				}
			}

			RS485DevControl *pnewCtrl = new RS485DevControl( devid, cmdcode, protocol, ver );
			pnewCtrl->SetReturnErrCode( ErrCode );

			*pctrl = pnewCtrl;
		}
		break;

	case defModbusCmd_Err_Write:
		{
			uint8_t ErrCode = 0;
			if( !ModbusProc::Decode_Write06_Err_Response( originalBuf, buflen, ErrCode ) )
			{
				return false;
			}

			RS485DevControl *pnewCtrl = new RS485DevControl( devid, cmdcode, protocol, ver );
			pnewCtrl->SetReturnErrCode( ErrCode );

			*pctrl = pnewCtrl;
		}
		break;
	}

	return true;
}

defUseable RS485DevControl::set_NetUseable( defUseable NetUseable, bool *isChanged )
{
	if( !m_pmutex_RS485DevCtl ) return defUseable_Null;

	gloox::util::MutexGuard mutexguard( m_pmutex_RS485DevCtl );

	return set_NetUseable_nolock( NetUseable, isChanged );
}

defUseable RS485DevControl::set_NetUseable_nolock( defUseable NetUseable, bool *isChanged )
{
	if( m_NetUseable != NetUseable )
	{
		if( isChanged )
		{
			*isChanged = true;
		}

		LOGMSG( "RS485DevCtl(%s) set_NetUseable old=%d, new=%d\r\n", m_Name.c_str(), m_NetUseable, NetUseable );
	}
	else
	{
		if( isChanged )
		{
			*isChanged = false;
		}
	}

	m_NetUseable = NetUseable;

	if( defUseable_OK == m_NetUseable )
	{
		m_failedCount = 0;
		m_lastSuccessTS = timeGetTime();
	}

	return m_NetUseable;
}

defUseable RS485DevControl::get_NetUseable()
{
	if( !m_pmutex_RS485DevCtl ) return defUseable_Null;

	gloox::util::MutexGuard mutexguard( m_pmutex_RS485DevCtl );

	return m_NetUseable;
}

void RS485DevControl::check_NetUseable_RecvFailed( bool *isChanged )
{
	if( !m_pmutex_RS485DevCtl ) return ;

	gloox::util::MutexGuard mutexguard( m_pmutex_RS485DevCtl );

	if( m_NetUseable < 0 )
		return;

	const uint32_t Dev_RecvFailedCount_main = RUNCODE_Get(defCodeIndex_Dev_RecvFailedCount,defRunCodeValIndex_1);
	const uint32_t Dev_RecvFailedCount_small = RUNCODE_Get(defCodeIndex_Dev_RecvFailedCount,defRunCodeValIndex_2);
	const uint32_t Dev_RecvFailedTime_main = RUNCODE_Get(defCodeIndex_Dev_RecvFailedTime,defRunCodeValIndex_1);
	const uint32_t Dev_RecvFailedTime_small = RUNCODE_Get(defCodeIndex_Dev_RecvFailedTime,defRunCodeValIndex_2);

	const uint32_t failedtime = timeGetTime()-m_lastSuccessTS;
	m_failedCount++;
	if( ( m_failedCount > Dev_RecvFailedCount_main && failedtime > Dev_RecvFailedTime_main*1000 )
		||
		( m_failedCount > Dev_RecvFailedCount_small && failedtime > Dev_RecvFailedTime_small*1000 )
		)
	{
		set_NetUseable_nolock( defUseable_ErrNoData, isChanged );
	}
}
