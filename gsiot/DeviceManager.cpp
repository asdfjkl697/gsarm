#include "DeviceManager.h"
#include "gloox/util.h"
//#include "IPCameraBase.h"

DeviceManager::DeviceManager(void)
	:SQLiteHelper()
{
	// load first
	LoadDB_rfsignal();

	// load other
	LoadDB_rf_trigger();

	LoadDB_rf_remote();

	SQLite::Statement rf_m_query(*this->db,"select * from devices_rf_module order by id desc");
	while(rf_m_query.executeStep()){
		int id = rf_m_query.getColumn(0);
		std::string name = rf_m_query.getColumn(1);
		std::string ver = rf_m_query.getColumn(2);
		std::string serialno = rf_m_query.getColumn(3);
		std::string factoryno = rf_m_query.getColumn(4);
		uint32_t pro_id =  rf_m_query.getColumn(5).getInt();
		uint32_t pass_code =  rf_m_query.getColumn(6).getInt();
		uint32_t addr_count =  rf_m_query.getColumn(7).getInt();
		uint8_t addr_type =  rf_m_query.getColumn(8).getInt();
		
		RFDevice *dev = new RFDevice(pro_id,pass_code,serialno,name,addr_count,addr_type);

		std::string sql = "select * from address where mudule_type="+util::int2string(CC1101_433)+" and module_id="+util::int2string(id);
		SQLite::Statement addr_query(*this->db,sql.c_str());
	    while(addr_query.executeStep()){
			uint32_t addr_data = addr_query.getColumn(3).getInt();
			DeviceAddress *addr=new DeviceAddress(addr_data);
			int col = 4;
			if( !addr_query.isColumnNull(col) ) { addr->SetName(addr_query.getColumn(col)); } col++;
			addr->SetDeviceType((IOTDeviceType)addr_query.getColumn(col++).getInt());
			addr->SetReadType((IOTDeviceReadType)addr_query.getColumn(col++).getInt());
			addr->SetDataType((DataType)addr_query.getColumn(col++).getInt());
			addr->SetDefualtValue(addr_query.getColumn(col++));
			addr->SetMinValue(addr_query.getColumn(col++));
			addr->SetMaxValue(addr_query.getColumn(col++));
			addr->set_save_col_val_enable((uint32_t)addr_query.getColumn(col++).getInt());
			dev->AddAddress(addr);
		}
		RFDeviceControl *rfControl = new RFDeviceControl(dev);
		GSIOTDevice *iotdev = new GSIOTDevice(id,name,IOT_DEVICE_RFDevice,ver,serialno,factoryno,rfControl);
		m_devices.push_back(iotdev);
	}
	SQLite::Statement can_query(*this->db,"select * from devices_canbus order by id desc");
	while(can_query.executeStep()){
		int id = can_query.getColumn(0);
		std::string name = can_query.getColumn(1);
		std::string ver = can_query.getColumn(2);
		std::string serialno = can_query.getColumn(3);
		std::string factoryno = can_query.getColumn(4);
		uint32_t device_id = can_query.getColumn(5).getInt();
		uint32_t pro_id =  can_query.getColumn(6).getInt();
		uint32_t addr_count =  can_query.getColumn(7).getInt();

		CANDeviceControl *ctl = new CANDeviceControl(device_id,pro_id,addr_count);
		ctl->SetName(name);

		std::string sql = "select * from address where mudule_type="+util::int2string(CAN_Chip)+" and module_id="+util::int2string(id);
		SQLite::Statement addr_query(*this->db,sql.c_str());
	    while(addr_query.executeStep()){
			uint32_t addr_data = addr_query.getColumn(3).getInt();
			DeviceAddress *addr=new DeviceAddress(addr_data);
			int col = 4;
			if( !addr_query.isColumnNull(col) ) { addr->SetName(addr_query.getColumn(col)); } col++;
			addr->SetDeviceType((IOTDeviceType)addr_query.getColumn(col++).getInt());
			addr->SetReadType((IOTDeviceReadType)addr_query.getColumn(col++).getInt());
			addr->SetDataType((DataType)addr_query.getColumn(col++).getInt());
			addr->SetDefualtValue(addr_query.getColumn(col++));
			addr->SetMinValue(addr_query.getColumn(col++));
			addr->SetMaxValue(addr_query.getColumn(col++));
			addr->set_save_col_val_enable((uint32_t)addr_query.getColumn(col++).getInt());
			ctl->AddAddress(addr);
		}
		GSIOTDevice *iotdev = new GSIOTDevice(id,name,IOT_DEVICE_CANDevice,ver,serialno,factoryno,ctl);
		m_devices.push_back(iotdev);
	}

	LoadDB_RS485();

	m_mapRFSignal.clear();
}


DeviceManager::~DeviceManager(void)
{
	while( !m_devices.empty() )
	{
		GSIOTDevice *p = m_devices.front();
		delete(p);
		m_devices.pop_front();
	}
}

GSIOTDevice* DeviceManager::GetIOTDevice( IOTDeviceType deviceType, uint32_t deviceId ) const
{
	if( !m_devices.empty() )
	{
		std::list<GSIOTDevice*>::const_iterator it = m_devices.begin();
		for( ;it!=m_devices.end(); ++it )
		{
			if( (*it)->getType() == deviceType
				&& (*it)->getId() == deviceId )
			{
				return (*it);
			}
		}
	}

	return NULL;
}

int DeviceManager::AddController(ControlBase* controller, const std::string &ver)
{
	GSIOTDevice *dev = new GSIOTDevice(m_devices.size()+1,controller->GetName(),controller->GetType(),ver,"","", controller);
	m_devices.push_back(dev);
	dev->setId( this->SaveToDB(dev) );

	return dev->getId();
}

void DeviceManager::AddTempController(ControlBase* controller, const std::string &ver)
{
	GSIOTDevice *dev = new GSIOTDevice(m_devices.size()+1,controller->GetName(),controller->GetType(),ver,"","", controller);
	m_devices.push_back(dev);
}

bool DeviceManager::Add_remote_button( GSIOTDevice *iotdevice, RemoteButton *pRemoteButton )
{
	if( !iotdevice
		|| !pRemoteButton
		|| IOT_DEVICE_Remote != iotdevice->getType()
		|| !iotdevice->getControl() )
	{
		return false;
	}

	if( pRemoteButton->GetObjName().empty() )
	{
		pRemoteButton->SetName( "addr" );
	}

	RFRemoteControl *ctl = (RFRemoteControl*)iotdevice->getControl();
	pRemoteButton->SetId( SaveToDB_remote_button( iotdevice->getType(), iotdevice->getId(), pRemoteButton ) );

	ctl->AddButton( pRemoteButton );

	return true;
}

bool DeviceManager::Add_DeviceAddress( GSIOTDevice *iotdevice, DeviceAddress *pAddr )
{
	if( !iotdevice
		|| !pAddr
		|| IOT_DEVICE_RS485 != iotdevice->getType()
		|| !iotdevice->getControl() )
	{
		return false;
	}

	if( pAddr->GetName().empty() )
	{
		pAddr->SetName( "addr" );
	}

	RS485DevControl *ctl = (RS485DevControl*)iotdevice->getControl();
	ctl->AddAddress( pAddr );

	SaveToDB_DeviceAddress( iotdevice->getType(), iotdevice->getId(), pAddr );
	return true;
}

