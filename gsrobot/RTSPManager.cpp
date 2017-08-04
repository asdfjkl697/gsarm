//#include "StdAfx.h"
#include "RTSPManager.h"
#include "RTSPConnection.h"
//#include "BasicUsageEnvironment.hh"  //jyc20170323 remove
//#include "DiskManager.h"

/*
bool CompareArray(CStringArray *list,CString str)
{
	for(int i=0; i<list->GetCount(); i++)
	{
		CString strTmp = list->ElementAt(i);
		if(strTmp.CompareNoCase(str)==0){
			return true;
		}
	}
	return false;
}


RTSPManager::RTSPManager(void)
{
}


RTSPManager::~RTSPManager(void)
{
}

bool RTSPManager::Start(rtspcontrol *ctl,CString recordPath)
{
    TaskScheduler* scheduler = BasicTaskScheduler::createNew();
    UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);

	RTSPConnection* rtspClient = RTSPConnection::createNew(*env, ctl,0,"GS_RTSP_CLIENT");
    if (rtspClient == NULL) {
	    return FALSE;
	}

	CString fileName = L"video";
	const char *recordFullPath = DiskManager::GetFileName(recordPath,fileName);
	rtspClient->SetFileName(recordFullPath);
	rtspClient->Start();
	//Is 24hours and never sleep and never stop
	env->taskScheduler().doEventLoop();
	return TRUE;
}
*/
