#include "IPCameraManager.h"
#include "GsCamera.h" //jyc20170518 just for define


IPCameraManager::IPCameraManager(void)
	:SQLiteHelper()
{
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "LoadDB devices_ipcamera" );

	SQLite::Statement query(*this->db,"select * from devices_ipcamera order by name");
	while(query.executeStep()){
		int col = 0;
		int id = query.getColumn(col++);
		std::string device_id = query.getColumn(col++);
		uint32_t  module_type = query.getColumn(col++).getInt();
		std::string name = query.getColumn(col++);
		std::string ver = query.getColumn(col++);
		std::string ipaddress = query.getColumn(col++);
		uint32_t port = query.getColumn(col++).getInt();
		std::string username = query.getColumn(col++);
		std::string password = query.getColumn(col++);

		int channel = query.getColumn(col++).getInt();
		int streamfmt = query.getColumn(col++).getInt();

		GSPtzFlag PTZFlag = (GSPtzFlag)query.getColumn(col++).getInt();

		GSFocalFlag FocalFlag = (GSFocalFlag)query.getColumn(col++).getInt();
		uint32_t focalmin = query.getColumn(col++).getInt();
		uint32_t focalmax = query.getColumn(col++).getInt();
		uint32_t focalzoom = query.getColumn(col++).getInt();

		int enable = 0;
		int AGRunState = 0;
		IPCameraBase::uncode_enable_col_save( query.getColumn(col++).getInt(), enable, AGRunState );

		int32_t buffertime = query.getColumn(col++).getInt();

		uint32_t rec_mod = query.getColumn(col++).getInt();
		uint32_t rec_svrtype = query.getColumn(col++).getInt();
		std::string rec_ipaddress = query.getColumn(col++);
		uint32_t rec_port = query.getColumn(col++).getInt();
		std::string rec_username = query.getColumn(col++);
		std::string rec_password = query.getColumn(col++);
		int rec_channel = query.getColumn(col++).getInt();

		// FixParm
		uint32_t FixTSMod = query.getColumn(col++).getInt();
		uint32_t span_Min = query.getColumn(col++).getInt();
		uint32_t span_Max = query.getColumn(col++).getInt();
		uint32_t fix_Min = query.getColumn(col++).getInt();
		uint32_t fix_Max = query.getColumn(col++).getInt();
		uint32_t spanWarn = query.getColumn(col++).getInt();
		uint32_t mfactor_span_Min = query.getColumn(col++).getInt();
		uint32_t mfactor_span_Max = query.getColumn(col++).getInt();
		uint32_t mfactor_fix_Min = query.getColumn(col++).getInt();
		uint32_t mfactor_fix_Max = query.getColumn(col++).getInt();
		uint32_t mfactor_spanWarn = query.getColumn(col++).getInt();

		uint32_t Cam_Info1 = query.getColumn(col++).getInt();
		uint32_t Cam_Info2 = query.getColumn(col++).getInt();

		CAudioCfg AudioCfg;
		AudioCfg.set_Audio_Source( (defAudioSource_)query.getColumn(col++).getInt() );
		AudioCfg.set_Audio_FmtType( (defAudioFmtType_)query.getColumn(col++).getInt() );
		AudioCfg.set_Audio_ParamDef( (defAudioParamDef_)query.getColumn(col++).getInt() );
		AudioCfg.set_Audio_Channels( query.getColumn(col++).getInt() );
		AudioCfg.set_Audio_bitSize( query.getColumn(col++).getInt() );
		AudioCfg.set_Audio_SampleRate( query.getColumn(col++).getInt() );
		AudioCfg.set_Audio_ByteRate( query.getColumn(col++).getInt() );

		if( !query.isColumnNull(col) ) { 
			//AudioCfg.set_Audio_cap_src((std::string)query.getColumn(col)); //jyc20170406 remove
		} col++;
		if( !query.isColumnNull(col) ) { 
			//AudioCfg.set_Audio_cap_param((std::string)query.getColumn(col)); //jyc20170511 remove
		} col++;
		IPCameraBase *camBase = NULL;
		camBase = new GsCamera(device_id,name,ipaddress.c_str(),port,
						username,password,ver,PTZFlag,FocalFlag,
						channel, streamfmt );
		
		/* jyc20170406 remove
		if(module_type == TI368){
			camBase = new TI368Camera(device_id,name,ipaddress.c_str(),port,
				username,password,ver,PTZFlag,FocalFlag,
				channel, streamfmt );
		}else if(module_type == SSD1935){
			camBase = new SSD1935Camera(device_id,name,ipaddress.c_str(),port,
				username,password,ver,PTZFlag,FocalFlag,
				channel, streamfmt );
		}else if(module_type == CameraType_hik){
			camBase = new HikCamera(device_id,name,ipaddress.c_str(),port,
				username,password,ver,PTZFlag,FocalFlag,
				channel, streamfmt );
		}else if(module_type == CameraType_dh){
			camBase = new DHCamera(device_id,name,ipaddress.c_str(),port,
				username,password,ver,PTZFlag,FocalFlag,
				channel, streamfmt );
		}*/ 

		if( buffertime >= 0 )
		{
			camBase->setBufferTime( buffertime ); //jyc20170518 remove
		}

		CRecCfg &RecCfg = camBase->GetRecCfg();
		RecCfg.setrec_mod( (defRecMod)rec_mod );
		RecCfg.setrec_svrtype( (IPCameraType)rec_svrtype );
		RecCfg.setrec_ip( rec_ipaddress );
		RecCfg.setrec_port( rec_port );
		RecCfg.setrec_username( rec_username );
		RecCfg.setrec_password( rec_password );
		RecCfg.setrec_channel( rec_channel );

		camBase->UpdateAudioCfg( AudioCfg );

		struCameraFixParm &FixParm = camBase->GetFixParmObj();

		FixParm.FixTSMod = (defFixTSMod)FixTSMod;
		FixParm.span_Min = span_Min;
		FixParm.span_Max = span_Max;
		FixParm.fix_Min = fix_Min;
		FixParm.fix_Max = fix_Max;
		FixParm.spanWarn = spanWarn;
		FixParm.mfactor_span_Min = mfactor_span_Min;
		FixParm.mfactor_span_Max = mfactor_span_Max;
		FixParm.mfactor_fix_Min = mfactor_fix_Min;
		FixParm.mfactor_fix_Max = mfactor_fix_Max;
		FixParm.mfactor_spanWarn = mfactor_spanWarn;
		FixParm.CheckParam();

		CCamAdvAttr AdvAttr;
		AdvAttr.set_AdvAttr_uintfull( Cam_Info1 );
		camBase->UpdateAdvAttr( AdvAttr );

		GSIOTDevice *dev = new GSIOTDevice(id,camBase->GetName(),IOT_DEVICE_Camera,ver,"","", camBase);
		dev->SetEnable( enable );

		if( dev->getControl() )
		{
			IPCameraBase *ctl = (IPCameraBase*)dev->getControl();
			ctl->SetAGRunState( AGRunState, true );
		}
		
		std::string PrePicChangeCode;
		if( IPCameraBase::LoadPrePicChangeInfo( id, PrePicChangeCode ) )
		{
			camBase->SetPrePicChangeCode( PrePicChangeCode );
		}

		this->m_cameraQueue.push_back(dev);
	}
}


