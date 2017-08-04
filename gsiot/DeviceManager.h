#ifndef DEVICEMANAGER_H_
#define DEVICEMANAGER_H_

#include "SQLiteHelper.h"
#include "GSIOTDevice.h"
#include "TriggerControl.h"
#include "RFDevice.h"
#include "RFDeviceControl.h"
#include "CANDeviceControl.h"
#include "RFRemoteControl.h"
#include "RS485DevControl.h"
#include "PresetManager.h"

class DeviceManager: public SQLiteHelper
{
private:
	std::list<GSIOTDevice *> m_devices;

public:
	DeviceManager(void);
	~DeviceManager(void);
	
	GSIOTDevice* GetIOTDevice( IOTDeviceType deviceType, uint32_t deviceId ) const;

	int AddController(ControlBase* controller, const std::string &ver=c_DefaultVer);
	void AddTempController(ControlBase* controller, const std::string &ver=c_DefaultVer);
	
	bool Add_remote_button( GSIOTDevice *iotdevice, RemoteButton *pRemoteButton );
	bool Add_DeviceAddress( GSIOTDevice *iotdevice, DeviceAddress *pAddr );
	bool Add_Preset( GSIOTDevice *iotdevice, CPresetObj *pPreset );

	bool DeleteDevice( GSIOTDevice *iotdevice );
	bool Delete_remote_button( GSIOTDevice *iotdevice, RemoteButton *pRemoteButton );
	bool Delete_DeviceAddress( GSIOTDevice *iotdevice, uint32_t address );
	bool Delete_Preset( GSIOTDevice *iotdevice, uint32_t Presetid );

	bool DB_DeleteDeviceAddress( GSIOTDevice *iotdevice );
	bool DB_Delete_rfsignal( int signal_id );
	bool DB_Delete_remote_button( GSIOTDevice *iotdevice );
	bool DB_DeleteOne_remote_button( IOTDeviceType device_type, int device_id, RemoteButton *pRemoteButton );
	bool DB_DeleteOne_DeviceAddress( IOTDeviceType device_type, int device_id, uint32_t address );
	bool DB_DeleteOne_Preset( uint32_t Presetid, IOTDeviceType device_type, int device_id );

public:
	bool DB_ModifyDevice( GSIOTDevice *iotdevice, uint32_t ModifySubFlag=0 );
	bool DB_ModifyAddress( GSIOTDevice *iotdevice, const DeviceAddress *addr );
	bool DB_Modify_rfsignal_s( const RFSignal &signal );
	bool DB_Modify_rfsignal( const RFSignal &signal );
	bool DB_Modify_remote_button( IOTDeviceType device_type, int device_id, const RemoteButton *pRemoteButton );
	bool DB_CleanSignal_remote_button( RemoteButton *pRemoteButton );
	bool DB_Modify_Preset( const CPresetObj *pPreset, IOTDeviceType device_type, int device_id );
	
	bool Swap_Preset( CPresetObj *pPresetA, CPresetObj *pPresetB, IOTDeviceType device_type, int device_id );

	bool SaveSort_Preset( GSIOTDevice *iotdevice );
	bool SaveSort_RemoteButton( GSIOTDevice *iotdevice );

	std::list<GSIOTDevice *> GetDeviceList()
	{
		return this->m_devices;
	}
	int SaveToDB(GSIOTDevice* device);
	void SaveToFile(){};

	bool LoadDB_rfsignal();
	bool LoadDB_RS485();
	bool LoadDB_rf_trigger();
	bool LoadDB_rf_remote();
	bool LoadDB_remote_button( const IOTDeviceType device_type, const int device_id, RemoteButtonManager *pRemoteButtonManager, IOTDeviceType ExType );
	bool LoadDB_Preset( defDBSavePresetQueue &que );

	int SaveToDB_rfsignal( const RFSignal &signal );
	int SaveToDB_RS485(GSIOTDevice* device);
	int SaveToDB_rf_trigger(GSIOTDevice* device);
	int SaveToDB_rf_remote(GSIOTDevice* device);
	int SaveToDB_remote_button( const IOTDeviceType device_type, const int device_id, RemoteButton *pRemoteButton );
	int SaveToDB_DeviceAddress( const IOTDeviceType device_type, const int device_id, DeviceAddress *pAddr );
	
	bool CheckExist_ACCtl( const std::string &ip, const int selfid=-1 );

	uint32_t GetDevCount( const IOTDeviceType ExType );

protected:
	int SaveToDB_Preset( const IOTDeviceType device_type, const int device_id, const CPresetObj *pPreset );

private:
	std::map<int,RFSignal> m_mapRFSignal; // 载入数据库时 临时缓存<id,signal>
};

#endif

