#include "GSIOTDevice.h"
#include "gloox/tag.h"
//#include "MediaControl.h"
//#include "IPCameraBase.h"
#include "RFDeviceControl.h"
//#include "CANDeviceControl.h"
#include "RS485DevControl.h"
#include "RFRemoteControl.h"
#include "common.h"

#include <stdlib.h>

bool operator< ( const GSIOTDeviceKey &key1, const GSIOTDeviceKey &key2 )
{
	if( key1.m_type != key2.m_type )
		return (key1.m_type < key2.m_type);

	return (key1.m_id < key2.m_id);
}

GSIOTDevice::GSIOTDevice(int id,const std::string& name,
	IOTDeviceType type,
	const std::string& ver,
	const std::string& serialno,
	const std::string& factoryno,ControlBase *control)
	: m_id(id),m_name(name),
	m_type(type),m_ver(ver),m_control(control), m_enable(defDeviceEnable)
{
	if( this->m_control )
		this->m_control->SetName( name );
}

GSIOTDevice::GSIOTDevice( const Tag* tag)
	:m_id(0), m_type(IOT_DEVICE_Unknown), m_control(NULL), m_enable(defDeviceEnable)
{
	if( !tag || tag->name() != "device" )
      return;
	
	if(tag->hasAttribute("id"))
		this->m_id = atoi(tag->findAttribute("id").c_str());

	if(tag->hasAttribute("name"))
		this->m_name = UTF8ToASCII(tag->findAttribute("name"));

	if(tag->hasAttribute("type"))
		this->m_type= (IOTDeviceType)atoi(tag->findAttribute("type").c_str());

	if(tag->hasAttribute("ver"))
		this->m_ver= tag->findAttribute("ver");

	this->UntagEditAttr( tag );
	/*/jyc20160527
	Tag *t = tag->findChild(defDeviceTypeTag_media);
	if(t){
	    	this->m_control = new MediaControl(t); //jyc20160527
		return;
	}

	t = tag->findChild(defDeviceTypeTag_camera);
	if(t){
	    	this->m_control = new CameraControl(t);
		return;
	}

	t = tag->findChild(defDeviceTypeTag_candevice);
	if(t){
	    	this->m_control = new CANDeviceControl(t);
		return;
	}*/

	Tag *t = tag->findChild(defDeviceTypeTag_rfdevice);
	if(t){
	    	this->m_control = new RFDeviceControl(t);
		return;
	}

	t = tag->findChild(defDeviceTypeTag_rs485device);
	if(t){
	    	this->m_control = new RS485DevControl(t);
		return;
	}

	t = tag->findChild(defDeviceTypeTag_rfremote);
	if(t){
		this->m_control = new RFRemoteControl(t);
		return;
	}

	if( m_control )
	{
		if(  this->m_type != m_control->GetType() )
		{
			printf( "GSIOTDevice() error, dev_type=%d, ctl_type=%d!\r\n", m_type, m_control->GetType() );
			delete m_control;
			m_control = NULL;
		}
	}
	else
	{
		printf( "\n GSIOTDevice() ctl null, dev_type=%d, ctl_type unknown!\r\n", m_type );
	}

	if( this->m_control )
		this->m_control->SetName( this->m_name );
}

GSIOTDevice::~GSIOTDevice(void)
{
	if(m_control){
	   delete(m_control);
	   m_control = NULL;
	}
}

bool GSIOTDevice::IsCanRelation( IOTDeviceType type )
{
	switch( type )
	{
	case IOT_DEVICE_Switch:
	case IOT_DEVICE_Temperature:
	case IOT_DEVICE_Humidity:
	case IOT_DEVICE_Remote:
	case IOT_DEVICE_RS485:
	case IOT_DEVICE_Wind:
		return true;

	default:
		break;
	}

	return false;
}

bool GSIOTDevice::IsHasSignalDevice( IOTDeviceType type )
{
	switch( type )
	{
	case IOT_DEVICE_Remote:
	case IOT_DEVICE_Trigger:
		return true;

	default:
		break;
	}

	return false;
}

bool GSIOTDevice::IsSupportAlarm( const GSIOTDevice *device )
{
	if( !device )
		return false;

	if( !device->getControl() )
		return false;

	switch( device->getType() )
	{
	case IOT_DEVICE_Trigger:
		return true;
	case IOT_DEVICE_Camera:

		break;

	default:
		break;
	}

	return false;
}

