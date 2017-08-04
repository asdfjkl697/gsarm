#ifndef AutoNoticeEvent_H_
#define AutoNoticeEvent_H_

#include "ControlEvent.h"

using namespace gloox;

class AutoNoticeEvent:public ControlEvent
{
private:
	std::string m_to_jid;
	std::string m_msg_subject;
	std::string m_msg_body;

public:
	AutoNoticeEvent(void);
	AutoNoticeEvent(const Tag* tag);
	~AutoNoticeEvent(void);

	bool doEditAttrFromAttrMgr( const EditAttrMgr &attrMgr, defCfgOprt_ oprt = defCfgOprt_Modify );

	EventType GetType() const
	{
		return NOTICE_Event;
	}

	//void SetToJid( std::string &to_jid )
	void SetToJid(const std::string &to_jid )
	{
		this->m_to_jid = to_jid;
	}

	//void SetSubject( std::string &subject ) //jyc20170224
	void SetSubject(const std::string &subject )
	{
		this->m_msg_subject = subject;
	}

	//void SetBody( std::string &body  )
	void SetBody(const std::string &body  )
	{
		this->m_msg_body = body;
	}

	std::string GetToJid() const
	{
		return m_to_jid;
	}

	std::string GetSubject() const
	{
		return m_msg_subject;
	}

	std::string GetBody() const
	{
		return m_msg_body;
	}

	bool EventParamIsComplete();

	Tag* tag(const struTagParam &TagParam) const;
	virtual ControlEvent* clone() const{
		return new AutoNoticeEvent(*this);
	}
};

#endif
