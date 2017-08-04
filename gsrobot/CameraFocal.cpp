#include "CameraFocal.h"
#include "gloox/util.h"

CameraFocal::CameraFocal(int min,int max,int zoom)
	:m_max(max), m_min(min), m_zoom(zoom), m_autoflag(0)
{
}
CameraFocal::CameraFocal( const Tag* tag)
	:m_max(0), m_min(0), m_zoom(0), m_autoflag(0)
{
	untag( tag );
}

CameraFocal::~CameraFocal(void)
{
}

void CameraFocal::untag( const Tag* tag)
{
	if( !tag || tag->name() != "focal" )
		return;

	//控制只需一个参数
	if(tag->hasChild("zoom"))
		this->m_zoom = atoi(tag->findChild("zoom")->cdata().c_str());

	if(tag->hasChild("auto"))
	{
		this->m_autoflag = atoi(tag->findChild("auto")->cdata().c_str());
	}
}

Tag* CameraFocal::tag() const
{
	Tag* i = new Tag( "focal" );
	new Tag(i,"max", util::int2string(this->m_max));
	new Tag(i,"min", util::int2string(this->m_min));
	new Tag(i,"zoom", util::int2string(this->m_zoom));

	return i;
}