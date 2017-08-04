#pragma once

#include "gsiot/GSIOTDevice.h"
#include "ControlEvent.h"

using namespace gloox;

class AutoEventthing
	: public ControlEvent
{
public:
	AutoEventthing(void);
	AutoEventthing(const Tag* tag);
	~AutoEventthing(void);

	bool doEditAttrFromAttrMgr( const EditAttrMgr &attrMgr, defCfgOprt_ oprt = defCfgOprt_Modify );

	bool UpdateForDev( const GSIOTDevice *pDevice );

	EventType GetType() const
	{
		return Eventthing_Event;
	}

	bool isSameDevice( const GSIOTDevice *pDevice ) const
	{
		if( !pDevice )
			return false;

		return ( pDevice->getType()==m_control_device_type && pDevice->getId()==m_control_device_id );
	}

	bool IsAllDevice() const
	{
		return ( IOT_DEVICE_All == this->m_control_device_type );
	}
	
	void SetAllDevice()
	{
		this->m_control_device_type = IOT_DEVICE_All;
		this->m_control_device_id = 0;
	}

	GSIOTDevice* GetTempDevice() const
	{
		return m_pTempDevice;
	}

	void SetControlDeviceType( const IOTDeviceType device_type )
	{
		this->m_control_device_type = device_type;
	}

	void SetControlDeviceId(const uint32_t device_id)
	{
		this->m_control_device_id = device_id;
	}

	IOTDeviceType GetControlDeviceType() const
	{
		return m_control_device_type;
	}

	uint32_t GetControlDeviceId() const
	{
		return m_control_device_id;
	}

	uint32_t GetRunState() const
	{
		return m_runstate;
	}

	void SetRunState( uint32_t RunState )
	{
		m_runstate = RunState;
	}

	bool EventParamIsComplete();

	Tag* tag(const struTagParam &TagParam) const;
	virtual ControlEvent* clone() const{
		return new AutoEventthing(*this);
	}

private:
	IOTDeviceType m_control_device_type;
	uint32_t m_control_device_id;
	//uint32_t m_address;
	GSIOTDevice *m_pTempDevice;
	uint32_t m_runstate;
};

