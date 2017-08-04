#ifndef IDEVICEHANDLER_H
#define IDEVICEHANDLER_H

#include "typedef.h"
#include <stdint.h>
#include "gsiot/GSIOTDevice.h"
#include "ControlBase.h"
#include "DeviceAddress.h"

enum defDeviceNotify_
{
	defDeviceNotify_Unknown = 0,
	defDeviceNotify_Connect,
	defDeviceNotify_Disconnect,
	defDeviceNotify_Modify,
	defDeviceNotify_StateChanged
};

class IDeviceHandler
{
public:
	virtual void OnDeviceNotify( defDeviceNotify_ notify, GSIOTDevice *iotdevice, DeviceAddress *addr ){};
	virtual void OnDeviceDisconnect(GSIOTDevice *iotdevice){};
	virtual void OnDeviceConnect(GSIOTDevice *iotdevice){};
	virtual void OnDeviceData( defLinkID LinkID, GSIOTDevice *iotdevice, ControlBase *ctl, GSIOTObjBase *addr ){};
};

#endif
