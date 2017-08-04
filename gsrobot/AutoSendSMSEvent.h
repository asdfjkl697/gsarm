#pragma once
//#include "controlevent.h"
#include "ControlEvent.h"

class AutoSendSMSEvent :
	public ControlEvent
{
public:
	AutoSendSMSEvent( std::string &Phone, std::string &SMS, uint32_t flag=0, uint32_t SendCount=1 );
	AutoSendSMSEvent(const Tag* tag);
	~AutoSendSMSEvent(void);
	
	bool doEditAttrFromAttrMgr( const EditAttrMgr &attrMgr, defCfgOprt_ oprt = defCfgOprt_Modify );

	bool isTimeOver() const;

	EventType GetType() const
	{
		return SMS_Event;
	}

	void SetFlag( uint32_t Flag )
	{
		m_flag = Flag;
	}

	void SetPhone( const std::string &Phone )
	{
		this->m_Phone = Phone;
	}

	void SetSMS( const std::string &SMS  )
	{
		this->m_SMS = SMS;
	}

	uint32_t SetSendCount( uint32_t SendCount )
	{
		if( SendCount>0 && SendCount<9)
		{
			m_SendCount = SendCount;
		}
		else
		{
			m_SendCount = 1;
		}

		return m_SendCount;
	}

	uint32_t GetFlag() const
	{
		return m_flag;
	}

	const std::string& GetPhone() const
	{
		return m_Phone;
	}

	const std::string& GetSMS() const
	{
		return m_SMS;
	}

	uint32_t GetSendCount() const
	{
		if( m_SendCount>0 && m_SendCount<9)
		{
			return m_SendCount;
		}

		return uint32_t(1);
	}

	bool EventParamIsComplete();

	Tag* tag(const struTagParam &TagParam) const;
	virtual ControlEvent* clone() const{
		return new AutoSendSMSEvent(*this);
	}

private:
	uint32_t m_ts;
	uint32_t m_flag;
	std::string m_Phone;
	std::string m_SMS;
	uint32_t m_SendCount;
};

