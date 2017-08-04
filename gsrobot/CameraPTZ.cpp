#include "CameraPTZ.h"
#include "gloox/util.h"

CameraPTZ::CameraPTZ(int command, int speed)
	:m_command(command), m_speed(speed), m_autoflag(0)
{
}

CameraPTZ::CameraPTZ( const Tag* tag)
{
	m_command = 0;
	m_speed = 0;
	m_autoflag = 0;
	
	untag( tag );
}

CameraPTZ::~CameraPTZ(void)
{
}

void CameraPTZ::untag( const Tag* tag)
{
	if( !tag || tag->name() != "ptz" )
		return;

	if(tag->hasChild("command"))
		this->m_command = atoi(tag->findChild("command")->cdata().c_str());

	if(tag->hasChild("speed"))
		this->m_speed = atoi(tag->findChild("speed")->cdata().c_str());

	if(tag->hasChild("auto"))
	{
		this->m_autoflag = atoi(tag->findChild("auto")->cdata().c_str());
	}
}

Tag* CameraPTZ::tag() const
{
	Tag* i = new Tag( "ptz" );
	if(this->m_command){
		new Tag(i,"command", util::int2string(this->m_command));
	}

	if(this->m_speed){
		new Tag(i,"speed", util::int2string(this->m_speed));
	}

	return i;
}