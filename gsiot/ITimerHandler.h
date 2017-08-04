#ifndef ITIMERHANDLER_H_
#define ITIMERHANDLER_H_

#include <stdint.h>

class ITimerHandler
{
public:
	virtual void OnTimer( int TimerID ){};
};

#endif