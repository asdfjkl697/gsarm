#ifndef IRFREADHANDLER_H_
#define IRFREADHANDLER_H_

#include "typedef.h"
#include "RFSignalDefine.h"

class IRFReadHandler
{
public:
	virtual void OnRead(const defLinkID LinkID, const RFSignal& signal, const bool isAdd, const std::string strAddedName){};
};

#endif