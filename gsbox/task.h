#ifndef _TASK_H
#define _TASK_H

#include "typedef.h"
#include "event2/event.h"

class Task
{
	public:
		virtual void Run() = 0;
};

#endif