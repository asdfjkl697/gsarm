#ifndef _IO_TASK_H
#define _IO_TASK_H

#include "task.h"

class IOTask
{
	private:
		struct event *_io_event;

	public:
		void SetEvent(struct event *io_event){ _io_event = io_event; };
		struct event *GetEvent(){ return _io_event; };

		virtual void OnEvent() = 0;
		virtual int GetFD() = 0;
};

#endif
