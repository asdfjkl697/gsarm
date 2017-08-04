#ifndef __TIMER_HANDLER_H
#define __TIMER_HANDLER_H

class TimerHandler
{
	private:
		struct event *timeout_event;

	public:
		void SetEvent(struct event *evt){ timeout_event = evt; };
		struct event *GetEvent(){ return timeout_event; };
		virtual void OnTimer() = 0;
};

#endif
