#include "AutoControlEvent.h"
#include "gloox/util.h"
#include "gsiot/GSIOTDevice.h"
#include "RS485DevControl.h"
#include "RFRemoteControl.h"

AutoControlEvent::AutoControlEvent(void)
	:m_control_device_type(IOT_DEVICE_Unknown),m_control_device_id(0),m_address(0),m_pTempDevice(NULL)
{
}

AutoControlEvent::AutoControlEvent(const Tag* tag)
	:ControlEvent(tag),m_control_device_type(IOT_DEVICE_Unknown),m_control_device_id(0),m_address(0),m_pTempDevice(NULL)
{
	if( !tag || tag->name() != "devicething" )
		return;

	this->untagBase( tag );

	this->UntagEditAttr( tag );

	Tag *tDevice = tag->findChild("device");
	if( tDevice )
	{
		m_pTempDevice = new GSIOTDevice(tDevice);
		this->UpdateForDev( m_pTempDevice );
	}

	if( tag->hasAttribute( "ctl_devtype" ) )
		this->m_control_device_type = (IOTDeviceType)atoi( tag->findAttribute( "ctl_devtype" ).c_str() );

	if( tag->hasAttribute( "ctl_devid" ) )
		this->m_control_device_id = atoi( tag->findAttribute( "ctl_devid" ).c_str() );

	if( tag->hasAttribute( "ctl_addr" ) )
		this->m_address = atoi( tag->findAttribute( "ctl_addr" ).c_str() );

	if( tag->hasAttribute( "ctl_value" ) )
		this->m_value = tag->findAttribute( "ctl_value" ).c_str();
}

AutoControlEvent::~AutoControlEvent(void)
{
	if( m_pTempDevice )
	{
		delete m_pTempDevice;
		m_pTempDevice = NULL;
	}
}

bool AutoControlEvent::doEditAttrFromAttrMgr( const EditAttrMgr &attrMgr, defCfgOprt_ oprt )
{
	if( attrMgr.GetEditAttrMap().empty() )
		return false;

	bool doUpdate = this->doEditAttrFromAttrMgr_EvtBase( attrMgr, oprt );
	std::string outAttrValue;

	return doUpdate;
}

bool AutoControlEvent::UpdateForOther( const AutoControlEvent *pother )
{
	if( !pother )
		return false;

	if( IOT_DEVICE_Unknown == pother->m_control_device_type || 0==pother->m_control_device_id )
		return false;

	bool doUpdate = false;
	doUpdate |= this->UpdateForDev( pother->GetTempDevice() );

	if( pother->m_control_device_type != this->m_control_device_type )
	{
		doUpdate = true;
		this->m_control_device_type = pother->m_control_device_type;
	}

	if( pother->m_control_device_id != this->m_control_device_id )
	{
		doUpdate = true;
		this->m_control_device_id = pother->m_control_device_id;
	}

	if( pother->m_address != this->m_address )
	{
		doUpdate = true;
		this->m_address = pother->m_address;
	}

	if( pother->m_value != this->m_value )
	{
		doUpdate = true;
		this->m_value = pother->m_value;
	}

	return doUpdate;
}

bool AutoControlEvent::UpdateForDev( const GSIOTDevice *pDevice )
{
	if( !pDevice )
		return false;

	if( IOT_DEVICE_Unknown == pDevice->getType() || 0==pDevice->getId() )
		return false;

	bool doUpdate = false;

	if( pDevice->getType() != this->m_control_device_type )
	{
		doUpdate = true;
		this->m_control_device_type = pDevice->getType();
	}

	if( pDevice->getId() != this->m_control_device_id )
	{
		doUpdate = true;
		this->m_control_device_id = pDevice->getId();
	}

	switch(pDevice->getType())
	{
	case IOT_DEVICE_RS485:
		{
			RS485DevControl *ctl = (RS485DevControl*)pDevice->getControl();

			DeviceAddress *pAddr = ctl->GetFristAddress();
			if( pAddr )
			{
				if( pAddr->GetAddress() != this->m_address )
				{
					doUpdate = true;
					this->m_address = pAddr->GetAddress();
				}

				if( pAddr->GetCurValue() != this->m_value )
				{
					doUpdate = true;
					this->m_value = pAddr->GetCurValue();
				}
			}
		}
		break;

	case IOT_DEVICE_Remote:
		{
			RFRemoteControl *ctl = (RFRemoteControl*)pDevice->getControl();

			RemoteButton *pButton = ctl->GetFristButton();
			if( pButton )
			{
				if( pButton->GetId() != this->m_address )
				{
					doUpdate = true;
					this->m_address = pButton->GetId();
				}
			}
		}
		break;
	}

	return doUpdate;
}

Tag* AutoControlEvent::tag(const struTagParam &TagParam) const
{
	Tag* i = new Tag( "devicething" );

	tagBase( i, TagParam );

	if( TagParam.isValid && TagParam.isResult )
	{
		this->tagEditAttr( i, TagParam );
	}
	else
	{
		std::string ctrl_devtype_name;
		std::string address_name;
		this->FindEditAttr( "ctrl_devtype_name", ctrl_devtype_name );
		this->FindEditAttr( "address_name", address_name );

		switch( m_control_device_type )
		{
		case IOT_DEVICE_RS485:
		case IOT_DEVICE_Remote:
			{
				GSIOTDevice Device( NULL );
				Device.setType( this->m_control_device_type );
				Device.setId( this->m_control_device_id );
				Device.setName( ctrl_devtype_name );
				Device.AddAddressObj( m_address, address_name, m_value );
				i->addChild( Device.tag( TagParam ) );
			}
			break;

		default:
			break;
		}

		i->addAttribute( "ctl_devtype", (int)m_control_device_type );
		i->addAttribute( "ctl_devid", (int)m_control_device_id );
		i->addAttribute( "ctl_devname", ASCIIToUTF8(ctrl_devtype_name) );
		i->addAttribute( "ctl_addr", (int)m_address );
		i->addAttribute( "ctl_addrname", ASCIIToUTF8(address_name) );
		i->addAttribute( "ctl_value", m_value );
	}

	return i;
}

bool AutoControlEvent::EventParamIsComplete()
{
	if( IOT_DEVICE_Unknown == this->m_control_device_type )
	{
		return false;
	}

	if(!this->m_control_device_id){
	   return false;
	}
	if(!m_address){
	   return false;
	}
	if(m_value.length()==0){
	   return false;
	}
	return true;
}
