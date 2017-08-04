#ifndef _RADIO_TIMER_HANDLER_H
#define _RADIO_TIMER_HANDLER_H

class RadioTimerHandler
{
	public:
		virtual void AsyncDiscoveryCall() = 0;
		virtual void AsyncChildDiscoveryCall() = 0;
		virtual void RadioDiscoveryCall() = 0;
		virtual void AsyncPrecessPacketCall() = 0;
		virtual void AsyncComDataCall() = 0;
};

#endif
