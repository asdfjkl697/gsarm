#ifndef IRFDEVICEHANDLER_H_
#define IRFDEVICEHANDLER_H_

#include "RFDevice.h"
#include "RFDeviceControl.h"

class IRFDeviceHandler
{
public:
	virtual void OnDeviceBroadcast(uint32_t productID){};
	virtual void OnDevicePassError(){};
	virtual void OnDeviceInfo(RFDeviceControl *ctl){};
	virtual void OnDeviceAddress(DeviceAddress *addr){};
};

#endif