#ifndef CANDEVICECONTROL_H_
#define CANDEVICECONTROL_H_


#include "typedef.h"
#include "DeviceAddress.h"
#include "ControlBase.h"

using namespace gloox;


typedef struct _can_buffer
{
	uint8_t addr;
	uint8_t cmd;
	uint8_t *data;
	uint32_t len;
}CANBuffer;


class CANDeviceControl:public ControlBase
{
private:
	uint32_t m_deviceID;
	uint32_t m_ProductID;
	uint8_t m_AddressCount;
	std::string m_Name;
	std::list<DeviceAddress *> m_AddressQueue;
	std::list<CANBuffer> m_CANBuffer_Queue;
	CANCommand m_cmd;

private:
	uint8_t *GetDataToStr(DataType type,uint8_t *data,std::string& val);

public:
	CANDeviceControl(uint32_t devid,uint32_t pid,uint8_t addrcount);
	CANDeviceControl(const Tag* tag = 0);
	~CANDeviceControl(void);

	IOTDeviceType GetType() const
	{
		return IOT_DEVICE_CANDevice;
	}

    uint32_t GetDeviceid() const
	{
		return m_deviceID;
	}	
	uint32_t GetProductid() const
	{
		return m_ProductID;
	}	
	uint8_t GetAddressCount() const
	{
		return m_AddressCount;
	}	
	
	const std::string& GetName() const
	{
		return m_Name;
	}

	CANCommand GetCommand()
	{
		return m_cmd;
	}

	void SetCommand(CANCommand cmd)
	{
		m_cmd = cmd;
	}
	void SetName( const std::string &name )
	{
		m_Name = name;
	}
	void SetProductid(uint32_t pid)
	{
		m_ProductID = pid;
	}
	
	void SetAddressCount(uint32_t count)
	{
		m_AddressCount = count;
	}
	void AddAddress(DeviceAddress *addr)
	{
		m_AddressQueue.push_back(addr);
	}

	DeviceAddress *GetAddress(uint32_t address);
	DeviceAddress *GetFristAddress();
	void ClearAddress(){
		m_AddressQueue.clear();
		m_CANBuffer_Queue.clear();
	}
	std::list<DeviceAddress *> GetAddressList() const
	{
		return this->m_AddressQueue;
	}

	bool AddressInfoIsComplete();
	DeviceAddress *GetErrorTypeAddress();

	bool DecodeAddressData(uint8_t *data,int size);
	DeviceAddress *DecodeAddressInfoData(uint8_t *data,int size);
	DeviceAddress *DecodeAddressSettingData(uint8_t *data,int size);
	void DoPacketExtData(uint8_t *data,int size);
	void DoMergePacketQueue(uint8_t *data,int size);

	bool HasExtPacket(CANCommand cmd,uint8_t addr);

	Tag* tag(const struTagParam &TagParam) const;	
	virtual ControlBase* clone( bool CreateLock=true ) const;
};

#endif

