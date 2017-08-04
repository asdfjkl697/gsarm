#ifndef CAMERAPTZ_H_
#define CAMERAPTZ_H_

#include "gloox/tag.h"

using namespace gloox;

class CameraPTZ
{
private:
	int m_command;
	int m_speed;
	int m_autoflag; // -2:自动快速完成; -1:自动默认完成; 0:不自动; >0:自定义自动持续间隔时间

public:
	CameraPTZ(int command, int speed);
	CameraPTZ( const Tag* tag = 0 );
	~CameraPTZ(void);

	int getCommand() const
	{
		return this->m_command;
	}
	
	int getSpeed() const
	{
		return this->m_speed;
	}

	int getAutoflag() const
	{
		return this->m_autoflag;
	}

	void untag( const Tag* tag);
	Tag* tag() const;
	virtual CameraPTZ* clone() const
	{
		return new CameraPTZ(*this);
	}
};

#endif