IPCameraManager::~IPCameraManager(void)
{
	if( !m_cameraQueue.empty() )
	{
		for( std::list<GSIOTDevice*>::iterator it=m_cameraQueue.begin(); it!=m_cameraQueue.end(); ++it )
		{
			delete *it;
		}
		m_cameraQueue.clear();
	}
}

GSIOTDevice* IPCameraManager::GetIOTDevice( uint32_t deviceId ) const
{
	if( !m_cameraQueue.empty() )
	{
		std::list<GSIOTDevice*>::const_iterator it = m_cameraQueue.begin();
		for( ;it!=m_cameraQueue.end(); ++it )
		{
			if( (*it)->getId() == deviceId )
			{
				return (*it);
			}
		}
	}

	return NULL;
}

IPCameraBase* IPCameraManager::FindIPCamera( std::string &ip, int port, int channel, int streamfmt )
{
		std::list<GSIOTDevice*>::const_iterator it = m_cameraQueue.begin();
		std::list<GSIOTDevice*>::const_iterator itEnd = m_cameraQueue.end();
		for( ; it!=itEnd; ++it )
		{
			IPCameraBase *curcam = (IPCameraBase*)((*it)->getControl());

			if( curcam->GetIPAddress() == ip
				&& curcam->GetPort() == port
				&& curcam->GetChannel() == channel
				&& curcam->GetStreamfmt() == streamfmt
				)
			{
				return curcam;
			}
		}

	return NULL;
}

