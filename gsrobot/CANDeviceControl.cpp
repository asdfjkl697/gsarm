#include "CANDeviceControl.h"
#include "common.h"
#include "gloox/util.h"
#include <sstream>
#include <iostream>

CANDeviceControl::CANDeviceControl(uint32_t devid,uint32_t pid,uint8_t addrcount)
	:m_deviceID(devid),m_ProductID(pid), m_AddressCount(addrcount)
{
}

CANDeviceControl::CANDeviceControl( const Tag* tag)
	:ControlBase(tag)
{
	if( !tag || tag->name() != defDeviceTypeTag_candevice)
      return;
	
	if(tag->hasAttribute("device_id"))
		this->m_deviceID = atoi(tag->findAttribute("device_id").c_str());	
	if(tag->hasAttribute("pro_id"))
		this->m_ProductID = atoi(tag->findAttribute("pro_id").c_str());	
	if(tag->hasAttribute("addr_count"))
		this->m_AddressCount = atoi(tag->findAttribute("addr_count").c_str());
	if(tag->hasAttribute("command"))
	   this->m_cmd = (CANCommand)atoi(tag->findAttribute("command").c_str());

	const TagList& l = tag->children();
	TagList::const_iterator it = l.begin();
    for( ; it != l.end(); ++it )
    {
        const std::string& name = (*it)->name();
		if(name == "address"){
			this->m_AddressQueue.push_back(new DeviceAddress(*it));
		}
	}
}

CANDeviceControl::~CANDeviceControl(void)
{
	while(m_AddressQueue.size()>0)
	{
		DeviceAddress *addr = m_AddressQueue.front();
		delete(addr);
		m_AddressQueue.pop_front();
	}
	if(m_CANBuffer_Queue.size()>0){
		m_CANBuffer_Queue.clear();
	}
}

Tag* CANDeviceControl::tag(const struTagParam &TagParam) const
{
	Tag* i = new Tag( defDeviceTypeTag_candevice );
	
	i->addAttribute("device_id",util::int2string(this->m_deviceID));
	i->addAttribute("pro_id",util::int2string(this->m_ProductID));
	i->addAttribute("addr_count",util::int2string(this->m_AddressCount));

	if(this->m_AddressCount>0){
		std::list<DeviceAddress *>::const_iterator it = this->m_AddressQueue.begin();
		for(;it!=m_AddressQueue.end();it++){
			i->addChild((*it)->tag(TagParam));
		}
	}
	
	return i;
}

ControlBase* CANDeviceControl::clone( bool CreateLock ) const
{
	CANDeviceControl *ctl = new CANDeviceControl(*this);
	ctl->ClearAddress();

	std::list<DeviceAddress *>::const_iterator it = this->m_AddressQueue.begin();
	for(;it!=m_AddressQueue.end();it++){
		ctl->AddAddress((DeviceAddress *)(*it)->clone());
	}
	return ctl;
}

DeviceAddress *CANDeviceControl::GetAddress(uint32_t address)
{
	std::list<DeviceAddress *>::const_iterator it = m_AddressQueue.begin();
	for(;it!=m_AddressQueue.end();it++){
		if((*it)->GetAddress()== address){
		    return (*it);
		}
	}
	return NULL;
}

DeviceAddress *CANDeviceControl::GetFristAddress()
{
	std::list<DeviceAddress *>::const_iterator it = m_AddressQueue.begin();
	for(;it!=m_AddressQueue.end();it++){
		return (*it);
	}
	return NULL;
}

void CANDeviceControl::DoPacketExtData(uint8_t *data,int size)
{
	uint8_t *buf = data;
	switch(*buf++)
	{
	case CAN_Address_Response:
		this->DecodeAddressData(buf, size - (buf - data));
		break;
	case CAN_Address_Setting_Response:
	case CAN_Address_Read_Response:
		{
			uint8_t addr = *buf++;
			int len = size - (buf - data);

			std::list<CANBuffer>::iterator it = m_CANBuffer_Queue.begin();
			for(;it!=m_CANBuffer_Queue.end();it++){
				if(it->cmd==data[0] && it->addr == addr){
					uint8_t *buffer = NULL;				
					buffer = (uint8_t*)malloc(it->len+len);
					memcpy(buffer,it->data,it->len);
					memcpy(buffer+it->len,buf,len);
					free(it->data);
					it->data = buffer;
					it->len += len;
					return;
				}
			}
			CANBuffer cbuf = {0};
			cbuf.cmd = data[0];
			cbuf.addr = addr;
			cbuf.len = len;
			cbuf.data = (uint8_t *)malloc(len);
			memcpy(cbuf.data,buf,len);
			m_CANBuffer_Queue.push_back(cbuf);
		    break;
		}
	}
}