bool DeviceManager::Add_Preset( GSIOTDevice *iotdevice, CPresetObj *pPreset )
{
	if( !iotdevice
		|| !pPreset
		|| IOT_DEVICE_Camera != iotdevice->getType() // 目前只有摄像机有预置点
		|| !iotdevice->getControl() )
	{
		return false;
	}

	if( pPreset->GetObjName().empty() )
	{
		pPreset->SetName( "preset" );
	}

	// 允许添加 index 相同的预置位配置，比如别名
	/*
	IPCameraBase *ctl = (IPCameraBase*)iotdevice->getControl();

	defGSReturn ret = ctl->CheckExist( 0, pPreset->GetObjName(), NULL );
	if( macGSFailed(ret) )
	{
		return false;
	}

	if( !ctl->SendPTZ( GSPTZ_Add_Preset, pPreset->GetIndex() ) )
	{
		LOGMSG( "DeviceManager::Add_Preset failed!" );
		return false;
	}
	
	pPreset->SetId( SaveToDB_Preset( iotdevice->getType(), iotdevice->getId(), pPreset ) );

	ctl->AddPreset( pPreset );
	*/

	return true;
}

bool DeviceManager::DeleteDevice( GSIOTDevice *iotdevice )
{
	if( !iotdevice )
		return false;

	iotdevice->SetEnable( defDeviceDisable );

	char chTable[64] = {0};
	char sqlbuf[256] = {0};

	switch(iotdevice->getType())
	{
	case IOT_DEVICE_RS485:
		//strcpy_s( chTable, sizeof(chTable),"devices_rs485" );
		memcpy( chTable, "devices_rs485", sizeof(chTable)); //jyc20160921
		break;

	case IOT_DEVICE_Trigger:
		{
			TriggerControl *ctl = (TriggerControl*)iotdevice->getControl();
			if( ctl )
			{
				DB_Delete_rfsignal( ctl->GetSignal().id );
			}

			//strcpy_s( chTable, sizeof(chTable),"devices_rf_trigger" );
			memcpy( chTable,"devices_rf_trigger", sizeof(chTable) );
		}
		break;

	case IOT_DEVICE_Remote:
		{
			DB_Delete_remote_button( iotdevice );

			//strcpy_s( chTable, sizeof(chTable),"devices_rf_remote" );
			memcpy( chTable,"devices_rf_remote", sizeof(chTable) );
		}
		break;

	default:
		return false;
	}

	DB_DeleteDeviceAddress( iotdevice );

	snprintf( sqlbuf, sizeof(sqlbuf), "DELETE FROM %s WHERE id=%d", chTable, iotdevice->getId() );
	db->exec( sqlbuf );

	this->m_devices.remove( iotdevice );

	g_Changed( defCfgOprt_Delete, iotdevice->getType(), iotdevice->getId() );
	return true;
}

bool DeviceManager::Delete_remote_button( GSIOTDevice *iotdevice, RemoteButton *pRemoteButton )
{
	if( !iotdevice
		|| IOT_DEVICE_Remote != iotdevice->getType()
		|| !iotdevice->getControl() )
	{
		return false;
	}

	RFRemoteControl *pctl = (RFRemoteControl*)iotdevice->getControl();
	if( !pctl )
		return false;

	DB_DeleteOne_remote_button( iotdevice->getType(), iotdevice->getId(), pRemoteButton );
	pctl->DeleteButton( pRemoteButton->GetId() ); 

	return true;
}

bool DeviceManager::Delete_DeviceAddress( GSIOTDevice *iotdevice, uint32_t address )
{
	if( !iotdevice
		|| IOT_DEVICE_RS485 != iotdevice->getType()
		|| !iotdevice->getControl() )
	{
		return false;
	}

	RS485DevControl *pctl = (RS485DevControl*)iotdevice->getControl();
	if( !pctl )
		return false;

	DB_DeleteOne_DeviceAddress( iotdevice->getType(), iotdevice->getId(), address );
	pctl->DeleteAddress( address );

	return true;
}

bool DeviceManager::Delete_Preset( GSIOTDevice *iotdevice, uint32_t Presetid )
{
	if( !iotdevice
		|| IOT_DEVICE_Camera != iotdevice->getType()
		|| !iotdevice->getControl() )
	{
		return false;
	}
	/*
	IPCameraBase *pctl = (IPCameraBase*)iotdevice->getControl();
	if( !pctl )
		return false;

	// 为了删除时速度快点，不删除摄像机上的信息，如果有需要删除则再加代码

	DB_DeleteOne_Preset( Presetid, iotdevice->getType(), iotdevice->getId() );
	pctl->DeletePreset( Presetid );
	*/
	return true;
}

bool DeviceManager::DB_DeleteDeviceAddress( GSIOTDevice *iotdevice )
{
	if( !iotdevice )
		return false;

	char sqlbuf[256] = {0};
	int mudule_type = 0;

	switch( iotdevice->getType() )
	{
	case IOT_DEVICE_RS485:
		mudule_type = Module_RS485;
		break;

	default:
		return false;
	}

	snprintf( sqlbuf, sizeof(sqlbuf), "DELETE FROM address where mudule_type=%d and module_id=%d", mudule_type, iotdevice->getId() );
	db->exec( sqlbuf );

	g_Changed( defCfgOprt_Delete, iotdevice->getType(), iotdevice->getId(), defIOTDeviceAddr_AllAddr );
	return true;
}

bool DeviceManager::DB_Delete_rfsignal( const int signal_id )
{
	char sqlbuf[256] = {0};
	snprintf( sqlbuf, sizeof(sqlbuf), "DELETE FROM rfsignal where id=%d", signal_id );
	db->exec( sqlbuf );

	return true;
}

bool DeviceManager::DB_Delete_remote_button( GSIOTDevice *iotdevice )
{
	if( !iotdevice )
		return false;

	RemoteButtonManager *btnmgr = NULL;

	switch( iotdevice->getType() )
	{
	case IOT_DEVICE_Remote:
		{
			RFRemoteControl *ctl = (RFRemoteControl *)iotdevice->getControl();
			if( ctl )
			{
				btnmgr = (RemoteButtonManager*)ctl;
			}
		}
		break;

	default:
		return false;
	}

	if( btnmgr )
	{
		const defButtonQueue &que = btnmgr->GetButtonList();
		defButtonQueue::const_iterator it = que.begin();
		defButtonQueue::const_iterator itEnd = que.end();
		for( ; it!=itEnd; ++it )
		{
			RemoteButton *pCurButton = *it;

			DB_Delete_rfsignal( pCurButton->GetSignal().id );
		}
	}

	char sqlbuf[256] = {0};
	snprintf( sqlbuf, sizeof(sqlbuf), "DELETE FROM remote_button where device_type=%d and device_id=%d", iotdevice->getType(), iotdevice->getId() );
	db->exec( sqlbuf );

	g_Changed( defCfgOprt_Delete, iotdevice->getType(), iotdevice->getId(), defIOTDeviceAddr_AllAddr );
	return true;
}