int IPCameraManager::AddIPCamera(IPCameraBase *cam)
{
	if(cam)
	{
		std::list<GSIOTDevice*>::const_iterator it = m_cameraQueue.begin();
		std::list<GSIOTDevice*>::const_iterator itEnd = m_cameraQueue.end();
		for( ; it!=itEnd; ++it )
		{
			IPCameraBase *curcam = (IPCameraBase*)((*it)->getControl());
			if( curcam->GetIPAddress() == cam->GetIPAddress()
				&& curcam->GetPort() == cam->GetPort()
				&& curcam->GetChannel() == cam->GetChannel()
				&& curcam->GetStreamfmt() == cam->GetStreamfmt()
				)
			{
				LOGMSG( "%s:%d channel=%d streamfmt=%d is being, add cam failed!\r\n",
					cam->GetIPAddress().c_str(), cam->GetPort(), cam->GetChannel(), cam->GetStreamfmt() );
				
				return -2;
			}
		}

		GSIOTDevice *dev = new GSIOTDevice(0,cam->GetName(),IOT_DEVICE_Camera,"1.0","","", cam);
		int id = this->SaveToDB(dev);
		dev->setId( id );
		this->m_cameraQueue.push_back(dev);
		return 1;
	}

	return -1;
}

bool IPCameraManager::RemoveIPCamera(GSIOTDevice *cam)
{
	if( !cam )
		return false;

	char sqlbuf[256] = {0};

	snprintf( sqlbuf, sizeof(sqlbuf), "DELETE FROM devices_ipcamera WHERE id=%d", cam->getId() );
	db->exec( sqlbuf );

	this->m_cameraQueue.remove( cam );
	
	g_Changed( defCfgOprt_Delete, cam->getType(), cam->getId() );
	return true;
}

