#ifndef RFDEVICE_H_
#define RFDEVICE_H_

#include "typedef.h"
#include "DeviceAddress.h"

class RFDevice
{
private:
	uint32_t m_ProductID;
	uint32_t m_Passcode;
	std::string m_SN;
	std::string m_Name;
	uint32_t m_AddressCount;
	uint8_t m_AddressType;
	std::list<DeviceAddress *> m_AddressQueue;

private:
	//uint8_t *RFDevice::GetDataToStr(DataType type,uint8_t *data,std::string& val); //jyc20160824 modify
	uint8_t *GetDataToStr(DataType type,uint8_t *data,std::string& val);

public:
	RFDevice(void){};
	RFDevice(uint32_t pid,uint32_t passcode,std::string& sn,std::string& name,uint32_t addrCount,uint32_t addrType);
	~RFDevice(void);

	void SetProductid(uint32_t pid)
	{
		m_ProductID = pid;
	}
	void SetPasscode(uint32_t passcode)
	{
		m_Passcode = passcode;
	}
	void SetSN(std::string sn)
	{
		m_SN = sn;
	}
	void SetName(std::string name)
	{
		m_Name = name;
	}
	void SetAddressCount(uint32_t count)
	{
		m_AddressCount = count;
	}
	void SetAddressType(uint8_t type)
	{
		m_AddressType = type;
	}
	uint32_t GetProductid() const
	{
		return m_ProductID;
	}
	uint32_t GetPasscode() const
	{
		return m_Passcode;
	}
	const std::string& GetSN()const
	{
		return m_SN;
	}
	const std::string& GetName()const
	{
		return m_Name;
	}
	uint32_t GetAddressCount() const
	{
		return m_AddressCount;
	}	
	uint8_t GetAddressType() const
	{
		return m_AddressType;
	}
	std::list<DeviceAddress *> GetAddressList() const
	{
		return this->m_AddressQueue;
	}
	DeviceAddress *GetAddress(uint32_t address);

	void AddAddress(DeviceAddress *addr)
	{
		m_AddressQueue.push_back(addr);
	}
	
	void ClearAddress(){
		m_AddressQueue.clear();
	}
	
	bool AddressInfoIsComplete();
	DeviceAddress *GetErrorTypeAddress();

	bool DecodeData(uint8_t *data,int size);
	DeviceAddress *DecodeAddressData(uint8_t *data,int size);

	virtual RFDevice* clone() const;
};

#endif

