#pragma once
#include <string>

//jyc20160824add
#include <time.h> 
#include "memmacro.h"
#include "common.h"

//typedef unsigned long DWORD;

enum defHBMonMsgType_
{
	defHBMonMsgType_Unknown = 0,
	defHBMonMsgType_OvertimeOneAlive = 0x113C55,
	defHBMonMsgType_OvertimeLimit = 0x995A99,
};

class IHeartbeatMonHandler
{
public:
	IHeartbeatMonHandler(void){};
	~IHeartbeatMonHandler(void){};

public:
	virtual void OnHeartbeatMonMessage( defHBMonMsgType_ MsgType, unsigned long Param ){};
};



/*
=======
心跳监测

=======
*/

// 初始化监测模块
bool hbmon_Init();

// 反初始化，销毁监测模块
bool hbmon_UnInit();

// 注册一个监测目标
bool hbmon_reg( const char *pname );

// 注销一个监测目标
bool hbmon_unreg( const char *pname );

// 设置回调通知
bool hbmon_SetHandler( IHeartbeatMonHandler *handler );

// 被监测目标调用此接口输入心跳，并检测是否有异常目标存在
bool hbmon_alive( const char *pname, bool printalive );

// 线程执行到那一步，调试用
bool hbmon_step( const char *pname, const char *pfunc, const int stepnum, const int linenum );

// 打印当前监测线程的情况
void hbmon_print();

// 监测对象实例类
class CHeartbeatGuard
{
public:
	CHeartbeatGuard( const char *pname, unsigned long hbtime=20*1000, unsigned long printtime=3*60*1000 );
	~CHeartbeatGuard(void);

	bool alive();
	bool step( const char *pfunc, const int stepnum, const int linenum )
	{
		return hbmon_step( m_name.c_str(), pfunc, stepnum, linenum );
	}

private:
	std::string m_name;

	unsigned long m_hbtime;
	unsigned long m_printtime;

	unsigned long m_lastalive;
	unsigned long m_lastprint;
};

#define macHeartbeatGuard_SpecStep( phbGuard, stepnum ) if( phbGuard ) { phbGuard->step(__FUNCTION__, stepnum, __LINE__); }
#define macHeartbeatGuard_step( stepnum ) macHeartbeatGuard_SpecStep( phbGuard, stepnum )