bool IPCameraManager::ModifyDevice(GSIOTDevice *cam, uint32_t ModifySubFlag )
{
	if( !cam )
		return false;
	
	if( cam->getName().empty() )
	{
		return false;
	}

	IPCameraBase *camctl = (IPCameraBase*)cam->getControl();

	SQLite::Statement query( *this->db, "UPDATE devices_ipcamera SET name=?,ver=?,ipaddress=?,port=?,username=?,password=?,channel=?,streamfmt=?,"\
		"ptz=?,focal=?,enable=?,buffertime=?,device_id=?,"\
		"rec_mod=?,rec_svrtype=?,rec_ipaddress=?,rec_port=?,rec_username=?,rec_password=?,rec_channel=?,"\
		"Cam_Info1=?,Cam_Info2=?,Audio_Source=?,Audio_FmtType=?,Audio_ParamDef=?,Audio_Channels=?,Audio_bitSize=?,Audio_SampleRate=?,Audio_ByteRate=?,Audio_cap_src=?,Audio_cap_param=? WHERE id=?" );

	int col = 1;
	query.bind(col++, cam->getName().c_str());
	query.bind(col++, cam->getVer().c_str());
	query.bind(col++, camctl->GetIPAddress().c_str());
	query.bind(col++, (int)camctl->GetPort());
	query.bind(col++, camctl->GetUsername().c_str());
	query.bind(col++, camctl->GetPassword().c_str());
	query.bind(col++, (int)camctl->GetChannel());
	query.bind(col++, (int)camctl->GetStreamfmt());

	query.bind(col++, camctl->getPTZFlag());
	query.bind(col++, camctl->getFocalFlag());
	query.bind(col++, IPCameraBase::encode_enable_col_save( (int)cam->GetEnable(), (int)camctl->GetAGRunState() ));
	query.bind(col++, (int)camctl->getBufferTime());
	query.bind(col++, camctl->GetDeviceId().c_str());

	CRecCfg &RecCfg = camctl->GetRecCfg();
	query.bind(col++, (int)RecCfg.getrec_mod());
	query.bind(col++, (int)RecCfg.getrec_svrtype());
	query.bind(col++, RecCfg.getrec_ip());
	query.bind(col++, (int)RecCfg.getrec_port());
	query.bind(col++, RecCfg.getrec_username());
	query.bind(col++, RecCfg.getrec_password());
	query.bind(col++, (int)RecCfg.getrec_channel());

	query.bind( col++, int(camctl->GetAdvAttr().get_AdvAttr_uintfull()) ); // Cam_Info1
	query.bind( col++, int(0) ); // Cam_Info2

	CAudioCfg &AudioCfg = camctl->GetAudioCfg();
	query.bind(col++, (int)AudioCfg.get_Audio_Source());
	query.bind(col++, (int)AudioCfg.get_Audio_FmtType());
	query.bind(col++, (int)AudioCfg.get_Audio_ParamDef());
	query.bind(col++, (int)AudioCfg.get_Audio_Channels());
	query.bind(col++, (int)AudioCfg.get_Audio_bitSize());
	query.bind(col++, (int)AudioCfg.get_Audio_SampleRate());
	query.bind(col++, (int)AudioCfg.get_Audio_ByteRate());
	query.bind(col++, AudioCfg.get_Audio_cap_src());
	query.bind(col++, AudioCfg.get_Audio_cap_param());

	query.bind(col++, cam->getId());
	query.exec();

	g_Changed( defCfgOprt_Modify, cam->getType(), cam->getId() );
	return true;
}

