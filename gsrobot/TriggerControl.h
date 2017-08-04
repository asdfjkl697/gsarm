#ifndef TRIGGERCONTROL_H_
#define TRIGGERCONTROL_H_

#include "gloox/tag.h"
#include "ControlBase.h"
#include "RFSignalDefine.h"
#include "gloox/mutexguard.h"

using namespace gloox;

class TriggerControl:public ControlBase, public AGRunStateBase
{
public:
	TriggerControl( const RFSignal &signal, bool AGRunState=true );
	~TriggerControl(void);

	virtual bool IsSupportAGRunState() const
	{
		return true;
	}

	IOTDeviceType GetType() const
	{
		return IOT_DEVICE_Trigger;
	}

	RFSignal& GetSignal()
	{
		return this->m_signal;
	}
	
	void UpdateSignal( const RFSignal &signal, bool onlysignal )
	{
		this->m_signal.Update( signal, onlysignal );
	}

	virtual const std::string& GetName() const
	{
		return m_Name;
	}

	void SetName( const std::string &name )
	{
		m_Name = name;
	}
	
	bool IsTrigger( bool isdoNow );
	void CompareTick();
	void SetTriggerDo();
	uint32_t GetCurTriggerCount() const
	{
		return m_cur_trigger_count;
	}

	Tag* tag(const struTagParam &TagParam) const;

	virtual ControlBase* clone( bool CreateLock=true ) const
	{
		TriggerControl *pNew = new TriggerControl(*this);
		pNew->InitNewMutex( CreateLock );
		return pNew;
	}

private:
	void InitNewMutex( bool CreateLock=true );
	void SetTriggerDo_nolock();

private:
	gloox::util::Mutex *m_pmutex_Trigger;
	RFSignal m_signal;
	uint32_t m_lastTimeTick_RecvStart; // 本轮接收到的开始时间
	uint32_t m_lastTimeTick_Do; // 最近一次的触发执行时间
	uint32_t m_cur_trigger_count; //check count
	std::string m_Name;

};

#endif

