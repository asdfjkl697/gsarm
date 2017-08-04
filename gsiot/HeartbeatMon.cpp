/*
#include "HeartbeatMon.h"
#include <windows.h>
#include <Mmsystem.h>
#include <map>
#include "mutexguard.h"
#include "logFileExFunc.h"
#include "..\\libiot\\RunCode.h"
*/

#include "HeartbeatMon.h"
#include <map>
#include "gloox/mutexguard.h"
#include "RunCode.h"


class CHeartbeatMon
{
public:
	CHeartbeatMon(void);
	~CHeartbeatMon(void);

	struct struHbMon
	{
		DWORD alive;
		std::string func;
		int step;
		int line;

		struHbMon()
			: alive(0), step(0), line(0)
		{
		}

		struHbMon( DWORD in_alive, const char *pfunc=NULL, int in_step=0, int in_line=0 )
			: alive(in_alive), step(in_step), line(in_line)
		{
			if( pfunc )
			{
				func = pfunc;
			}
		}
	};
	typedef std::map<std::string,struHbMon> defmapHBMap;

	bool SetHandler( IHeartbeatMonHandler *handler );

	bool reg( const char *pname );
	bool unreg( const char *pname );

	bool alive( const char *pname, bool printalive );
	bool step( const char *pname, const char *pfunc, const int stepnum, const int linenum );
	void print();

private:
	IHeartbeatMonHandler *m_Handler;

	DWORD m_lastcheck;
	gloox::util::Mutex m_mutex_hb;
	defmapHBMap m_hbmap;
};

CHeartbeatMon::CHeartbeatMon(void)
{
	m_Handler = nullptr;
	m_lastcheck = timeGetTime();
}

CHeartbeatMon::~CHeartbeatMon(void)
{
}

bool CHeartbeatMon::SetHandler( IHeartbeatMonHandler *handler )
{
	m_Handler = handler;
	return true;
}

bool CHeartbeatMon::reg( const char *pname )
{
	if( !pname )
	{
		return false;
	}

	gloox::util::MutexGuard mutexguard( this->m_mutex_hb );

	//LOGMSGEX( defLOGNAME, defLOG_INFO, "HBMon reg: \"%s\"%s", pname, (m_hbmap.find(pname)!=m_hbmap.end()) ? " is exists already!!!":"" );
	m_hbmap[pname] = struHbMon(timeGetTime());
	return true;
}

bool CHeartbeatMon::unreg( const char *pname )
{
	if( !pname )
	{
		return false;
	}

	gloox::util::MutexGuard mutexguard( this->m_mutex_hb );

	defmapHBMap::iterator it = m_hbmap.find(pname);
	if( it != m_hbmap.end() )
	{
		//LOGMSGEX( defLOGNAME, defLOG_INFO, "HBMon unreg: \"%s\"", pname );
		m_hbmap.erase(it);
	}

	return true;
}

bool CHeartbeatMon::alive( const char *pname, bool printalive )
{
	this->m_mutex_hb.lock();
	
	bool retalive = true;

	if( pname )
	{
		defmapHBMap::iterator it = m_hbmap.find(pname);
		if( it != m_hbmap.end() )
		{
			it->second.alive = timeGetTime();
			if( printalive )
			{
				//LOGMSGEX( defLOGNAME, defLOG_INFO, "HBMon alive: \"%s\"", pname );
			}
		}
	}

	// 每间隔检测
	if( timeGetTime()-m_lastcheck > 60000 )
	{
		m_lastcheck = timeGetTime();
		//LOGMSGEX( defLOGNAME, defLOG_INFO, "HBMon check" );

		for( defmapHBMap::const_iterator it = m_hbmap.begin(); it != m_hbmap.end(); ++it )
		{
			const int overtime = (timeGetTime() - it->second.alive) / 1000; // 秒

			// 某个时间就开始打印
			if( overtime > 180 )
			{
				retalive  = false;
				//LOGMSGEX( defLOGNAME, defLOG_WORN, "HBMon check: overtime %dm %ds, func=%s, step=%d, line=%d \"%s\"", overtime/60, overtime%60, it->second.func.empty()?"":it->second.func.c_str(), it->second.step, it->second.line, it->first.c_str() );
			}

			// 超过最大时间重启系统
			if( overtime >= (5*60) )
			{
				//LOGMSGEX( defLOGNAME, defLOG_INFO, "HBMon check: overtime limit func=%s, step=%d, line=%d \"%s\"!!!", it->second.func.empty()?"":it->second.func.c_str(), it->second.step, it->second.line, it->first.c_str() );
				if( m_Handler )
				{
					m_Handler->OnHeartbeatMonMessage( defHBMonMsgType_OvertimeLimit, defHBMonMsgType_OvertimeLimit );
					break;
				}
			}
		}
	}

	this->m_mutex_hb.unlock();

	if( !retalive )
	{
		m_Handler->OnHeartbeatMonMessage( defHBMonMsgType_OvertimeOneAlive, 0 );
	}
	
	return retalive;
}

