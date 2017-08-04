#ifndef IPCAMERAMANAGER_H_
#define IPCAMERAMANAGER_H_

#include "SQLiteHelper.h"
//#include "TI368Camera.h" //jyc20170331 remove
//#include "SSD1935Camera.h"
//#include "HikCamera.h"
//#include "DHCamera.h"
#include "IPCameraBase.h"
#include "gsiot/GSIOTDevice.h"

class IPCameraManager:public SQLiteHelper
{
private:	
	std::list<GSIOTDevice *> m_cameraQueue;

public:
	IPCameraManager(void);
	~IPCameraManager(void);

	GSIOTDevice* GetIOTDevice( uint32_t deviceId ) const;

	IPCameraBase* FindIPCamera( std::string &ip, int port, int channel, int streamfmt );
	int AddIPCamera(IPCameraBase *cam);
	bool RemoveIPCamera(GSIOTDevice *cam);
	bool ModifyDevice(GSIOTDevice *cam, uint32_t ModifySubFlag=0 );

	void InitAllCamera_Preset( const defDBSavePresetQueue &que );

	std::list<GSIOTDevice *>& GetCameraList()
	{
		return m_cameraQueue;
	}
	int SaveToDB(GSIOTDevice *dev);
	void SaveToFile();
};

#endif

