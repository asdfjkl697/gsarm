#pragma once
#include <string>

//jyc20160824 add
#include <time.h> 
#include "memmacro.h"
#include "common.h"

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
������

=======
*/

// ��ʼ�����ģ��
bool hbmon_Init();

// ����ʼ������ټ��ģ��
bool hbmon_UnInit();

// ע��һ�����Ŀ��
bool hbmon_reg( const char *pname );

// ע��һ�����Ŀ��
bool hbmon_unreg( const char *pname );

// ���ûص�֪ͨ
bool hbmon_SetHandler( IHeartbeatMonHandler *handler );

// �����Ŀ����ô˽ӿ�������������Ƿ����쳣Ŀ�����
bool hbmon_alive( const char *pname, bool printalive );

// �߳�ִ�е���һ����������
bool hbmon_step( const char *pname, const char *pfunc, const int stepnum, const int linenum );

// ��ӡ��ǰ����̵߳����
void hbmon_print();

// ������ʵ����
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