bool CHeartbeatMon::step( const char *pname, const char *pfunc, const int stepnum, const int linenum )
{
	if( !pname )
	{
		return false;
	}

	gloox::util::MutexGuard mutexguard( this->m_mutex_hb );

	defmapHBMap::iterator it = m_hbmap.find(pname);
	if( it != m_hbmap.end() )
	{
		if( pfunc )
		{
			it->second.func = pfunc;
		}

		it->second.step = stepnum;
		it->second.line = linenum;

		return true;
	}

	return false;
}

void CHeartbeatMon::print()
{
	gloox::util::MutexGuard mutexguard( this->m_mutex_hb );
	
	const size_t count = m_hbmap.size();

	//LOGMSGEX( defLOGNAME, defLOG_INFO, "HBMon print(count=%u)", count );

	int i=1;
	for( defmapHBMap::const_iterator it = m_hbmap.begin(); it != m_hbmap.end(); ++it, ++i )
	{
		const int alivetime = (timeGetTime() - it->second.alive) / 1000; // 秒

		//LOGMSGEX( defLOGNAME, defLOG_INFO, "%sHBMon print(%d/%u)(%s): alivetime=%ds, func=%s, step=%d, line=%d", alivetime>6?">>>>>":"", i, count, it->first.c_str(), alivetime, it->second.func.empty()?"":it->second.func.c_str(), it->second.step, it->second.line );
	}
}


//=========================================================

static CHeartbeatMon *g_hbmon = nullptr;

bool hbmon_Init()
{
	if( IsRUNCODEEnable(defCodeIndex_TEST_DisableHeartbeatMon) )
	{
		//LOGMSGEX( defLOGNAME, defLOG_INFO, "HBMon Disabled!" );

		return false;
	}

	if( !g_hbmon )
	{
		//LOGMSGEX( defLOGNAME, defLOG_INFO, "HBMon Init" );

		g_hbmon = new CHeartbeatMon();
	}

	return (g_hbmon?true:false);
}

bool hbmon_UnInit()
{
	if( g_hbmon )
	{
		//LOGMSGEX( defLOGNAME, defLOG_INFO, "HBMon UnInit" );

		delete g_hbmon;
		g_hbmon = nullptr;
	}

	return true;
}

bool hbmon_reg( const char *pname )
{
	if( g_hbmon )
	{
		return g_hbmon->reg(pname);
	}

	return false;
}

bool hbmon_unreg( const char *pname )
{
	if( g_hbmon )
	{
		return g_hbmon->unreg(pname);
	}

	return false;
}

bool hbmon_SetHandler( IHeartbeatMonHandler *handler )
{
	if( g_hbmon )
	{
		return g_hbmon->SetHandler(handler);
	}

	return false;
}

bool hbmon_alive( const char *pname, bool printalive )
{
	if( g_hbmon )
	{
		return g_hbmon->alive(pname, printalive);
	}

	return false;
}

bool hbmon_step( const char *pname, const char *pfunc, const int stepnum, const int linenum )
{
	if( g_hbmon )
	{
		return g_hbmon->step(pname, pfunc, stepnum, linenum);
	}

	return false;
}

void hbmon_print()
{
	if( g_hbmon )
	{
		g_hbmon->print();
	}
}

//=========================================================

CHeartbeatGuard::CHeartbeatGuard( const char *pname, unsigned long hbtime, unsigned long printtime )
{
	m_name = pname;

	m_hbtime = hbtime;
	m_printtime = printtime;

	m_lastalive = timeGetTime();
	m_lastprint = m_lastalive;

	hbmon_reg(pname);
}

CHeartbeatGuard::~CHeartbeatGuard(void)
{
}

// 达到心跳间隔时才产生心跳
bool CHeartbeatGuard::alive()
{
	if( timeGetTime()-m_lastalive >= m_hbtime )
	{
		m_lastalive = timeGetTime();

		bool doprint = false;
		if( timeGetTime()-m_lastprint >= m_printtime )
		{
			m_lastprint = timeGetTime();
			doprint = true;
		}

		return hbmon_alive( m_name.c_str(), doprint );
	}
	return true;
}
