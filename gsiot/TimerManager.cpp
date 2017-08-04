/*
#include "TimerManager.h"
#include <Windows.h>
#include <process.h>
#include "HeartbeatMon.h"
*/

#include "TimerManager.h"
#include "pthread.h"
#include "HeartbeatMon.h"

unsigned __stdcall TimerThread(LPVOID lpPara)
{
	TimerManager *tManager = (TimerManager *)lpPara;
	tManager->m_isThreadExit = false;
	//CoInitialize(NULL);
	
	CHeartbeatGuard hbGuard( "TimerMgr" );

	while(tManager->IsRunning()){
		hbGuard.alive();
		sleep(1);
		tManager->OnThreadCall();
	}

	tManager->m_isThreadExit = true;
	//CoUninitialize();
	return 0;
}

TimerManager::TimerManager(void)
	:isRunning(false), m_isThreadExit(true)
{
}


TimerManager::~TimerManager(void)
{
	DWORD dwStart = ::timeGetTime();
	while( !m_isThreadExit && ::timeGetTime()-dwStart < 3000 )
	{
		usleep(1000);
	}

	timerQueue.clear();
}

//void *thread_1(void *arg)  
void *Timer_Thread(LPVOID lpPara) 
{
	/*
    int i=0;  
    for(i=0;i<=6;i++)  
    {  
        printf("This is a pthread_1.\n");  
        if(i==2)  
            pthread_exit(0);  
        sleep(1);  
    } */ 

	TimerManager *tManager = (TimerManager *)lpPara;
	tManager->m_isThreadExit = false;
	//CoInitialize(NULL);
	
	CHeartbeatGuard hbGuard( "TimerMgr" );

	while(tManager->IsRunning()){
		hbGuard.alive();
		//pthread_exit(0); //jyc20160826
		sleep(1);
		tManager->OnThreadCall();
	}
	printf("pthread test...\n");
	tManager->m_isThreadExit = true;
	//CoUninitialize();
	//return 0; //jyc20160826
}  

void TimerManager::Run()
{
	/*
	HANDLE   hth1;
    unsigned  uiThread1ID;

	if(!this->isRunning){
		this->isRunning = true;
		hth1 = (HANDLE)_beginthreadex(NULL, 0, TimerThread, this, 0, &uiThread1ID);
		CloseHandle(hth1);
	}*/

    pthread_t id_1;  
    int i,ret;  
	this->isRunning = true; //jyc20160826 add just test

    ret=pthread_create(&id_1,NULL,Timer_Thread,this);  
    if(ret!=0)  
    {  
        printf("Create timemanager_pthread error!\n");  
		//return -1;  
		return; 
    }  
}

void TimerManager::OnThreadCall()
{	
	if( !IsRunning() )
	{
		return;
	}

	std::list<TIMER_ST>::iterator it = timerQueue.begin();
	for(;it!=timerQueue.end();it++){

		if( !IsRunning() )
		{
			break;
		}

		if(it->expired >= it->second){
			it->expired = 0;
			it->handler->OnTimer(it->TimerID);
		}else{
			it->expired++;
		}
	}
}

void TimerManager::registerTimer(ITimerHandler *handler, int TimerID, int second)
{
	if(handler){
		TIMER_ST t = {0};
		t.TimerID = TimerID;
		t.expired = 0;
		t.second = second;
		t.handler = handler;
		this->timerQueue.push_back(t);

		if(!this->isRunning){
			this->Run();
		}
	}
}


void TimerManager::removeTimer(ITimerHandler *handler, int TimerID)
{
	if(handler){
		//std::list<TIMER_ST>::const_iterator it = timerQueue.begin();
		std::list<TIMER_ST>::iterator it = timerQueue.begin();
		for(;it!=timerQueue.end();it++){
			if(handler == (it)->handler && TimerID == (it)->TimerID){
				this->timerQueue.erase(it);
				break;
			}
		}
		//¹Ø±Õ¶¨Ê±Æ÷
		if(timerQueue.size()==0){
			this->isRunning = false;
		}
	}
}

void TimerManager::Stop()
{
	this->isRunning = false;
}
