#ifndef ICANDEVICEHANDLER_H_
#define ICANDEVICEHANDLER_H_

#include "CANDeviceControl.h"

class ICANDeviceHandler
{
public:
	virtual void OnDeviceInfo(CANDeviceControl *ctl){};
	virtual void OnAddressInfo(CANDeviceControl *ctl){};	
	virtual void OnDeviceAddress(DeviceAddress *addr){};
};

#endif