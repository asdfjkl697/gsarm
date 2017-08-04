#pragma once
#include "common.h"
#include "gloox/tag.h"

using namespace gloox;

class CVObj_common
{
public:
	CVObj_common(void);
	~CVObj_common(void);

	static Tag* tag( const stru_vobj_config &cfg, const struTagParam &TagParam );
	static defGSReturn Untag( const Tag* tag, stru_vobj_config &cfg );
};