bool DeviceManager::DB_DeleteOne_remote_button( IOTDeviceType device_type, int device_id, RemoteButton *pRemoteButton )
{
	DB_Delete_rfsignal( pRemoteButton->GetSignal().id );

	char sqlbuf[256] = {0};
	snprintf( sqlbuf, sizeof(sqlbuf), "DELETE FROM remote_button WHERE id=%d", pRemoteButton->GetId() );
	db->exec( sqlbuf );

	g_Changed( defCfgOprt_Delete, device_type, device_id, pRemoteButton->GetId() );
	return true;
}

bool DeviceManager::DB_DeleteOne_DeviceAddress( IOTDeviceType device_type, int device_id, uint32_t address )
{
	int mudule_type = 0;
	switch( device_type )
	{
	case IOT_DEVICE_RS485:
		mudule_type = Module_RS485;
		break;

	default:
		return false;
	}

	char sqlbuf[256] = {0};
	snprintf( sqlbuf, sizeof(sqlbuf), "DELETE FROM address where mudule_type=%d and module_id=%d and addr=%d", mudule_type, device_id, address );
	db->exec( sqlbuf );

	g_Changed( defCfgOprt_Delete, device_type, device_id, address );
	return true;
}

bool DeviceManager::DB_DeleteOne_Preset( uint32_t Presetid, IOTDeviceType device_type, int device_id )
{
	char sqlbuf[1024] = {0};

	snprintf( sqlbuf, sizeof(sqlbuf), "DELETE FROM dev_preset WHERE device_type=%d and device_id=%d and id=%d", device_type, device_id, Presetid );

	db->exec( sqlbuf );

	g_Changed( defCfgOprt_Delete, device_type, device_id, defIOTDeviceAddr_Other );
	return true;
}

bool DeviceManager::DB_ModifyDevice( GSIOTDevice *iotdevice, uint32_t ModifySubFlag )
{
	if( !iotdevice )
		return false;
	
	if( iotdevice->getName().empty() )
	{
		return false;
	}

	char sqlbuf[1024] = {0};

	switch(iotdevice->getType())
	{
	case IOT_DEVICE_RS485:
		{
			RS485DevControl *pctl = (RS485DevControl*)iotdevice->getControl();
			if( !pctl )
				return false;

			pctl->set_NetUseable( defUseable_OK );

			snprintf( sqlbuf, sizeof(sqlbuf), "update devices_rs485 set name='%s',ver='%s', device_id=%d, enable=%d WHERE id=%d", iotdevice->getName().c_str(), iotdevice->getVer().c_str(), pctl->GetDeviceid(), iotdevice->GetEnable(), iotdevice->getId() );
		}
		break;

	case IOT_DEVICE_Trigger:
		{
			TriggerControl *pctl = (TriggerControl*)iotdevice->getControl();
			if( !pctl )
				return false;

			if( ModifySubFlag )
			{
				DB_Modify_rfsignal_s( pctl->GetSignal() );
			}

			snprintf( sqlbuf, sizeof(sqlbuf), "update devices_rf_trigger set name='%s',ver='%s', enable=%d , runstate=%d WHERE id=%d", iotdevice->getName().c_str(), iotdevice->getVer().c_str(), iotdevice->GetEnable(), (int)pctl->GetAGRunState(), iotdevice->getId() );
		}
		break;

	case IOT_DEVICE_Remote:
		{
			RFRemoteControl *pctl = (RFRemoteControl*)iotdevice->getControl();
			if( !pctl )
				return false;

			std::string serialno = pctl->Save_RefreshToSave();

			snprintf( sqlbuf, sizeof(sqlbuf), "update devices_rf_remote set name='%s',ver='%s',serialno='%s',enable=%d WHERE id=%d", iotdevice->getName().c_str(), iotdevice->getVer().c_str(), serialno.c_str(), iotdevice->GetEnable(), iotdevice->getId() );
		}
		break;

	default:
		return false;
	}

	db->exec( sqlbuf );

	g_Changed( defCfgOprt_Modify, iotdevice->getType(), iotdevice->getId() );
	return true;
}

bool DeviceManager::DB_ModifyAddress( GSIOTDevice *iotdevice, const DeviceAddress *addr )
{
	if( !iotdevice || !addr )
		return false;

	char sqlbuf[1024] = {0};
	int mudule_type = 0;

	switch( iotdevice->getType() )
	{
	case IOT_DEVICE_RS485:
		mudule_type = Module_RS485;
		break;

	default:
		return false;
	}

	snprintf( sqlbuf, sizeof(sqlbuf), "update address set name='%s',defualt_value='%s',min_value='%s',max_value='%s',enable=%d where mudule_type=%d and module_id=%d and addr=%d", 
		addr->GetName().c_str(), addr->GetDefualtValue().c_str(), addr->GetMinValue().c_str(), addr->GetMaxValue().c_str(), addr->get_save_col_val_enable(),
		mudule_type, iotdevice->getId(), addr->GetAddress() );

	db->exec( sqlbuf );

	g_Changed( defCfgOprt_Modify, iotdevice->getType(), iotdevice->getId(), addr->GetAddress() );
	return true;
}

bool DeviceManager::DB_Modify_rfsignal_s( const RFSignal &signal )
{
	char sqlbuf[1024] = {0};

	snprintf( sqlbuf, sizeof(sqlbuf), "update rfsignal set signal_count=%d WHERE id=%d", signal.signal_count, signal.id );

	db->exec( sqlbuf );
	return true;
}

bool DeviceManager::DB_Modify_rfsignal( const RFSignal &signal )
{
	SQLite::Statement   query(*db,"UPDATE rfsignal SET signal_type=?,freq=?,code=?,codeValidLen=?,"\
		"one_high_time=?,one_low_time=?,zero_high_time=?,zero_low_time=?,"\
		"silent_interval=?,signal_count=?,headlen=?,taillen=?,"\
		"headcode1=?,headcode2=?,headcode3=?,headcode4=?,headcode5=?,headcode6=?,headcode7=?,"\
		"tailcode1=?,tailcode2=?,tailcode3=?,tailcode4=?,tailcode5=?,tailcode6=?,tailcode7=?,"\
		"original_headflag=?,original_headtime=?,original=? WHERE id=?");

	int col = 1;

	query.bind(col++, (int)signal.signal_type);
	query.bind(col++, (int)signal.freq);
	query.bind(col++, (int)signal.code);
	query.bind(col++, (int)signal.codeValidLen);

	query.bind(col++, (int)signal.one_high_time);
	query.bind(col++, (int)signal.one_low_time);
	query.bind(col++, (int)signal.zero_high_time);
	query.bind(col++, (int)signal.zero_low_time);

	query.bind(col++, (int)signal.silent_interval);
	query.bind(col++, (int)signal.signal_count);
	query.bind(col++, (int)signal.headlen);
	query.bind(col++, (int)signal.taillen);

	query.bind(col++, (int)signal.headcode[0]);
	query.bind(col++, (int)signal.headcode[1]);
	query.bind(col++, (int)signal.headcode[2]);
	query.bind(col++, (int)signal.headcode[3]);
	query.bind(col++, (int)signal.headcode[4]);
	query.bind(col++, (int)signal.headcode[5]);
	query.bind(col++, (int)signal.headcode[6]);

	query.bind(col++, (int)signal.tailcode[0]);
	query.bind(col++, (int)signal.tailcode[1]);
	query.bind(col++, (int)signal.tailcode[2]);
	query.bind(col++, (int)signal.tailcode[3]);
	query.bind(col++, (int)signal.tailcode[4]);
	query.bind(col++, (int)signal.tailcode[5]);
	query.bind(col++, (int)signal.tailcode[6]);

	query.bind(col++, (int)signal.original_headflag);
	query.bind(col++, (int)signal.original_headtime);
	query.bind(col++, signal.Get_original() );

	query.bind(col++, (int)signal.id);

	query.exec();
	return true;
}

