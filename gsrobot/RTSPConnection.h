#pragma once

//#include "liveMedia.hh"
//#include "FileSink.hh"
//#include "H264VideoBuffer.h"
#include "common.h"

class StreamClientState {
public:
  StreamClientState();
  virtual ~StreamClientState();

public:  //jyc20170323 remove
  //MediaSubsessionIterator* iter;
  //MediaSession* session;
  //MediaSubsession* subsession;
  //TaskToken streamTimerTask;
  double duration;
};

class RTSPConnection //:public RTSPClient  //jyc20170323 remove
{
public:
	//static RTSPConnection* createNew(UsageEnvironment& env, rtspcontrol *ctl,
		//		  int verbosityLevel = 0,
			//	  char const* applicationName = NULL,
				//  portNumBits tunnelOverHTTPPortNum = 0);

public:
	StreamClientState scs;
	//rtspcontrol *_ctl;

	void SetFileName(const char *fileName)
	{
		this->fileName = fileName;
	}

	void Start();

	const char *GetFileName()
	{
		return this->fileName;
	}
	
private:
	//UsageEnvironment* env;
	const char *fileName;

public:
	//RTSPConnection(UsageEnvironment& env, rtspcontrol *ctl,
		//int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum);

	~RTSPConnection(void);
};