bool GSIOTDevice::doEditAttrFromAttrMgr( const EditAttrMgr &attrMgr, defCfgOprt_ oprt )
{
	if( attrMgr.GetEditAttrMap().empty() )
		return false;

	bool doUpdate = false;
	std::string outAttrValue;

	if( attrMgr.FindEditAttr( "name", outAttrValue ) && !outAttrValue.empty() )
	{
		doUpdate = true;
		this->setName( outAttrValue );
	}

	if( attrMgr.FindEditAttr( "enable", outAttrValue ) )
	{
		doUpdate = true;
		this->SetEnable( atoi(outAttrValue.c_str()) );
	}

	return doUpdate;
}

bool GSIOTDevice::doEditAttrFromAttrMgr_All()
{
	bool doUpdate = this->doEditAttrFromAttrMgr( *this );

	if( !this->m_control )
		return doUpdate;

	switch(m_type)
	{
	case IOT_DEVICE_RS485:
		{
			RS485DevControl *ctl = (RS485DevControl*)this->m_control;

			doUpdate |= ctl->doEditAttrFromAttrMgr( *ctl );

			// 遍历发来的列表
			const defAddressQueue &AddrQue = ctl->GetAddressList();
			defAddressQueue::const_iterator itAddrQue = AddrQue.begin();
			for( ; itAddrQue!=AddrQue.end(); ++itAddrQue )
			{
				DeviceAddress *pSrcAddr = *itAddrQue;

				doUpdate |= pSrcAddr->doEditAttrFromAttrMgr( *pSrcAddr );
			}
		}
		break;

	case IOT_DEVICE_Remote:
		{
			RFRemoteControl *ctl = (RFRemoteControl*)this->m_control;

			// get list
			const defButtonQueue &que = ctl->GetButtonList();
			defButtonQueue::const_iterator it = que.begin();
			defButtonQueue::const_iterator itEnd = que.end();
			for( ; it!=itEnd; ++it )
			{
				RemoteButton *pSrcButton = *it;

				doUpdate |= pSrcButton->doEditAttrFromAttrMgr( *pSrcButton );
			}

		}
		break;
	}

	return doUpdate;
}

Tag* GSIOTDevice::tag(const struTagParam &TagParam) const
{
	Tag* i = new Tag( "device" );
	i->addAttribute("type",this->m_type);
	if( this->m_type!=this->getExType() && IOT_DEVICE_Unknown!=this->getExType() ){ i->addAttribute( "extype", this->getExType() ); }
	i->addAttribute("id",this->m_id);

	if( TagParam.isValid && TagParam.isResult )
	{
		this->tagEditAttr( i, TagParam );
	}
	else
	{
		i->addAttribute("name",ASCIIToUTF8(this->m_name));
		if( !this->m_ver.empty() && this->m_ver!="1.0" ) i->addAttribute("ver",this->m_ver);
		i->addAttribute("readtype",this->getReadType());

		const defUseable useable = this->get_all_useable_state();
		if( defUseable_OK != useable ) { i->addAttribute( "useable", useable ); }; // 默认可用，不可用时才提供此值

		const defAlarmState AlarmState = this->GetCurAlarmState();
		if( macAlarmState_IsAlarm(AlarmState) ) { i->addAttribute( "almst", AlarmState ); }; // 默认正常

		const std::string PrePicChangeCode = this->GetPrePicChangeCode();
		if( !PrePicChangeCode.empty() ) { i->addAttribute( "prepic", PrePicChangeCode ); };
	}

	if(m_control){
		i->addChild(m_control->tag(TagParam));
	}

	return i;
}

GSIOTDevice* GSIOTDevice::clone( bool CreateLock ) const
{
	GSIOTDevice *dev = new GSIOTDevice( *this );
	if(m_control){
		dev->setControl(m_control->clone( CreateLock ));
	}
    return dev;
}