bool DeviceManager::DB_Modify_remote_button( IOTDeviceType device_type, int device_id, const RemoteButton *pRemoteButton )
{
	if( !pRemoteButton )
		return false;

	char sqlbuf[1024] = {0};

	snprintf( sqlbuf, sizeof(sqlbuf), "update remote_button set name='%s', enable=%d WHERE id=%d", pRemoteButton->GetObjName().c_str(), pRemoteButton->get_enable_col_save(), pRemoteButton->GetId() );

	db->exec( sqlbuf );

	g_Changed( defCfgOprt_Modify, device_type, device_id, pRemoteButton->GetId() );
	return true;
}

bool DeviceManager::DB_CleanSignal_remote_button( RemoteButton *pRemoteButton )
{
	if( !pRemoteButton )
		return false;

	DB_Delete_rfsignal( pRemoteButton->GetSignal().id );

	char sqlbuf[1024] = {0};
	snprintf( sqlbuf, sizeof(sqlbuf), "update remote_button set rfsignal_id=0 WHERE id=%d", pRemoteButton->GetId() );
	db->exec( sqlbuf );

	RFSignal::Init( pRemoteButton->GetSignal() );

	return true;
}

bool DeviceManager::DB_Modify_Preset( const CPresetObj *pPreset, IOTDeviceType device_type, int device_id )
{
	if( !pPreset )
		return false;

	char sqlbuf[1024] = {0};

	snprintf( sqlbuf, sizeof(sqlbuf), "update dev_preset set name='%s', enable=%d, preset_index=%d, sort_no=%d WHERE id=%d and device_type=%d and device_id=%d",
		pPreset->GetObjName().c_str(), pPreset->GetEnable(), pPreset->GetIndex(), pPreset->GetSortNo(),
		pPreset->GetId(), device_type, device_id );

	db->exec( sqlbuf );

	g_Changed( defCfgOprt_Modify, device_type, device_id, defIOTDeviceAddr_Other );
	return true;
}

bool DeviceManager::Swap_Preset( CPresetObj *pPresetA, CPresetObj *pPresetB, IOTDeviceType device_type, int device_id )
{
	if( !pPresetA || !pPresetB )
		return false;

	pPresetA->SwapInfo( *pPresetB );

	DB_Modify_Preset(pPresetA, device_type, device_id);
	DB_Modify_Preset(pPresetB, device_type, device_id);

	return true;
}

bool DeviceManager::SaveSort_Preset( GSIOTDevice *iotdevice )
{
	if( !iotdevice
		|| IOT_DEVICE_Camera != iotdevice->getType()
		|| !iotdevice->getControl() )
	{
		return false;
	}
	/*
	const IPCameraBase *ctl = (IPCameraBase*)iotdevice->getControl();
	if( !ctl )
		return false;

	UseDbTransAction dbta(db);

	char sqlbuf[1024] = {0};
	snprintf( sqlbuf, sizeof(sqlbuf), "update dev_preset set sort_no=0 WHERE device_type=%d and device_id=%d", iotdevice->getType(), iotdevice->getId() );
	db->exec( sqlbuf );

	const defPresetQueue& PresetList = ctl->GetPresetList();
	for( defPresetQueue::const_iterator it=PresetList.begin(); it!=PresetList.end(); ++it )
	{
		const CPresetObj *pPreset = *it;
		DB_Modify_Preset( pPreset, iotdevice->getType(), iotdevice->getId() );
	}
	*/
	return true;
}

int DeviceManager::SaveToDB(GSIOTDevice* device)
{
	switch(device->getType()){
		case IOT_DEVICE_Trigger:
			{
				return this->SaveToDB_rf_trigger( device );
			}
		case IOT_DEVICE_Remote:
			{
				return this->SaveToDB_rf_remote( device );
				break;
			}
		case IOT_DEVICE_RFDevice:
			{
				SQLite::Statement  query(*db,"INSERT INTO devices_rf_module VALUES(NULL,:name,:ver,:serialno,:factoryno,"\
					":pro_id,:pass_code,:addr_count,:addr_type)");
				query.bind(1, device->getName());
				query.bind(2, device->getVer());
				query.bind(3, c_NullStr);
				query.bind(4, c_NullStr);

				RFDeviceControl *rfControl = (RFDeviceControl *)device->getControl();
				if(!rfControl) break;
				RFDevice *dev = rfControl->GetDevice();
				query.bind(3, dev->GetSN());
				query.bind(5, (int)dev->GetProductid());
				query.bind(6, (int)dev->GetPasscode());
				query.bind(7, (int)dev->GetAddressCount());
				query.bind(8, (int)dev->GetAddressType());
				query.exec();

				int id = db->execAndGet("SELECT id FROM devices_rf_module ORDER BY id DESC LIMIT 1");

				std::list<DeviceAddress*> addrList = dev->GetAddressList();
				if(addrList.size()>0){
					std::list<DeviceAddress*>::const_iterator ita = addrList.begin();
					for(;ita!=addrList.end();ita++){
						SQLite::Statement  address_query(*db,"INSERT INTO address VALUES(NULL,:mudule_type,:module_id,"\
                                ":addr,:name,:type,:readtype,:datatype,:defualt_value,:min_value,:max_value,:enable)");
						address_query.bind(1,CC1101_433);
						int col = 2;
						address_query.bind(col++,id);
						address_query.bind(col++,(int)(*ita)->GetAddress());
						address_query.bind(col++,(*ita)->GetName());
						address_query.bind(col++,(int)(*ita)->GetType());
						address_query.bind(col++,(int)(*ita)->GetReadType());
						address_query.bind(col++,(int)(*ita)->GetDataType());
						address_query.bind(col++,(*ita)->GetDefualtValue());
						address_query.bind(col++,(*ita)->GetMinValue());
						address_query.bind(col++,(*ita)->GetMaxValue());
						address_query.bind(col++,(int)(*ita)->get_save_col_val_enable());
						address_query.exec();
					}
				}

				g_Changed( defCfgOprt_Add, device->getType(), id );
				return id;
			}
		case IOT_DEVICE_CANDevice:
			{				
				SQLite::Statement  query(*db,"INSERT INTO devices_canbus VALUES(NULL,:name,:ver,:serialno,:factoryno,:device_id,:pro_id,:addr_count)");
				query.bind(1, device->getName());
				query.bind(2, device->getVer());
				query.bind(3, c_NullStr);
				query.bind(4, c_NullStr);
								
				CANDeviceControl *ctl = (CANDeviceControl *)device->getControl();
				if(!ctl) break;
				query.bind(5, (int)ctl->GetDeviceid());
				query.bind(6, (int)ctl->GetProductid());
				query.bind(7, (int)ctl->GetAddressCount());
				query.exec();

				int id = db->execAndGet("SELECT id FROM devices_canbus ORDER BY id DESC LIMIT 1");

				std::list<DeviceAddress*> addrList = ctl->GetAddressList();
				if(addrList.size()>0){
					std::list<DeviceAddress*>::const_iterator ita = addrList.begin();
					for(;ita!=addrList.end();ita++){
						SQLite::Statement  address_query(*db,"INSERT INTO address VALUES(NULL,:mudule_type,:module_id,"\
                                ":addr,:name,:type,:readtype,:datatype,:defualt_value,:min_value,:max_value,:enable)");
						address_query.bind(1,CAN_Chip);
						int col = 2;
						address_query.bind(col++,id);
						address_query.bind(col++,(int)(*ita)->GetAddress());
						address_query.bind(col++,(*ita)->GetName());
						address_query.bind(col++,(int)(*ita)->GetType());
						address_query.bind(col++,(int)(*ita)->GetReadType());
						address_query.bind(col++,(int)(*ita)->GetDataType());
						address_query.bind(col++,(*ita)->GetDefualtValue());
						address_query.bind(col++,(*ita)->GetMinValue());
						address_query.bind(col++,(*ita)->GetMaxValue());
						address_query.bind(col++,(int)(*ita)->get_save_col_val_enable());
						address_query.exec();
					}
				}

				g_Changed( defCfgOprt_Add, device->getType(), id );
				return id;
			}

		case IOT_DEVICE_RS485:
			{
				return SaveToDB_RS485( device );
			}
		}
		return 0;
}

