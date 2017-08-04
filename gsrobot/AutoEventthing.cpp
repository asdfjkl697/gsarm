#include "AutoEventthing.h"
#include "gloox/util.h"


AutoEventthing::AutoEventthing(void)
	: m_control_device_type(IOT_DEVICE_Unknown),m_control_device_id(0),m_pTempDevice(NULL),m_runstate(1)
{
	m_isForce = true; // 这种事件默认总是执行
}


AutoEventthing::AutoEventthing(const Tag* tag)
	: ControlEvent(tag), m_control_device_type(IOT_DEVICE_Unknown),m_control_device_id(0),m_pTempDevice(NULL),m_runstate(1)
{
	m_isForce = true; // 这种事件默认总是执行
	if( !tag || tag->name() != "eventthing" )
		return;

	this->untagBase( tag );

	this->UntagEditAttr( tag );

	Tag *tall = tag->findChild("all");
	if( tall )
	{
		if( atoi( tall->cdata().c_str() ) )
		{
			this->SetAllDevice();
		}
	}
	
	if( !IsAllDevice() )
	{
		Tag *tDevice = tag->findChild("device");
		if( tDevice )
		{
			m_pTempDevice = new GSIOTDevice(tDevice);
			this->m_control_device_type = m_pTempDevice->getType();
			this->m_control_device_id = m_pTempDevice->getId();
		}
	}

	Tag *tState = tag->findChild("state");
	if( tState )
	{
		m_runstate = atoi( tState->cdata().c_str() );
	}
}

AutoEventthing::~AutoEventthing(void)
{
	if( m_pTempDevice )
	{
		delete m_pTempDevice;
		m_pTempDevice = NULL;
	}
}

bool AutoEventthing::doEditAttrFromAttrMgr( const EditAttrMgr &attrMgr, defCfgOprt_ oprt )
{
	if( attrMgr.GetEditAttrMap().empty() )
		return false;

	bool doUpdate = this->doEditAttrFromAttrMgr_EvtBase( attrMgr, oprt );
	std::string outAttrValue;

	if( attrMgr.FindEditAttr( "state", outAttrValue ) )
	{
		doUpdate = true;
		this->SetRunState( atoi(outAttrValue.c_str()) );
	}

	if( attrMgr.FindEditAttr( "all", outAttrValue ) )
	{
		doUpdate = true;
		if( atoi( outAttrValue.c_str() ) )
			this->m_control_device_type = IOT_DEVICE_All;
		else
			this->m_control_device_type = IOT_DEVICE_Unknown;
	}

	return doUpdate;
}

bool AutoEventthing::UpdateForDev( const GSIOTDevice *pDevice )
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
	
	return doUpdate;
}

Tag* AutoEventthing::tag(const struTagParam &TagParam) const
{
	Tag* i = new Tag( "eventthing" );

	tagBase( i, TagParam );

	if( TagParam.isValid && TagParam.isResult )
	{
		this->tagEditAttr( i, TagParam );
	}
	else
	{
		if( IsAllDevice() )
		{
			new Tag( i, "all", IsAllDevice() ? std::string("1"):std::string("0") );
		}
		else
		{
			std::string ctrl_devtype_name;
			this->FindEditAttr( "ctrl_devtype_name", ctrl_devtype_name );

			GSIOTDevice Device(NULL);
			Device.setType( this->m_control_device_type );
			Device.setId( this->m_control_device_id );
			Device.setName( ctrl_devtype_name );
			i->addChild( Device.tag(TagParam) );
		}

		new Tag( i, "state", util::int2string(m_runstate) );
	}

	return i;
}

bool AutoEventthing::EventParamIsComplete()
{
	if( IsAllDevice() )
		return true;

	if( IOT_DEVICE_Unknown == this->m_control_device_type )
	{
		return false;
	}

	if(!this->m_control_device_id){
		return false;
	}

	return true;
}
