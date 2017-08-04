#ifndef IPUBLISHHANDLER_H_
#define IPUBLISHHANDLER_H_

#include "typedef.h"

class IPublishHandler
{
public:
	virtual void OnPublishStart(){};
	virtual void OnPublishStop(defGSReturn code){};
	virtual uint32_t OnPublishUpdateSession( const std::string &strjid, bool *isAdded=NULL ){ return 0; };
};

#endif