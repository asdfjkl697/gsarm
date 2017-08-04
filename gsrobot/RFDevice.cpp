#include "RFDevice.h"
#include "common.h"
#include "gloox/util.h"
#include <sstream>
#include <iostream>

RFDevice::RFDevice(uint32_t pid,uint32_t passcode,std::string& sn,std::string& name,uint32_t addrCount,uint32_t addrType)
	:m_ProductID(pid),
	m_Passcode(passcode),
	m_SN(sn),
	m_Name(name),
	m_AddressCount(addrCount),
	m_AddressType(addrType)
{
}


RFDevice::~RFDevice(void)
{
	while(m_AddressQueue.size()>0)
	{
		DeviceAddress *addr = m_AddressQueue.front();
		delete(addr);
		m_AddressQueue.pop_front();
	}
}

DeviceAddress *RFDevice::GetAddress(uint32_t address)
{
	std::list<DeviceAddress *>::const_iterator it = m_AddressQueue.begin();
	for(;it!=m_AddressQueue.end();it++){
		if((*it)->GetAddress()== address){
		    return (*it);
		}
	}
	return NULL;
}

bool RFDevice::DecodeData(uint8_t *data,int size)
{
	//ack
	if(0xac!=*data++){
	   return false;
	}
	data++; //packet index ignoe
	if(IOT_String!=*data++){
	   return false;
	}

	int strlen = *data++;
	this->m_SN = ByteToString(data,strlen);
	data+=strlen;
	if(IOT_Integer!=*data++){
		return false;
	}

	this->m_AddressCount = ByteToInt32(data);
	data+=4;
	this->m_AddressType = *data++;

	for (uint32_t i=0; i < m_AddressCount;){
        switch(m_AddressType)
		{
		case IOT_Integer:
			{
				this->m_AddressQueue.push_back(new DeviceAddress(ByteToInt32(data)));
				data+=4;
				i+=4;
				break;
			}
		case IOT_Byte:
			{				
				this->m_AddressQueue.push_back(new DeviceAddress(*data++));
				i++;
				break;
			}
		case IOT_Int16:
			{
				this->m_AddressQueue.push_back(new DeviceAddress(ByteToInt16(data)));
				data+=2;
				i+=2;
				break;
			}
		default:
			{
				i++;
				break;
			}
		}
	}

	return true;
}

uint8_t *RFDevice::GetDataToStr(DataType type,uint8_t *data,std::string& val)
{
    switch(type)
	{
	case IOT_Integer:
		{
			val = util::int2string(ByteToInt32(data));
			return data+4;
		}
    case IOT_String:
		{
			uint8_t len = *data++;
			val = ByteToString(data,len);
			return data+len;
		}
    case IOT_Boolean:
		{
			val = *data++ ? "1":"0";
			return data;
		}
	case IOT_Byte:
		{
			val = util::int2string(*data++);
			return data;
		}
	case IOT_Int16:
		{
			val = util::int2string(ByteToInt16(data));
			return data+2;
		}
	case IOT_Long:
		{
			std::ostringstream buffer;
			double d_val = ByteToDouble(data);
			buffer << d_val;
			val = buffer.str();
			return data+8;
		}
	case IOT_Double:
		{
			std::ostringstream buffer;
			double d_val = ByteToDouble(data);
			buffer << d_val;
			val = buffer.str();
			return data+8;
		}
	case IOT_Float:
		{  
			std::ostringstream buffer;
			float f_val = ByteToFloat(data);
			buffer << f_val;
			val = buffer.str();
			return data+4;
		}
	}
	return data;
}

DeviceAddress *RFDevice::DecodeAddressData(uint8_t *data,int size)
{
	uint8_t *buf = data;
	//ack
	if(0xac!=*buf++){
	   return NULL;
	}
	buf++; //packet index ignoe
	uint8_t addressType = *buf++;
	uint32_t address = 0;
	switch(addressType)
		{
		case IOT_Integer:
			{
				address = ByteToInt32(buf);
				buf+=4;
				break;
			}
		case IOT_Byte:
			{				
				
				address = *buf++;
				break;
			}
		case IOT_Int16:
			{
				address = ByteToInt16(buf);
				buf+=2;
				break;
			}
	}

	DeviceAddress *addr = NULL;
	
	std::list<DeviceAddress *>::const_iterator it = m_AddressQueue.begin();
	for(;it!=m_AddressQueue.end();it++){
		if((*it)->GetAddress()==address){
			addr = (*it);
			break;
		}
	}

	if(addr){
		uint8_t moduleType = *buf++;
		uint8_t readType = *buf++;
		uint8_t valueType = *buf++;
		std::string val;

		addr->SetDeviceType((IOTDeviceType)moduleType);
		addr->SetReadType((IOTDeviceReadType)readType);
		addr->SetDataType((DataType)valueType);

		if(readType == IOT_READ){
			   buf = this->GetDataToStr(addr->GetDataType(),buf, val);
			addr->SetCurValue(val);
		}else{
			while(size - (buf - data)>0){
			    uint8_t valueDef = *buf++;
				buf = this->GetDataToStr(addr->GetDataType(),buf, val);
				switch(valueDef)
				{
					case 0x00:
						{  
							//defualt
							addr->SetDefualtValue(val);
							break;
						}
					case 0x01:
						{
							//cur
			                addr->SetCurValue(val);
							break;
						}
					case 0x02:
						{
							//minValue
							addr->SetMinValue(val);
							break;
						}
					case 0x03:
						{
							//maxValue
							addr->SetMaxValue(val);
							break;
						}
				}
			}
		}
	}
	return addr;
}

bool RFDevice::AddressInfoIsComplete()
{
	std::list<DeviceAddress *>::const_iterator it = m_AddressQueue.begin();
	for(;it!=m_AddressQueue.end();it++){
		if((*it)->GetType()==IOT_DEVICE_Unknown){
		    return false;
		}
	}
	return true;
}

DeviceAddress *RFDevice::GetErrorTypeAddress()
{
	std::list<DeviceAddress *>::const_iterator it = m_AddressQueue.begin();
	for(;it!=m_AddressQueue.end();it++){
		if((*it)->GetType()==IOT_DEVICE_Unknown){
		    return *it;
		}
	}
	return NULL;
}

RFDevice* RFDevice::clone() const
{
	RFDevice *dev = new RFDevice(*this);
	dev->ClearAddress();
	std::list<DeviceAddress *>::const_iterator it = m_AddressQueue.begin();
	for(;it!=m_AddressQueue.end();it++){
		dev->AddAddress((DeviceAddress *)(*it)->clone());
	}
	return dev;
}