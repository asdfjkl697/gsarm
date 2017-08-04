#ifndef AUTOCONTROLEVENT_H_
#define AUTOCONTROLEVENT_H_
#include "GSIOTDevice.h"
#include "ControlEvent.h"

using namespace gloox;

class AutoControlEvent:public ControlEvent
{
private:
	IOTDeviceType m_control_device_type;
	uint32_t m_control_device_id;
	uint32_t m_address;
	std::string m_value;
	GSIOTDevice *m_pTempDevice;

public:
	AutoControlEvent(void);
	AutoControlEvent(const Tag* tag);
	~AutoControlEvent(void);
	
	bool doEditAttrFromAttrMgr( const EditAttrMgr &attrMgr, defCfgOprt_ oprt = defCfgOprt_Modify );
	
	bool UpdateForOther( const AutoControlEvent *pother );
	bool UpdateForDev( const GSIOTDevice *pDevice );

	EventType GetType() const
	{
		return CONTROL_Event;
	}

	bool isSameDevice( const GSIOTDevice *pDevice ) const
	{
		if( !pDevice )
			return false;

		return ( pDevice->getType()==m_control_device_type && pDevice->getId()==m_control_device_id );
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

	void SetAddress(const uint32_t addr)
	{
		m_address = addr;
	}
	void SetValue(const std::string& val)
	{
		m_value = val;
	}

	IOTDeviceType GetControlDeviceType() const
	{
		return m_control_device_type;
	}

	uint32_t GetControlDeviceId() const
	{
		return m_control_device_id;
	}

	uint32_t GetAddress() const
	{
		return m_address;
	}
	
	const std::string& GetValue() const
	{
		return m_value;
	}


	bool EventParamIsComplete();

	Tag* tag(const struTagParam &TagParam) const;
	virtual ControlEvent* clone() const{
		return new AutoControlEvent(*this);
	}
};

#endif