std::string GSIOTDevice::GetStrAlmBody( const bool isAlarm, const struGSTime &dt, const std::string alarmstr, const std::string suffix ) const
{
	std::string almstrf;

	if( IsRUNCODEEnable(defCodeIndex_SYS_AutoNotiContAddIOTName) )
	{
		almstrf += RUNCODE_GetStr(defCodeIndex_SYS_IOT_NAME);
		almstrf += ":";
	}
	
	almstrf += this->getName();
	if( !alarmstr.empty() )
	{
		almstrf += ":";
		almstrf += alarmstr;
	}

	if( !suffix.empty() )
	{
		almstrf += suffix; //isAlarm?"报警":"恢复正常";
	}
	else if( IOT_DEVICE_Trigger==this->getType() )
	{
		almstrf += "被触发";
	}

	if( IsRUNCODEEnable(defCodeIndex_SYS_AutoNotiContAddTime) )
	{
		if( 0!=dt.Year )
		{
			char buf[32] = {0};
			snprintf( buf, sizeof(buf), " (%02d-%02d %02d:%02d)", dt.Month, dt.Day, dt.Hour, dt.Minute );
			almstrf += buf;
		}
	}

	return almstrf;
}

std::string GSIOTDevice::GetStrAlmSubject( const bool isAlarm ) const
{
	return ( isAlarm?std::string("注意"):std::string("恢复正常") );
}


bool GSIOTDevice::ResetUpdateState( const uint32_t address )
{
	switch(m_type)
	{
	case IOT_DEVICE_RS485:
		{
			if( this->m_control )
			{
				DeviceAddress *curaddr = ((RS485DevControl*)this->m_control)->GetAddress(address);

				if( curaddr )
				{
					curaddr->ResetUpdateState();
				}
				else
				{
					return false;
				}
			}
		}
		break;

	default:
		return false;
		break;
	}

	return true;
}

bool GSIOTDevice::SetCurValue( DeviceAddress *addr )
{
	switch(m_type)
	{
	case IOT_DEVICE_RS485:
		{
			if( this->m_control )
			{
				DeviceAddress *curaddr = ((RS485DevControl*)this->m_control)->GetAddress(addr->GetAddress());

				if( curaddr )
				{
					curaddr->SetCurValue( addr->GetCurValue() );
				}
				else
				{
					return false;
				}
			}
		}
		break;

	default:
		return false;
		break;
	}

	return true;
}

bool GSIOTDevice::AddAddressObj( const uint32_t address, const std::string &address_name, const std::string &value )
{
	switch(m_type)
	{
	case IOT_DEVICE_RS485:
		{
			DeviceAddress *pnew = new DeviceAddress((Tag*)NULL);
			pnew->SetAddress( address );
			pnew->SetName( address_name );
			pnew->SetCurValue( value );
			
			return this->AddAddress( pnew );
		}
		break;

	case IOT_DEVICE_Remote:
		{
			RemoteButton *pnew = new RemoteButton(NULL);
			pnew->SetId( address );
			pnew->SetName( address_name );

			return this->AddButton( pnew );
		}
		break;

	default:
		break;
	}

	return false;
}

bool GSIOTDevice::AddAddress( DeviceAddress *addr )
{
	if( !addr )
		return false;

	switch(m_type)
	{
	case IOT_DEVICE_RS485:
		{
			if( !this->m_control )
			{
				this->m_control = new RS485DevControl(NULL);
			}

			if( this->m_control )
			{
				((RS485DevControl*)this->m_control)->AddAddress( addr );
				return true;
			}
		}
		break;

	default:
		break;
	}

	delete addr;
	return false;
}

bool GSIOTDevice::AddButton( RemoteButton *btn )
{
	if( !btn )
		return false;

	switch(m_type)
	{
	case IOT_DEVICE_Remote:
		{
			if( !this->m_control )
			{
				this->m_control = new RFRemoteControl(NULL);
			}

			if( this->m_control )
			{
				((RFRemoteControl*)this->m_control)->AddButton( btn );
				return true;
			}
		}
		break;

	default:
		break;
	}

	delete btn;
	return false;
}

bool GSIOTDevice::hasChild() const
{
	if( !this->m_control )
		return false;

	switch( this->m_type )
	{
	case IOT_DEVICE_RS485:
		{
			const RS485DevControl *ctl = (RS485DevControl*)this->m_control;
			if( !ctl->GetAddressList().empty() )
			{
				return true;
			}
		}
		break;

	case IOT_DEVICE_Remote:
		{
		{
			const RFRemoteControl *ctl = (RFRemoteControl*)this->m_control;
			if( !ctl->GetButtonList().empty() )
			{
				return true;
			}
		}
		}
		break;

	default:
		break;
	}

	return false;
}

