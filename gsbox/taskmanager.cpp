#include "taskmanager.h"
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <string.h>

#include "packet.h"
#include "event2/bufferevent.h"
#include "event2/buffer.h"
#include "event2/listener.h"

static long int GetTickCount(){
	struct timeval tv;
	if (gettimeofday(&tv, NULL) != 0)
		return 0;
	return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

static void Socket_Read(evutil_socket_t fd, short what, void *arg)
{
	SocketTask *task = (SocketTask *)arg;

	int ret;
	struct sockaddr_in sin_from;
	int addr_len = sizeof(struct sockaddr);
	char buffer[UDP_BUFFER_SIZE];

	ret = recvfrom(fd, buffer, UDP_BUFFER_SIZE, 0, (struct sockaddr *)&sin_from, (socklen_t*)&addr_len);
	if (SOCKET_ERROR == ret){
		printf("recvfrom error!, error code is %d\r\n", errno);
		return; // -1;
	}
	
	task->PacketRead((uint8_t *)buffer, ret, &sin_from);
}

static void onIOCallback(evutil_socket_t sock, short flag, void *arg)
{
	IOTask *task = (IOTask *)arg;
	task->OnEvent();
}

void TaskManager_OnTimer(evutil_socket_t fd, short what, void *arg)
{
	TimerHandler *handler = (TimerHandler *)arg;
	//handler->OnTimer();
}

static void connection_read(struct bufferevent *bev, void *arg)
{
	int size;
	char buffer[BUFFER_SIZE];
	Client *client = (Client *)arg;
	struct evbuffer *input = bufferevent_get_input(bev);
	TCPServerTask *task = (TCPServerTask *)client->GetTask();
	
	while ((size = evbuffer_remove(input, buffer, BUFFER_SIZE)) > 0) {
		task->OnDataRead(client, (uint8_t *)buffer, size);
	}
}

static void connection_event(struct bufferevent *bev, short what, void *arg)
{
	if (what & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
		bufferevent_free(bev);
	}
	
	Client *cl = (Client *)arg;
	TCPServerTask *task = (TCPServerTask *)cl->GetTask();
	task->OnDisconnect(cl);
    	delete(cl);
}

void connection_accept(struct evconnlistener *listener,
	evutil_socket_t fd, struct sockaddr *address, int socklen,
	void *ctx)
{
	/* We got a new connection! Set up a bufferevent for it. */
	struct event_base *base = evconnlistener_get_base(listener);
	struct bufferevent *bev = bufferevent_socket_new(
		base, fd, BEV_OPT_CLOSE_ON_FREE);

	TCPServerTask *task = (TCPServerTask *)ctx;
		
	Client *cl = new Client(ctx);
	cl->SetEvent(bev);

	task->OnAccept(cl);


	bufferevent_setcb(bev, connection_read, NULL, connection_event, cl);
	bufferevent_enable(bev, EV_READ | EV_WRITE);
}

void connection_accept_error(struct evconnlistener *listener, void *ctx)
{
    	struct event_base *base = evconnlistener_get_base(listener);
	int err = EVUTIL_SOCKET_ERROR();
	fprintf(stderr, "Got an error %d (%s) on the listener. "
		"Shutting down.\n", err, evutil_socket_error_to_string(err));

	event_base_loopexit(base, NULL);
}

void TaskManager_Timeout(evutil_socket_t fd, short what, void *arg) //jyc20160718
{
	//TimerHandler *task = (TimerHandler *)arg;
	//task->OnEvent();

	//Packet pkt(11);	
	//uint8_t *enc = pkt.toHardware(buffer);
	
	/*if(boardhandler!=NULL){
		boardhandler->Send(buffer, enc - buffer);
	}*/
}

TaskManager::TaskManager(void)
:m_waitexit(false)
{

		this->evbase = event_base_new();
		
		struct timeval tv;  	
		tv.tv_sec = 5;
		tv.tv_usec = 0;

		this->timeout_event = event_new(this->evbase, -1, EV_PERSIST, TaskManager_Timeout, this);
		event_add(this->timeout_event, &tv);

}

TaskManager::~TaskManager(void)
{
	taskList.clear();
	//event_del(this->timeout_event);
	//event_free(this->timeout_event);
	event_base_free(this->evbase);	
}

void TaskManager::AddIOTask(IOTask *task)
{
	struct event *io_event = event_new(this->evbase, task->GetFD(), EV_READ | EV_PERSIST, onIOCallback, task);
	task->SetEvent(io_event);
	event_add(io_event, NULL);
}

void TaskManager::RemoveIOTask(IOTask *task)
{
	event_del(task->GetEvent());
	event_free(task->GetEvent());
}

void TaskManager::AddSocketTask(SocketTask *task)
{
	struct event *socket_event = event_new(this->evbase, task->GetSocket(), EV_READ | EV_PERSIST, Socket_Read, task);
	task->SetEvent(socket_event);
	event_add(socket_event, NULL);
}

void TaskManager::RemoveSocketTask(SocketTask *task)
{
	event_del(task->GetEvent());
	event_free(task->GetEvent());
}

void TaskManager::AddTCPServerTask(TCPServerTask *task)
{
	struct evconnlistener *listener;
	struct sockaddr_in sin;	
	//TCP Server
	memset(&sin, 0, sizeof(sin));
	/* This is an INET address */
	sin.sin_family = AF_INET;
	/* Listen on 0.0.0.0 */
	sin.sin_addr.s_addr = htonl(0);
	/* Listen on the given port. */
	sin.sin_port = htons(task->GetPort());

	listener = evconnlistener_new_bind(this->evbase, connection_accept, task,
		LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1,
		(struct sockaddr*)&sin, sizeof(sin));

	if (!listener) {
		perror("Couldn't create listener");
		return;
	}

	task->SetListener(listener);
	evconnlistener_set_error_cb(listener, connection_accept_error);
}

void TaskManager::RemoveTCPServerTask(TCPServerTask *task)
{
	evconnlistener_free(task->GetListener());
}

void TaskManager::AddTimer(TimerHandler *handler, uint32_t millisecond)
{
	struct timeval tv;
	struct event *evt;

    	if(millisecond > 1000){
	   tv.tv_sec = millisecond / 1000;
	   tv.tv_usec = 0;
	}else{
	   tv.tv_sec = 0;
	   tv.tv_usec = millisecond * 1000;
	}

	evt = event_new(this->evbase, -1, EV_PERSIST, TaskManager_OnTimer, handler);
	event_add(evt, &tv);

	handler->SetEvent(evt);
}

void TaskManager::RemoveTimer(TimerHandler *handler)
{
	struct event *evt = handler->GetEvent();
	event_del(evt);
	event_free(evt);
}

void TaskManager::Run()
{
	m_waitexit = true;
	event_base_loop(this->evbase, 0);
	m_waitexit = false;
}

void TaskManager::Close()
{
	event_base_loopexit(this->evbase, 0);

	uint32_t startTick = GetTickCount();
	while (m_waitexit && GetTickCount() - startTick < 30000)
	{
		usleep(20000);
	}
}
