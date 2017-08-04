#ifndef CONTROLMESSAGE_H_
#define CONTROLMESSAGE_H_

#include "gloox/gloox.h"
#include "gloox/jid.h"
#include "GSIOTDevice.h"
#include "ControlBase.h"
#include "GSIOTObjBase.h"
#include "common.h"

using namespace gloox;

class ControlMessage
{
private:
	JID m_jid;
	std::string m_id;
	GSIOTDevice *m_device;
	GSIOTObjBase *m_obj;

	uint32_t m_time;
	uint32_t m_overtime;

public:
	ControlMessage(const JID& jid,const std::string& id,GSIOTDevice *device,GSIOTObjBase *obj, uint32_t overtime=defNormMsgOvertime );
	~ControlMessage(void);

	GSIOTDevice *GetDevice()
	{
		return m_device;
	}
	GSIOTObjBase *GetObj()
	{
		return this->m_obj;
	}
	const JID& GetJid() const 
	{
		return this->m_jid;
	}
	const std::string& GetId() const 
	{
		return this->m_id;
	}

	void SetNowTime();
	bool IsOverTime();
	void Print( const char *info );
};

#endif