IOTDeviceReadType GSIOTDevice::getStaticReadType( const IOTDeviceType type )
{
	switch( type )
	{
		case IOT_DEVICE_CO2:// 温度
		case IOT_DEVICE_HCHO:	// 湿度
		//case IOT_DEVICE_PM25:
		case IOT_DEVICE_Temperature:// 温度
		case IOT_DEVICE_Humidity:	// 湿度
		case IOT_DEVICE_Wind:		// 风速
		case IOT_DEVICE_Trigger:	// 触发器
		case IOT_DEVICE_Event:		// 驱动事件
		case IOT_DEVICE_RFDevice:	// 无线设备
			return IOT_READ;

		case IOT_DEVICE_Remote:		// 遥控
			return IOT_WRITE;

		case IOT_DEVICE_Switch:		// 开关
		case IOT_DEVICE_CANDevice:	// CAN设备
		case IOT_DEVICE_Camera:
			return IOT_READWRITE;

		case IOT_DEVICE_RS485:
			break;

		default:
			break;
	}

	return IOT_Unknow;
}

IOTDeviceReadType GSIOTDevice::getReadType() const
{
	switch( this->getType() )
	{
	case IOT_DEVICE_RS485:
		{
			RS485DevControl *ctl = (RS485DevControl*)this->getControl();
			if( ctl )
			{
				DeviceAddress *pAddr = ctl->GetFristAddress();
				if( pAddr )
				{
					return getStaticReadType( pAddr->GetType() );
				}
			}
		}
		break;

	default:
		break;
	}

	return getStaticReadType( this->getType() );
}

defUseable GSIOTDevice::get_all_useable_state() const
{
	if( !this->GetEnable() )
	{
		return defUseable_OK;
	}

	switch( this->getType() )
	{
	case IOT_DEVICE_Camera:
		{

		}
		break;

	case IOT_DEVICE_Remote:
		{
			RFRemoteControl *ctl = (RFRemoteControl*)this->getControl();
			if( ctl )
			{
				const defUseable NetUseable = ctl->get_NetUseable();

				if( defUseable_OK != NetUseable )
					return NetUseable;
			}
		}
		break;

	case IOT_DEVICE_RS485:
		{
			RS485DevControl *ctl = (RS485DevControl*)this->getControl();
			if( ctl )
			{
				const defUseable NetUseable = ctl->get_NetUseable();

				if( defUseable_OK != NetUseable )
					return NetUseable;
			}
		}
		break;
	}

	IGSClientExFunc *client = g_SYS_GetGSIOTClient();
	if( client )
	{
		return client->get_all_useable_state_ForLinkID( this->GetLinkID() );
	}

	return defUseable_OK;
}

defAlarmState GSIOTDevice::GetCurAlarmState() const
{
	if( !this->getControl() )
	{
		return defAlarmState_UnInit;
	}

	switch( this->getType() )
	{
	case IOT_DEVICE_Trigger:
		return defAlarmState_UnInit;

	case IOT_DEVICE_Camera:
		{

		}
		break;

	default:
		break;
	}

	return defAlarmState_UnInit;
}

// 是设备自身，并且包含指定地址
bool GSIOTDevice::isDevSelfAndIncludeAddr( const IOTDeviceType deviceType, const uint32_t deviceId, const uint32_t address ) const
{
	if( deviceType != this->m_type || deviceId != this->m_id )
	{
		return false;
	}

	if( !this->m_control )
		return false;

	switch(this->m_type)
	{
	case IOT_DEVICE_RS485:
		{
			RS485DevControl *ctl = (RS485DevControl*)this->m_control;

			if( 0==address )
			{
				return ctl->GetFristAddress(); 
			}

			return ctl->GetAddress(address); 
		}
		break;

	case IOT_DEVICE_Remote:
		{
			RFRemoteControl *ctl = (RFRemoteControl*)this->m_control;

			if( 0==address )
			{
				return ctl->GetFristButton();
			}

			return ctl->GetButton(address);
		}
		break;
	}

	return false;
}

std::string GSIOTDevice::GetPrePicChangeCode() const
{
	if( !this->GetEnable() )
	{
		return c_NullStr;
	}

	switch( this->getType() )
	{
	case IOT_DEVICE_Camera:
	{

	}
	break;
	}

	return c_NullStr;
}