bool CANDeviceControl::DecodeAddressData(uint8_t *data,int size)
{
	for(int i=0;i<size;i++){
		DeviceAddress *addr = this->GetAddress(data[i]);
		if(!addr){
		    this->m_AddressQueue.push_back(new DeviceAddress(data[i]));
		}
	}
	return true;
}

bool CANDeviceControl::AddressInfoIsComplete()
{
	std::list<DeviceAddress *>::const_iterator it = m_AddressQueue.begin();
	for(;it!=m_AddressQueue.end();it++){
		if((*it)->GetType()==IOT_DEVICE_Unknown){
		    return false;
		}
	}
	return true;
}

DeviceAddress *CANDeviceControl::GetErrorTypeAddress()
{
	std::list<DeviceAddress *>::const_iterator it = m_AddressQueue.begin();
	for(;it!=m_AddressQueue.end();it++){
		if((*it)->GetType()==IOT_DEVICE_Unknown){
		    return *it;
		}
	}
	return NULL;
}

uint8_t *CANDeviceControl::GetDataToStr(DataType type,uint8_t *data,std::string& val)
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
			val = 0x00 == *data++ ? "0":"1";
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

bool CANDeviceControl::HasExtPacket(CANCommand cmd,uint8_t addr)
{
	std::list<CANBuffer>::iterator it = m_CANBuffer_Queue.begin();
	for(;it!=m_CANBuffer_Queue.end();it++){
		if(it->cmd==cmd && it->addr == addr){
			return true;
		}
	}
	return false;
}


DeviceAddress *CANDeviceControl::DecodeAddressSettingData(uint8_t *data,int size)
{
	uint8_t tempData[512];
	uint8_t *buffer = tempData;
	int len = size -1;
	uint8_t address = *data++; 

	bool hasExtData = HasExtPacket(CAN_Address_Setting_Response,address);
	if(hasExtData){
		std::list<CANBuffer>::iterator it = m_CANBuffer_Queue.begin();
		for(;it!=m_CANBuffer_Queue.end();it++){
			if(it->cmd==CAN_Address_Setting_Response && it->addr == address){
				memcpy(buffer,it->data,it->len); //copy ext data frist
				memcpy(buffer+it->len,data,len);
				free(it->data); //释放掉原来的内存
				len+= it->len;
				m_CANBuffer_Queue.erase(it);
				break;
			}
		} 
	}else{
	    memcpy(tempData,data,len);
	}
	
	DeviceAddress *addr = this->GetAddress(address);
	if(addr){
		addr->SetObjectData(buffer,len - (buffer - tempData));
	}
	return addr;
}

DeviceAddress *CANDeviceControl::DecodeAddressInfoData(uint8_t *data,int size)
{
	uint8_t tempData[512];
	uint8_t *buffer = tempData;
	int len = size -1;
	uint8_t address = *data++; 

	bool hasExtData = HasExtPacket(CAN_Address_Read_Response,address);
	if(hasExtData){
		std::list<CANBuffer>::iterator it = m_CANBuffer_Queue.begin();
		for(;it!=m_CANBuffer_Queue.end();it++){
			if(it->cmd==CAN_Address_Read_Response && it->addr == address){
				memcpy(buffer,it->data,it->len); //copy ext data frist
				memcpy(buffer+it->len,data,len);
				free(it->data); //释放掉原来的内存
				len+= it->len;
				m_CANBuffer_Queue.erase(it);
				break;
			}
		} 
	}else{
	    memcpy(tempData,data,len);
	}
	
	uint8_t moduleType = *buffer++;
	uint8_t readType = *buffer++;
	uint8_t valueType = *buffer++;
	
	DeviceAddress *addr = this->GetAddress(address);
	if(addr){
		addr->SetDeviceType((IOTDeviceType)moduleType);
		addr->SetReadType((IOTDeviceReadType)readType);
		addr->SetDataType((DataType)valueType);
		
		std::string val;

		if(IOT_Object == valueType){
			addr->SetObjectData(buffer,len - (buffer - tempData));
		}else if(readType == IOT_READ){
			buffer = this->GetDataToStr(addr->GetDataType(),buffer, val);
			addr->SetCurValue(val);
		}else{
			while(len - (buffer - tempData)>0){
				uint8_t valueDef = *buffer++;
				buffer = this->GetDataToStr(addr->GetDataType(),buffer, val);
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