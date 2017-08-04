#ifndef TIMERMANAGER_H_
#define TIMERMANAGER_H_

#include <list>
#include "ITimerHandler.h"

#include "common.h"


typedef struct _stTimer
{
	int TimerID;
	int expired;
	int second;
	ITimerHandler *handler;
}TIMER_ST;

class TimerManager
{
private:
	bool isRunning;
	std::list<TIMER_ST> timerQueue;

public:
	TimerManager(void);
	~TimerManager(void);
	
	bool m_isThreadExit;

	bool IsRunning()
	{
		return this->isRunning;
	}
	
	bool IsThreadExit()
	{
		return this->m_isThreadExit;
	}

	void OnThreadCall();
	void registerTimer(ITimerHandler *handler, int TimerID, int second);
	void removeTimer(ITimerHandler *handler, int TimerID);

	void Run();
	void Stop();
};

#endif

