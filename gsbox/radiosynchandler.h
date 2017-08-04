#ifndef _RADIO_SYNC_HANDLER_H
#define _RADIO_SYNC_HANDLER_H

#include "timerhandler.h"
#include "radiotimerhandler.h"
#include "taskmanager.h"

enum RadioAsyncType
{
	DiscoveryCall = 0,
	RadioDiscovery = 1,
	ChildDiscoveryCall = 2,
	PrecessPacketCall = 3,
	ComDataBackCall = 4
};

class RadioSyncHandler:public TimerHandler
{
	private:
		RadioTimerHandler *handler;
		RadioAsyncType _type;
		bool isRunning;
		TaskManager *task;
		
	public:
		RadioSyncHandler(RadioTimerHandler *hd, TaskManager *tm);
		~RadioSyncHandler(void);
		void OnTimer();

		void Start(RadioAsyncType type, uint32_t millisecond);
		void Stop();
};

#endif
