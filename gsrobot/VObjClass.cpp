#include "VObjClass.h"


CVObj_common::CVObj_common(void)
{
}


CVObj_common::~CVObj_common(void)
{
}

Tag* CVObj_common::tag( const stru_vobj_config &cfg, const struTagParam &TagParam )
{
	Tag* i = new Tag( "vobj" );

	i->addAttribute( "type", (int)cfg.vobj_type );
	i->addAttribute( "id", (int)cfg.id );
	i->addAttribute( "en", (int)cfg.enable );
	i->addAttribute( "nm", ASCIIToUTF8(cfg.name) );

	return i;
}

defGSReturn CVObj_common::Untag( const Tag* tag, stru_vobj_config &cfg )
{
	if( tag->hasAttribute("type") )
	{
		cfg.vobj_type = (IOTDeviceType)atoi( tag->findAttribute("type").c_str() );

		if( IOT_VObj_scene != cfg.vobj_type )
		{
			return defGSReturn_UnSupport;
		}

		if( tag->hasAttribute("id") )
		{
			cfg.id = atoi( tag->findAttribute("id").c_str() );
		}
		else
		{
			cfg.id = 0;
		}

		if( tag->hasAttribute("en") )
		{
			cfg.enable = atoi(tag->findAttribute("en").c_str());
		}

		if( tag->hasAttribute("nm") )
		{
			cfg.name = UTF8ToASCII(tag->findAttribute("nm"));
		}

		return defGSReturn_Success;
	}

	return defGSReturn_ErrParam;
}