int IPCameraManager::SaveToDB(GSIOTDevice *dev)
{
	IPCameraBase *cam = (IPCameraBase*)dev->getControl();

	if( !cam )
		return 0;

	SQLite::Statement query(*this->db,"INSERT INTO devices_ipcamera VALUES(NULL,:device_id,:module_type,"\
              ":name,:ver,:ipaddress,:port,:username,:password,:channel,:streamfmt,"\
			  ":ptz,:focal,:focal_min,:focal_max,:focal_zoom,:enable,:buffertime,"\
			  ":rec_mod,:rec_svrtype,:rec_ipaddress,:rec_port,:rec_username,:rec_password,:rec_channel,"\
			  ":FixTSMod,:span_Min,:span_Max,:fix_Min,:fix_Max,:spanWarn,"\
			  ":mfactor_span_Min,:mfactor_span_Max,:mfactor_fix_Min,:mfactor_fix_Max,:mfactor_spanWarn,"\
			  ":Cam_Info1,:Cam_Info2,:Audio_Source,:Audio_FmtType,:Audio_ParamDef,:Audio_Channels,:Audio_bitSize,:Audio_SampleRate,:Audio_ByteRate,:Audio_cap_src,:Audio_cap_param)");

	int col = 1;
	query.bind(col++, cam->GetDeviceId());
	query.bind(col++, cam->GetCameraType());
	query.bind(col++, cam->GetName());
	query.bind(col++, cam->getVer());
	query.bind(col++, cam->GetIPAddress());
	query.bind(col++, (int)cam->GetPort());
	query.bind(col++, cam->GetUsername());
	query.bind(col++, cam->GetPassword());
	query.bind( col++, (int)cam->GetChannel() );
	query.bind( col++, (int)cam->GetStreamfmt() );

	query.bind(col++, (int)cam->getPTZFlag());
	query.bind(col++, (int)cam->getFocalFlag());
	const CameraFocal *focal = cam->getFocal();
	
	query.bind(col++, focal?(int)focal->GetMin():0);
	query.bind(col++, focal?(int)focal->GetMax():0);
	query.bind(col++, focal?(int)focal->GetZoom():0);

	query.bind( col++, IPCameraBase::encode_enable_col_save( (int)dev->GetEnable(), (int)cam->GetAGRunState() ) );
	query.bind( col++, (int)cam->getBufferTime() );

	CRecCfg &RecCfg = cam->GetRecCfg();
	query.bind(col++, (int)RecCfg.getrec_mod());
	query.bind(col++, (int)RecCfg.getrec_svrtype());
	query.bind(col++, RecCfg.getrec_ip());
	query.bind(col++, (int)RecCfg.getrec_port());
	query.bind(col++, RecCfg.getrec_username());
	query.bind(col++, RecCfg.getrec_password());
	query.bind(col++, (int)RecCfg.getrec_channel());

	// FixParm
	struCameraFixParm &FixParm = cam->GetFixParmObj();
	query.bind( col++, (int)FixParm.FixTSMod );

	query.bind( col++, (int)FixParm.span_Min );
	query.bind( col++, (int)FixParm.span_Max );
	query.bind( col++, (int)FixParm.fix_Min );
	query.bind( col++, (int)FixParm.fix_Max );
	query.bind( col++, (int)FixParm.spanWarn );

	query.bind( col++, (int)FixParm.mfactor_span_Min );
	query.bind( col++, (int)FixParm.mfactor_span_Max );
	query.bind( col++, (int)FixParm.mfactor_fix_Min );
	query.bind( col++, (int)FixParm.mfactor_fix_Max );
	query.bind( col++, (int)FixParm.mfactor_spanWarn );

	query.bind( col++, int(cam->GetAdvAttr().get_AdvAttr_uintfull()) ); // Cam_Info1
	query.bind( col++, int(0) ); // Cam_Info2

	CAudioCfg &AudioCfg = cam->GetAudioCfg();
	query.bind(col++, (int)AudioCfg.get_Audio_Source());
	query.bind(col++, (int)AudioCfg.get_Audio_FmtType());
	query.bind(col++, (int)AudioCfg.get_Audio_ParamDef());
	query.bind(col++, (int)AudioCfg.get_Audio_Channels());
	query.bind(col++, (int)AudioCfg.get_Audio_bitSize());
	query.bind(col++, (int)AudioCfg.get_Audio_SampleRate());
	query.bind(col++, (int)AudioCfg.get_Audio_ByteRate());
	query.bind(col++, AudioCfg.get_Audio_cap_src());
	query.bind(col++, AudioCfg.get_Audio_cap_param());

	query.exec();	
	const int newid = db->execAndGet("SELECT id FROM devices_ipcamera ORDER BY id DESC LIMIT 1");

	g_Changed( defCfgOprt_Add, dev->getType(), newid );

	return newid;
}

void IPCameraManager::SaveToFile()
{
}

void IPCameraManager::InitAllCamera_Preset( const defDBSavePresetQueue &que )
{
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "InitAllCamera_Preset" );

	for( defDBSavePresetQueue::const_iterator itP=que.begin(); itP!=que.end(); ++itP )
	{
		const struDBSavePreset &Preset = *itP;

		for( std::list<GSIOTDevice*>::const_iterator itCam = m_cameraQueue.begin(); itCam!=m_cameraQueue.end(); ++itCam )
		{
			if( (*itCam)->getType()==Preset.device_type && (*itCam)->getId()==Preset.device_id )
			{
				IPCameraBase *curcam = (IPCameraBase*)((*itCam)->getControl());

				CPresetObj *pPreset = new CPresetObj( Preset );

				curcam->AddPreset( pPreset );
				break;
			}
		}
	}
}