bool DeviceManager::LoadDB_rfsignal()
{
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "LoadDB_rfsignal" );

	SQLite::Statement query(*this->db,"select * from rfsignal order by id");
	while(query.executeStep()){

		RFSignal signal;
		memset( &signal, 0, sizeof(signal) );

		int col = 0;
		signal.id = query.getColumn(col++);

		signal.signal_type = (defRFSignalType)query.getColumn(col++).getInt();
		signal.freq = (defFreq)query.getColumn(col++).getInt();
//#ifdef OS_UBUNTU_FLAG  //jyc20170508 modify
#ifndef OS_OPENWRT
		signal.code = query.getColumn(col++).getInt();
		signal.codeValidLen = query.getColumn(col++).getInt();		
		signal.one_high_time =  query.getColumn(col++).getInt();
		signal.one_low_time =  query.getColumn(col++).getInt();
		signal.zero_high_time = query.getColumn(col++).getInt();
		signal.zero_low_time =  query.getColumn(col++).getInt();

		signal.silent_interval =  query.getColumn(col++).getInt();
		signal.signal_count = query.getColumn(col++).getInt();
		signal.headlen = query.getColumn(col++).getInt();
		signal.taillen = query.getColumn(col++).getInt();

		signal.headcode[0] = query.getColumn(col++).getInt();
		signal.headcode[1] = query.getColumn(col++).getInt();
		signal.headcode[2] = query.getColumn(col++).getInt();
		signal.headcode[3] = query.getColumn(col++).getInt();
		signal.headcode[4] = query.getColumn(col++).getInt();
		signal.headcode[5] = query.getColumn(col++).getInt();
		signal.headcode[6] = query.getColumn(col++).getInt();

		signal.tailcode[0] = query.getColumn(col++).getInt();
		signal.tailcode[1] = query.getColumn(col++).getInt();
		signal.tailcode[2] = query.getColumn(col++).getInt();
		signal.tailcode[3] = query.getColumn(col++).getInt();
		signal.tailcode[4] = query.getColumn(col++).getInt();
		signal.tailcode[5] = query.getColumn(col++).getInt();
		signal.tailcode[6] = query.getColumn(col++).getInt();

		signal.original_headflag = query.getColumn(col++).getInt();
		signal.original_headtime = query.getColumn(col++).getInt(); 
#else  //jyc20170228 add
		signal.code = Reversebytes_uint32(query.getColumn(col++).getInt());
		signal.codeValidLen = query.getColumn(col++).getInt();	
		signal.one_high_time = Reversebytes_uint16(query.getColumn(col++).getInt());
		signal.one_low_time =  Reversebytes_uint16(query.getColumn(col++).getInt());
		signal.zero_high_time = Reversebytes_uint16(query.getColumn(col++).getInt());
		signal.zero_low_time =  Reversebytes_uint16(query.getColumn(col++).getInt());

		signal.silent_interval =  Reversebytes_uint16(query.getColumn(col++).getInt());
		signal.signal_count = query.getColumn(col++).getInt();
		signal.headlen = query.getColumn(col++).getInt();
		signal.taillen = query.getColumn(col++).getInt();

		signal.headcode[0] = Reversebytes_uint16(query.getColumn(col++).getInt());
		signal.headcode[1] = Reversebytes_uint16(query.getColumn(col++).getInt());
		signal.headcode[2] = Reversebytes_uint16(query.getColumn(col++).getInt());
		signal.headcode[3] = Reversebytes_uint16(query.getColumn(col++).getInt());
		signal.headcode[4] = Reversebytes_uint16(query.getColumn(col++).getInt());
		signal.headcode[5] = Reversebytes_uint16(query.getColumn(col++).getInt());
		signal.headcode[6] = Reversebytes_uint16(query.getColumn(col++).getInt());

		signal.tailcode[0] = Reversebytes_uint16(query.getColumn(col++).getInt());
		signal.tailcode[1] = Reversebytes_uint16(query.getColumn(col++).getInt());
		signal.tailcode[2] = Reversebytes_uint16(query.getColumn(col++).getInt());
		signal.tailcode[3] = Reversebytes_uint16(query.getColumn(col++).getInt());
		signal.tailcode[4] = Reversebytes_uint16(query.getColumn(col++).getInt());
		signal.tailcode[5] = Reversebytes_uint16(query.getColumn(col++).getInt());
		signal.tailcode[6] = Reversebytes_uint16(query.getColumn(col++).getInt());

		signal.original_headflag = query.getColumn(col++).getInt();        		
		signal.original_headtime = Reversebytes_uint16(query.getColumn(col++).getInt());
#endif
		std::string stroriginal = query.getColumn(col++);
		signal.Set_original( stroriginal ); 

		m_mapRFSignal[signal.id] = signal;
	}

	return true;
}

