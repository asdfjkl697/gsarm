#ifndef RFDEVICECONTROL_H_
#define RFDEVICECONTROL_H_

#include "typedef.h"
#include "gloox/tag.h"
#include "ControlBase.h"
#include "RFDevice.h"

using namespace gloox;

class RFDeviceControl:public ControlBase
{
private:
	std::string m_name;
	RFDevice *m_device;
	RFCommand m_cmd;

public:
	RFDeviceControl(RFDevice *device);
	RFDeviceControl(const Tag* tag = 0);
	~RFDeviceControl(void);
		
	
	const std::string& GetName() const
	{
		return this->GetDevice()->GetName();
	}

	virtual void SetName( const std::string &name )
	{
		m_name = name;
	}

	IOTDeviceType GetType() const
	{
		return IOT_DEVICE_RFDevice;
	}
	RFDevice* GetDevice() const
	{
		return this->m_device;
	}
	void SetCommand(RFCommand cmd)
	{
		m_cmd = cmd;
	}
	RFCommand GetCommand()
	{
		return this->m_cmd;
	}
	DeviceAddress *GetFristAddress();

	Tag* tag(const struTagParam &TagParam) const;
	virtual ControlBase* clone( bool CreateLock=true ) const;
};

#endif
