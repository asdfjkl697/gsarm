#include "radiosynchandler.h"

RadioSyncHandler::RadioSyncHandler(RadioTimerHandler *hd, TaskManager *tm)
:handler(hd), isRunning(false), task(tm)
{
}

RadioSyncHandler::~RadioSyncHandler(void)
{
	Stop();
}

void RadioSyncHandler::OnTimer()
{
	switch(_type)
	{
		case DiscoveryCall:
		{
		    handler->AsyncDiscoveryCall();
			break;
		}
		case ChildDiscoveryCall:
		{
		    handler->AsyncChildDiscoveryCall();
			break;
		}
		case RadioDiscovery:
		{
			handler->RadioDiscoveryCall();
			break;
		}
		case PrecessPacketCall:
		{
			handler->AsyncPrecessPacketCall();
			break;
		}
		case ComDataBackCall:
		{
			handler->AsyncComDataCall();
			break;
		}
	}
}

void RadioSyncHandler::Start(RadioAsyncType type, uint32_t millisecond)
{
	_type = type;

	if(!isRunning){
		isRunning = true;
		task->AddTimer(this, millisecond);
	}
}

void RadioSyncHandler::Stop()
{
	if(isRunning){
		task->RemoveTimer(this);
		isRunning = false;		
	}
}
