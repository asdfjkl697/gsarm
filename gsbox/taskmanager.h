#ifndef _TASK_MANAGER_H
#define _TASK_MANAGER_H

#include <list>
#include "iotask.h"
#include "sockettask.h"
#include "tcpservertask.h"
#include "timerhandler.h"



class TaskManager
{
	private:
        	struct event_base *evbase;
		std::list<Task *> taskList;
		struct event *socket_event;
		struct event *timeout_event;
		bool m_waitexit;
		
    	public:
	    	TaskManager(void);
	    	~TaskManager(void);

		void AddIOTask(IOTask *task);
		void RemoveIOTask(IOTask *task);
		void AddSocketTask(SocketTask *task);
		void RemoveSocketTask(SocketTask *task);	
		void AddTCPServerTask(TCPServerTask *task);
		void RemoveTCPServerTask(TCPServerTask *task);	
		void AddTimer(TimerHandler *handler, uint32_t millisecond);
		void RemoveTimer(TimerHandler *handler);
		void Run();
		void Close();

};

#endif
