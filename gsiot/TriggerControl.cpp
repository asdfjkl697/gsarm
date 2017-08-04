#include "TriggerControl.h"
#include "gloox/util.h"
//#include <Windows.h>
#include "RunCode.h"
#include "common.h" //20160607

TriggerControl::TriggerControl( const RFSignal &signal, bool AGRunState )
	:AGRunStateBase(AGRunState), m_signal(signal), m_lastTimeTick_RecvStart(0),m_lastTimeTick_Do(0),m_cur_trigger_count(0)
{
	InitNewMutex();
}


TriggerControl::~TriggerControl(void)
{
	if( m_pmutex_Trigger )
	{
		delete m_pmutex_Trigger;
		m_pmutex_Trigger = NULL;
	}
}

void TriggerControl::InitNewMutex( bool CreateLock )
{
	if( !CreateLock )
	{
		m_pmutex_Trigger = NULL;
		return;
	}

	// 只有在新建实例时才会调用，所以这里是不判断是否已分配，总是强制分配
	m_pmutex_Trigger = new gloox::util::Mutex();
}

bool TriggerControl::IsTrigger( bool isdoNow )
{
	if( !m_pmutex_Trigger ) return false;

	const uint32_t SYS_TriggerAlarmInterval = RUNCODE_Get(defCodeIndex_SYS_TriggerAlarmInterval);

	gloox::util::MutexGuard( this->m_pmutex_Trigger );

	if( this->m_lastTimeTick_Do != 0 
		&& timeGetTime() - this->m_lastTimeTick_Do < SYS_TriggerAlarmInterval*1000 )
	{
		return false; //notice delay limit by 'SYS_TriggerAlarmInterval'  jyc20170223 trans
	}

	if(this->m_cur_trigger_count >= this->m_signal.signal_count){

		if( isdoNow )
		{
			SetTriggerDo_nolock();
		}

	    return true;
	}
	return false;
}

void TriggerControl::CompareTick()
{
	if( !m_pmutex_Trigger ) return ;

	gloox::util::MutexGuard( this->m_pmutex_Trigger );

	uint32_t timeTick = timeGetTime();
	if(timeTick - m_lastTimeTick_RecvStart < 9000){ //小于一秒
		this->m_cur_trigger_count++;
	}else{
	    m_lastTimeTick_RecvStart = timeTick;
		this->m_cur_trigger_count = 1;//0
		//this->m_lastTimeTick_Do = 0;
	}
}

void TriggerControl::SetTriggerDo()
{
	if( !m_pmutex_Trigger ) return ;

	gloox::util::MutexGuard( this->m_pmutex_Trigger );

	SetTriggerDo_nolock();
}

void TriggerControl::SetTriggerDo_nolock()
{
	this->m_lastTimeTick_Do = timeGetTime();

	if( 0==this->m_lastTimeTick_Do )
	{
		this->m_lastTimeTick_Do++;
	}
}

Tag* TriggerControl::tag(const struTagParam &TagParam) const
{
	Tag* i = new Tag( "signal" );

	new Tag(i,"freq",util::int2string(this->m_signal.freq));
	new Tag(i,"code",util::int2string(this->m_signal.code));

	if( IsSupportAGRunState() )
	{
		new Tag(i,"agstate",util::int2string(GetAGRunStateFinalST()));
	}

	//new Tag(i,"one_low_time",util::int2string(this->m_signal.one_low_time));
	//new Tag(i,"one_high_time",util::int2string(this->m_signal.one_high_time));
	//new Tag(i,"zero_low_time",util::int2string(this->m_signal.zero_low_time));
	//new Tag(i,"zero_high_time",util::int2string(this->m_signal.zero_high_time));
	//new Tag(i,"pulse_split_time",util::int2string(this->m_signal.pulse_split_time));
	//new Tag(i,"pulse_bit_size",util::int2string(this->m_signal.pulse_bit_size));

	return i;
}
