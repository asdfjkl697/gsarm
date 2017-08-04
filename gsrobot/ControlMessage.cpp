#include "ControlMessage.h"
#include "gsiot/GSIOTDevice.h"


ControlMessage::ControlMessage(const JID& jid,const std::string& id,GSIOTDevice *device,GSIOTObjBase *obj, uint32_t overtime )
	:m_jid(jid),m_id(id),m_device(device),m_time(timeGetTime()),m_overtime(overtime)
{
	m_obj = (GSIOTObjBase*)(obj?obj->cloneObj():NULL);
}


ControlMessage::~ControlMessage(void)
{
	if(this->m_device){
	    delete(m_device);
		m_device = NULL;
	}
	
	if( m_obj )
	{
		delete m_obj;
		m_obj = NULL;
	}
}

void ControlMessage::SetNowTime()
{
	m_time = timeGetTime();
}

bool ControlMessage::IsOverTime()
{
	return ( timeGetTime()-m_time > m_overtime );
}

void ControlMessage::Print( const char *info )
{
	LOGMSG( "%s Jid=%s, id=%s, DevID=%d, addr=%d\r\n", 
		info?info:"",
		this->GetJid().full().c_str()?this->GetJid().full().c_str():"",
		this->GetId().c_str()?this->GetId().c_str():"",
		this->GetDevice()?this->GetDevice()->getId():0,
		/*this->GetAddress()?this->GetAddress()->GetAddress():*/0 );
}