bool DeviceManager::LoadDB_RS485()
{
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "LoadDB_RS485" );

	SQLite::Statement query(*this->db,"select * from devices_rs485 order by name");
	while(query.executeStep()){
		int id = query.getColumn(0);
		std::string name = query.getColumn(1);
		std::string ver = query.getColumn(2);
		std::string serialno = query.getColumn(3);
		std::string factoryno = query.getColumn(4);
		uint32_t device_id = query.getColumn(5).getInt();
		uint32_t protocol =  query.getColumn(6).getInt();
		uint32_t enable =  query.getColumn(7).getInt();
		
		RS485DevControl *ctl = new RS485DevControl( device_id, 0, protocol, ver );

		std::string sql = "select * from address where mudule_type="+util::int2string(Module_RS485)+" and module_id="+util::int2string(id)+" order by addr";
		SQLite::Statement addr_query(*this->db,sql.c_str());
		while(addr_query.executeStep()){
			uint32_t addr_data = addr_query.getColumn(3).getInt();
			DeviceAddress *addr=new DeviceAddress(addr_data);
			int col = 4;
			if( !addr_query.isColumnNull(col) ) { addr->SetName(addr_query.getColumn(col)); } col++;
			addr->SetDeviceType((IOTDeviceType)addr_query.getColumn(col++).getInt());
			addr->SetReadType((IOTDeviceReadType)addr_query.getColumn(col++).getInt());
			addr->SetDataType((DataType)addr_query.getColumn(col++).getInt());
			addr->SetDefualtValue(addr_query.getColumn(col++));
			addr->SetMinValue(addr_query.getColumn(col++));
			addr->SetMaxValue(addr_query.getColumn(col++));
			addr->set_save_col_val_enable((uint32_t)addr_query.getColumn(col++).getInt());
			ctl->AddAddress(addr);
		}
		GSIOTDevice *iotdev = new GSIOTDevice(id,name,IOT_DEVICE_RS485,ver,serialno,factoryno,ctl);
		iotdev->SetEnable( enable );
		m_devices.push_back(iotdev);
	}

	return true;
}

bool DeviceManager::LoadDB_rf_trigger()
{
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "LoadDB_rf_trigger" );

	SQLite::Statement query(*this->db,"select * from devices_rf_trigger order by name");
	while(query.executeStep()){

		int col = 0;
		int id = query.getColumn(col++);
		std::string name = query.getColumn(col++);
		std::string ver = query.getColumn(col++);
		std::string serialno = query.getColumn(col++);
		std::string factoryno = query.getColumn(col++);
		int enable = query.getColumn(col++).getInt();
		int runstate = query.getColumn(col++).getInt();
		int rfsignal_id = query.getColumn(col++).getInt();

		std::map<int,RFSignal>::const_iterator it = m_mapRFSignal.find(rfsignal_id);
		if( it != m_mapRFSignal.end() )
		{
			GSIOTDevice *dev = new GSIOTDevice(id,name,IOT_DEVICE_Trigger,ver,serialno,factoryno, new TriggerControl(it->second, runstate));
			dev->SetEnable( enable );
			m_devices.push_back(dev);
		}
		else
		{
			//LOGMSGEX( defLOGNAME, defLOG_ERROR, "LoadDB_rf_trigger err, not found rfsignal, id=%d, rfsignal_id=%d\r\n", id, rfsignal_id );
		}
	}

	return true;
}

bool DeviceManager::LoadDB_rf_remote()
{
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "LoadDB_rf_remote" );

	SQLite::Statement query(*this->db,"select * from devices_rf_remote order by serialno,name");
	while(query.executeStep()){

		int col = 0;
		int id = query.getColumn(col++);
		std::string name = query.getColumn(col++);
		std::string ver = query.getColumn(col++);
		std::string serialno = query.getColumn(col++);
		std::string factoryno = query.getColumn(col++);
		int enable = query.getColumn(col++).getInt();
		
		const IOTDeviceType ExType = RFRemoteControl::Save_TransFromExType( serialno );
		RFRemoteControl *ctl = NULL;

#if 1
		switch( ExType )
		{
		case IOTDevice_AC_Ctl:
			ctl = new GSRemoteCtl_AC();
			break;

		case IOTDevice_Combo_Ctl:
			ctl = new GSRemoteCtl_Combo_Ctl();
			break;

		default:
			ctl = new RFRemoteControl();
			break;
		}
#else
		RFRemoteControl *ctl = ver=="ac1.0" ? new GSRemoteCtl_AC():new RFRemoteControl();
#endif

		if( !ctl )
			continue;

		ctl->Save_RefreshFromLoad( serialno );
		LoadDB_remote_button( IOT_DEVICE_Remote, id, (RemoteButtonManager*)ctl, ctl->GetExType() );

		GSIOTDevice *iotdev = new GSIOTDevice(id,name,IOT_DEVICE_Remote,ver,serialno,factoryno,ctl);
		iotdev->SetEnable( enable );
		m_devices.push_back(iotdev);
		
	}

	return true;
}

bool DeviceManager::LoadDB_remote_button( const IOTDeviceType device_type, const int device_id, RemoteButtonManager *pRemoteButtonManager, IOTDeviceType ExType )
{
	if( !pRemoteButtonManager )
	{
		return false;
	}

	//LOGMSGEX( defLOGNAME, defLOG_SYS, "LoadDB_remote_button" );
	const RFSignal baseRFSignal;

	std::string sql = "select * from remote_button where device_type="+util::int2string(device_type)+" and device_id="+util::int2string(device_id);
	SQLite::Statement query(*this->db,sql.c_str());
	while(query.executeStep())
	{
		int col = 0;
		int id = query.getColumn(col++);
		int read_device_type = query.getColumn(col++);
		int read_device_id = query.getColumn(col++);
		std::string name ;
		if( !query.isColumnNull(col) ) { 
			//(std::string)query.getColumn(col);  //jyc20160921
			string tmpname = query.getColumn(col); 
			name = tmpname ;
		} 
		col++;
		
		const int enable_col_save = query.getColumn(col++).getInt();
		int rfsignal_id = query.getColumn(col++).getInt();

		const RFSignal* pSignal = NULL;
		RemoteButton *pButton = NULL;
		if( 0==rfsignal_id )
		{
		}
		else
		{
			std::map<int,RFSignal>::const_iterator it = m_mapRFSignal.find(rfsignal_id);
			if( it != m_mapRFSignal.end() )
			{
				pSignal = &it->second;
			}
			else
			{
				//LOGMSGEX( defLOGNAME, defLOG_ERROR, "LoadDB_remote_button err, not found rfsignal, id=%d, rfsignal_id=%d\r\n", id, rfsignal_id );
			}
		}

		switch( ExType )
		{
		case IOTDevice_AC_Ctl:
			if( pSignal )
			{
				if( defRFSignalType_RemoteObj_ExType==pSignal->signal_type )
				{
					switch( pSignal->code )
					{
					case IOTDevice_AC_Door:
						pButton = new GSRemoteObj_AC_Door( *pSignal );
						break;

					default:
						break;
					}
				}
			}
			break;
			
		case IOTDevice_Combo_Ctl:
		default:
			pButton = new RemoteButton( pSignal ? *pSignal:baseRFSignal );
			break;
		}

		if( pButton )
		{
			pButton->SetId( id );
			pButton->SetName( name );
			pButton->set_enable_col_save( enable_col_save );

			pRemoteButtonManager->AddButton( pButton );
		}
		else
		{
			//LOGMSGEX( defLOGNAME, defLOG_ERROR, "LoadDB_remote_button err, id=%d, rfsignal_id=%d, signal_type=%d, code=%d err\r\n", id, rfsignal_id, pSignal->signal_type, pSignal->code );
		}
	}

	pRemoteButtonManager->SortUseNo();

	return true;
}

