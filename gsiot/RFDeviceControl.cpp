#include "RFDeviceControl.h"
#include "gloox/util.h"


RFDeviceControl::RFDeviceControl(RFDevice* device)
	:m_device(device)
{
}

RFDeviceControl::RFDeviceControl( const Tag* tag)
	:ControlBase(tag)
{
	if( !tag || tag->name() != defDeviceTypeTag_rfdevice)
      return;

	m_device = new RFDevice();

	if(tag->hasAttribute("name"))
	   this->m_device->SetName(tag->findAttribute("name"));
	if(tag->hasAttribute("sn"))
	   this->m_device->SetSN(tag->findAttribute("sn"));	
	if(tag->hasAttribute("pro_id"))
	   this->m_device->SetProductid(atoi(tag->findAttribute("pro_id").c_str()));	
	if(tag->hasAttribute("pass_code"))
	   this->m_device->SetPasscode(atoi(tag->findAttribute("pass_code").c_str()));
	if(tag->hasAttribute("addr_count"))
	   this->m_device->SetAddressCount(atoi(tag->findAttribute("addr_count").c_str()));
	if(tag->hasAttribute("addr_type"))
	   this->m_device->SetAddressType(atoi(tag->findAttribute("addr_type").c_str()));
	if(tag->hasAttribute("command"))
	   this->m_cmd = (RFCommand)atoi(tag->findAttribute("command").c_str());

	const TagList& l = tag->children();
	TagList::const_iterator it = l.begin();
    for( ; it != l.end(); ++it )
    {
        const std::string& name = (*it)->name();
		if(name == "address"){
			this->m_device->AddAddress(new DeviceAddress(*it));
		}
	}
}

RFDeviceControl::~RFDeviceControl(void)
{
	if(m_device){
	   delete(m_device);
	}
}

Tag* RFDeviceControl::tag(const struTagParam &TagParam) const
{
	Tag* i = new Tag( defDeviceTypeTag_rfdevice );
	
	i->addAttribute("name",this->m_device->GetName());
	i->addAttribute("sn",this->m_device->GetSN());
	i->addAttribute("pro_id",util::int2string(m_device->GetProductid()));
	i->addAttribute("pass_code",util::int2string(m_device->GetPasscode()));
	i->addAttribute("addr_count",util::int2string(m_device->GetAddressCount()));
	i->addAttribute("addr_type",util::int2string(m_device->GetAddressType()));

	if(m_device->GetAddressCount()>0){
		std::list<DeviceAddress *> addressList = m_device->GetAddressList();
		std::list<DeviceAddress *>::const_iterator it = addressList.begin();
		for(;it!=addressList.end();it++){
			i->addChild((*it)->tag(TagParam));
		}
	}
	
	return i;
}

DeviceAddress *RFDeviceControl::GetFristAddress()
{
	std::list<DeviceAddress *> addressList = m_device->GetAddressList();
	std::list<DeviceAddress *>::const_iterator it = addressList.begin();
	for(;it!=addressList.end();it++){
		return (*it);
	}
	return NULL;
}

ControlBase* RFDeviceControl::clone( bool CreateLock ) const{
	return new RFDeviceControl(this->m_device->clone());
}