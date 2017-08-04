#pragma once
#include "typedef.h"

class GSIOTObjBase
{
public:
	GSIOTObjBase(){};
	virtual ~GSIOTObjBase(void){};
	virtual const std::string GetObjName() const = 0;
	virtual GSIOTObjType GetObjType() const = 0;
	virtual int GetId() const = 0;
	virtual GSIOTObjBase* cloneObj() const = 0;
	virtual std::string Print( bool doPrint=true ) const { return std::string(""); };
};