bool DeviceManager::LoadDB_Preset( defDBSavePresetQueue &que )
{
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "LoadDB_Preset" );

	SQLite::Statement query(*this->db,"select * from dev_preset order by device_type, device_id, sort_no");
	while(query.executeStep()){

		struDBSavePreset Preset;

		int col = 0;
		Preset.id = query.getColumn(col++);
		Preset.device_type = query.getColumn(col++).getInt();
		Preset.device_id = query.getColumn(col++).getInt();
		Preset.sort_no = query.getColumn(col++).getInt();
		Preset.preset_index = query.getColumn(col++).getInt();
		Preset.enable = query.getColumn(col++).getInt();
		Preset.param1 = query.getColumn(col++).getInt();
		Preset.param2 = query.getColumn(col++).getInt();
		//Preset.name = query.getColumn(col++);

		que.push_back(Preset);
	}

	return true;
}

int DeviceManager::SaveToDB_rfsignal( const RFSignal &signal )
{
	SQLite::Statement   query(*db,"INSERT INTO rfsignal VALUES(NULL,:signal_type,:freq,:code,:codeValidLen,"\
		":one_high_time,:one_low_time,:zero_high_time,:zero_low_time,"\
		":silent_interval,:signal_count,:headlen,:taillen,"\
		":headcode1,:headcode2,:headcode3,:headcode4,:headcode5,:headcode6,:headcode7,"\
		":tailcode1,:tailcode2,:tailcode3,:tailcode4,:tailcode5,:tailcode6,:tailcode7,"\
		":original_headflag,:original_headtime,:original)");

	int col = 1;

	query.bind(col++, (int)signal.signal_type);
	query.bind(col++, (int)signal.freq);
	query.bind(col++, (int)signal.code);
	query.bind(col++, (int)signal.codeValidLen);

	query.bind(col++, (int)signal.one_high_time);
	query.bind(col++, (int)signal.one_low_time);
	query.bind(col++, (int)signal.zero_high_time);
	query.bind(col++, (int)signal.zero_low_time);

	query.bind(col++, (int)signal.silent_interval);
	query.bind(col++, (int)signal.signal_count);
	query.bind(col++, (int)signal.headlen);
	query.bind(col++, (int)signal.taillen);

	query.bind(col++, (int)signal.headcode[0]);
	query.bind(col++, (int)signal.headcode[1]);
	query.bind(col++, (int)signal.headcode[2]);
	query.bind(col++, (int)signal.headcode[3]);
	query.bind(col++, (int)signal.headcode[4]);
	query.bind(col++, (int)signal.headcode[5]);
	query.bind(col++, (int)signal.headcode[6]);

	query.bind(col++, (int)signal.tailcode[0]);
	query.bind(col++, (int)signal.tailcode[1]);
	query.bind(col++, (int)signal.tailcode[2]);
	query.bind(col++, (int)signal.tailcode[3]);
	query.bind(col++, (int)signal.tailcode[4]);
	query.bind(col++, (int)signal.tailcode[5]);
	query.bind(col++, (int)signal.tailcode[6]);

	query.bind(col++, (int)signal.original_headflag);
	query.bind(col++, (int)signal.original_headtime);
	query.bind(col++, signal.Get_original() );

	query.exec();
	return db->execAndGet("SELECT id FROM rfsignal ORDER BY id DESC LIMIT 1");
}

int DeviceManager::SaveToDB_RS485(GSIOTDevice* device)
{
	SQLite::Statement  query(*db,"INSERT INTO devices_rs485 VALUES(NULL,:name,:ver,:serialno,:factoryno,:device_id,:protocol,:enable)");
	query.bind(1, device->getName());
	query.bind(2, device->getVer());
	query.bind(3, c_NullStr);
	query.bind(4, c_NullStr);

	RS485DevControl *ctl = (RS485DevControl*)device->getControl();
	if(!ctl) return 0;
	query.bind(5, (int)ctl->GetDeviceid());
	query.bind(6, (int)ctl->GetProtocol());
	query.bind(7, int(1));
	query.exec();

	int id = db->execAndGet("SELECT id FROM devices_rs485 ORDER BY id DESC LIMIT 1");

	defAddressQueue addrList = ctl->GetAddressList();
	if(addrList.size()>0){
		defAddressQueue::const_iterator ita = addrList.begin();
		for(;ita!=addrList.end();ita++){
			SQLite::Statement  address_query(*db,"INSERT INTO address VALUES(NULL,:mudule_type,:module_id,"\
				":addr,:name,:type,:readtype,:datatype,:defualt_value,:min_value,:max_value,:enable)");
			address_query.bind(1,Module_RS485);
			int col = 2;
			address_query.bind(col++,id);
			address_query.bind(col++,(int)(*ita)->GetAddress());
			address_query.bind(col++,(*ita)->GetName());
			address_query.bind(col++,(int)(*ita)->GetType());
			address_query.bind(col++,(int)(*ita)->GetReadType());
			address_query.bind(col++,(int)(*ita)->GetDataType());
			address_query.bind(col++,(*ita)->GetDefualtValue());
			address_query.bind(col++,(*ita)->GetMinValue());
			address_query.bind(col++,(*ita)->GetMaxValue());
			address_query.bind(col++,(int)(*ita)->get_save_col_val_enable());
			address_query.exec();
		}
	}

	g_Changed( defCfgOprt_Add, device->getType(), id );
	return id;
}

int DeviceManager::SaveToDB_rf_trigger(GSIOTDevice* device)
{
	TriggerControl *ctl = (TriggerControl *)device->getControl();
	if( !ctl )
	{
		return -1;
	}

	int rfsignal_id = SaveToDB_rfsignal( ctl->GetSignal() );
	if( rfsignal_id<=0 )
	{
		return -1;
	}

	ctl->GetSignal().id = rfsignal_id;

	SQLite::Statement   query(*db,"INSERT INTO devices_rf_trigger VALUES(NULL,:name,:ver,:serialno,:factoryno,"\
		":enable,:runstate,:rfsignal_id)");

	int col = 1;
	query.bind(col++, device->getName());
	query.bind(col++, device->getVer());
	query.bind(col++, c_NullStr);
	query.bind(col++, c_NullStr);
	query.bind(col++, (int)device->GetEnable());
	query.bind(col++, (int)ctl->GetAGRunState());
	query.bind(col++, (int)ctl->GetSignal().id);

	query.exec();

	const int newid = db->execAndGet("SELECT id FROM devices_rf_trigger ORDER BY id DESC LIMIT 1");

	g_Changed( defCfgOprt_Add, device->getType(), newid );

	return newid;
}


int DeviceManager::SaveToDB_rf_remote(GSIOTDevice* device)
{
	RFRemoteControl *ctl = (RFRemoteControl *)device->getControl();
	if( !ctl )
	{
		return -1;
	}
	
	SQLite::Statement   query(*db,"INSERT INTO devices_rf_remote VALUES(NULL,:name,:ver,:serialno,:factoryno,"\
		":enable)");

	int col = 1;
	query.bind(col++, device->getName());
	query.bind(col++, device->getVer());
	query.bind(col++, ctl->Save_RefreshToSave());
	query.bind(col++, c_NullStr);
	query.bind(col++, (int)device->GetEnable());

	query.exec();
	int device_id = db->execAndGet("SELECT id FROM devices_rf_remote ORDER BY id DESC LIMIT 1");
	
	const defButtonQueue &que = ctl->GetButtonList();
	defButtonQueue::const_iterator it = que.begin();
	defButtonQueue::const_iterator itEnd = que.end();
	for( ; it!=itEnd; ++it )
	{
		RemoteButton *pCurButton = *it;
		
		pCurButton->SetId( SaveToDB_remote_button( IOT_DEVICE_Remote, device_id, pCurButton ) );
	}

	g_Changed( defCfgOprt_Add, device->getType(), device_id );
	return device_id;
}

int DeviceManager::SaveToDB_remote_button( const IOTDeviceType device_type, const int device_id, RemoteButton *pRemoteButton )
{
	if( !pRemoteButton )
	{
		return -1;
	}

	pRemoteButton->Save_RefreshToSave();

	int rfsignal_id = SaveToDB_rfsignal( pRemoteButton->GetSignal() );
	if( rfsignal_id<=0 )
	{
		return -1;
	}

	pRemoteButton->GetSignal().id = rfsignal_id;

	SQLite::Statement   query(*db,"INSERT INTO remote_button VALUES(NULL,:device_type,:device_id,:name,"\
		":enable,:rfsignal_id)");

	int col = 1;
	query.bind(col++, device_type);
	query.bind(col++, device_id);
	query.bind(col++, pRemoteButton->GetObjName());
	query.bind(col++, (int)pRemoteButton->get_enable_col_save());
	query.bind(col++, (int)pRemoteButton->GetSignal().id);

	query.exec();

	const int newaddrid = db->execAndGet("SELECT id FROM remote_button ORDER BY id DESC LIMIT 1");

	g_Changed( defCfgOprt_Add, device_type, device_id, newaddrid );

	return newaddrid;
}

int DeviceManager::SaveToDB_DeviceAddress( const IOTDeviceType device_type, const int device_id, DeviceAddress *pAddr )
{
	if( !pAddr )
	{
		return -1;
	}

	int mudule_type = 0;
	switch( device_type )
	{
	case IOT_DEVICE_RS485:
		mudule_type = Module_RS485;
		break;

	default:
		return false;
	}

	SQLite::Statement  address_query(*db,"INSERT INTO address VALUES(NULL,:mudule_type,:module_id,"\
		":addr,:name,:type,:readtype,:datatype,:defualt_value,:min_value,:max_value,:enable)");
	address_query.bind(1,device_type);
	int col = 2;
	address_query.bind(col++,device_id);
	address_query.bind(col++,(int)pAddr->GetAddress());
	address_query.bind(col++,pAddr->GetName());
	address_query.bind(col++,(int)pAddr->GetType());
	address_query.bind(col++,(int)pAddr->GetReadType());
	address_query.bind(col++,(int)pAddr->GetDataType());
	address_query.bind(col++,pAddr->GetDefualtValue());
	address_query.bind(col++,pAddr->GetMinValue());
	address_query.bind(col++,pAddr->GetMaxValue());
	address_query.bind(col++,(int)pAddr->get_save_col_val_enable());
	address_query.exec();

	g_Changed( defCfgOprt_Add, device_type, device_id, pAddr->GetAddress() );
	return 1;
}

int DeviceManager::SaveToDB_Preset( const IOTDeviceType device_type, const int device_id, const CPresetObj *pPreset )
{
	if( !pPreset )
	{
		return -1;
	}

	SQLite::Statement   query(*db,"INSERT INTO dev_preset VALUES(NULL,:device_type,:device_id,:sort_no,:preset_index,"
		":enable,:param1,:param2,:name)");

	int col = 1;
	query.bind(col++, device_type);
	query.bind(col++, device_id);
	query.bind(col++, (int)pPreset->GetSortNo());
	query.bind(col++, (int)pPreset->GetIndex());
	query.bind(col++, (int)pPreset->GetEnable());
	query.bind(col++, int(0));
	query.bind(col++, int(0));
	query.bind(col++, pPreset->GetObjName());

	query.exec();

	g_Changed( defCfgOprt_Add, device_type, device_id, defIOTDeviceAddr_Other );

	return db->execAndGet("SELECT id FROM dev_preset ORDER BY id DESC LIMIT 1");
}

bool DeviceManager::SaveSort_RemoteButton( GSIOTDevice *iotdevice )
{
	if( !iotdevice
		|| IOT_DEVICE_Remote != iotdevice->getType()
		|| !iotdevice->getControl() )
	{
		return false;
	}

	RFRemoteControl *pctl = (RFRemoteControl*)iotdevice->getControl();
	if( !pctl )
		return false;

	UseDbTransAction dbta(db);

	char sqlbuf[1024] = {0};
	snprintf( sqlbuf, sizeof(sqlbuf), "update remote_button set enable=(enable&1) WHERE device_type=%d and device_id=%d", iotdevice->getType(), iotdevice->getId() );
	db->exec( sqlbuf );

	const RFRemoteControl *ctl = (RFRemoteControl*)iotdevice->getControl();
	const defButtonQueue& ButtonList = ctl->GetButtonList();
	for( defButtonQueue::const_iterator it=ButtonList.begin(); it!=ButtonList.end(); ++it )
	{
		const RemoteButton *pButton = *it;
		DB_Modify_remote_button( iotdevice->getType(), iotdevice->getId(), pButton );
	}

	return true;
}

// 检测门禁是否已经存在
bool DeviceManager::CheckExist_ACCtl( const std::string &ip, const int selfid )
{
	if( ip.empty() )
		return false;

	if( !m_devices.empty() )
	{
		std::list<GSIOTDevice*>::const_iterator it = m_devices.begin();
		for( ; it!=m_devices.end(); ++it )
		{
			if( (*it)->getExType() != IOTDevice_AC_Ctl )
				continue;

			if( (*it)->getId() == selfid )
				continue;

			GSRemoteCtl_AC *pAC = (GSRemoteCtl_AC*)(*it)->getControl();
			if( !pAC )
				continue;

			if( pAC->get_param_safe().ip == ip )
				return true;
		}
	}

	return false;
}

// 检测门禁是否已经存在
uint32_t DeviceManager::GetDevCount( const IOTDeviceType ExType )
{
	uint32_t count = 0;
	
	if( !m_devices.empty() )
	{
		std::list<GSIOTDevice*>::const_iterator it = m_devices.begin();
		for( ; it!=m_devices.end(); ++it )
		{
			if( (*it)->getExType() != ExType )
				continue;
			
			count++;
		}
	}
	
	return count;
}

