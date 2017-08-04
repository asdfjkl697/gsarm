#include "DataStoreMgr.h"
#include "common.h"
//#include <windows.h>
#include <time.h>
#include "gloox/util.h"
#include <bitset>

using namespace gloox;

//#define defTEST_UseTestChangeDb

bool operator< ( const struRec_stat_day::struKey &key1, const struRec_stat_day::struKey &key2 )
{
	macOperator_Less( key1, key2, AddrObjKey.dev_type );
	macOperator_Less( key1, key2, AddrObjKey.dev_id );
	macOperator_Less( key1, key2, AddrObjKey.address_type );
	macOperator_Less( key1, key2, AddrObjKey.address_id );
	macOperator_Less( key1, key2, dt_year );
	macOperator_Less( key1, key2, dt_month );

	return (key1.dt_day < key2.dt_day);
}

bool operator< ( const struRec_stat_month::struKey &key1, const struRec_stat_month::struKey &key2 )
{
	macOperator_Less( key1, key2, AddrObjKey.dev_type );
	macOperator_Less( key1, key2, AddrObjKey.dev_id );
	macOperator_Less( key1, key2, AddrObjKey.address_type );
	macOperator_Less( key1, key2, AddrObjKey.address_id );
	macOperator_Less( key1, key2, dt_year );

	return (key1.dt_month < key2.dt_month);
}

// 是否属于数据库记录不存在的错误码归类
bool g_isNoDBRec( const defGSReturn ret )
{
	switch(ret)
	{
	case defGSReturn_DBNoExist:
	case defGSReturn_DBNoRec:
		return true;
	}

	return false;
}

// 分析统计最大最小
void g_AnalyseStat( struStat &Stat, const time_t curTime, const int curValue, const time_t data_dt_begin, const time_t data_dt_end )
{
	if( data_dt_begin && data_dt_end )
	{
		if( curTime < data_dt_begin || curTime > data_dt_end )
		{
			return;
		}
	}

	// 最大值比较
	if( 0==Stat.stat_v1k_max_dt || curValue > Stat.stat_v1k_max )
	{
		Stat.stat_v1k_max_dt = curTime;
		Stat.stat_v1k_max = curValue;
	}

	// 最小值比较
	if( 0==Stat.stat_v1k_min_dt || curValue < Stat.stat_v1k_min )
	{
		Stat.stat_v1k_min_dt = curTime;
		Stat.stat_v1k_min = curValue;
	}

	if( !Stat.stat_valid )
	{
		Stat.stat_valid = 1;
	}
}

// 分析统计最大最小
void g_AnalyseStat( struStat &Stat, const struStat &substat, const time_t data_dt_begin, const time_t data_dt_end )
{
	if( data_dt_begin && data_dt_end )
	{
		if( Stat.stat_v1k_max_dt < data_dt_begin || Stat.stat_v1k_max_dt > data_dt_end )
		{
			return;
		}

		if( Stat.stat_v1k_min_dt < data_dt_begin || Stat.stat_v1k_min_dt > data_dt_end )
		{
			return;
		}
	}

	if( !substat.stat_valid )
		return;

	// 最大值比较
	if( !Stat.stat_valid || substat.stat_v1k_max > Stat.stat_v1k_max )
	{
		Stat.stat_v1k_max_dt = substat.stat_v1k_max_dt;
		Stat.stat_v1k_max = substat.stat_v1k_max;
	}

	// 最小值比较
	if( !Stat.stat_valid || substat.stat_v1k_min < Stat.stat_v1k_min )
	{
		Stat.stat_v1k_min_dt = substat.stat_v1k_min_dt;
		Stat.stat_v1k_min = substat.stat_v1k_min;
	}

	if( !Stat.stat_valid )
	{
		Stat.stat_valid = 1;
	}
}

// lst_v1k中指定时间data_dt左边最近的值
bool g_getLeftV1k( const time_t data_dt, const defmapV1k &lst_v1k, stru_V1K &get_v1k, const time_t dt_valid_range )
{
	get_v1k.reset();

	if( lst_v1k.empty() )
		return false;

	defmapV1k::const_iterator it = lst_v1k.upper_bound( data_dt );	// 大于data_dt

	if( it==lst_v1k.begin() )
	{
		return false;
	}

	--it;

	if( dt_valid_range )
	{
		// 得到的值比左边时间范围更早，则超出有效取值范围
		if( it->first < (data_dt-dt_valid_range) )
		{
			return false;
		}
	}

	get_v1k = it->second;
	return true;
}

// lst_v1k中指定时间data_dt右边最近的值
// dt_valid_range有效范围内
bool g_getRightV1k( const time_t data_dt, const defmapV1k &lst_v1k, stru_V1K &get_v1k, const time_t dt_valid_range )
{
	get_v1k.reset();

	if( lst_v1k.empty() )
		return false;

	defmapV1k::const_iterator it = lst_v1k.lower_bound( data_dt );	// 不小于data_dt，即>=data_dt

	if( it==lst_v1k.end() )
	{
		return false;
	}

	if( dt_valid_range )
	{
		// 得到的值比右边时间范围更晚，则超出有效取值范围
		if( it->first > (data_dt+dt_valid_range) )
		{
			return false;
		}
	}

	get_v1k = it->second;
	return true;
}

// 日统计是否已存在
bool g_is_stat_day_spec( const defmap_is_stat_day &lst_is_stat_day, unsigned char day )
{
	return ( lst_is_stat_day.find(day) != lst_is_stat_day.end() );
}

void g_delete_vecDataSave( defvecDataSave &vecDataSave )
{
	if( vecDataSave.empty() )
		return;

	for( defvecDataSave::const_iterator it=vecDataSave.begin(); it!=vecDataSave.end(); ++it )
	{
		delete (*it);
	}

	vecDataSave.clear();
}

void g_delete_mapRec_stat_day( defmapRec_stat_day &mapRec_stat_day )
{
	if( mapRec_stat_day.empty() )
		return;

	for( defmapRec_stat_day::const_iterator it=mapRec_stat_day.begin(); it!=mapRec_stat_day.end(); ++it )
	{
		delete it->second;
	}

	mapRec_stat_day.clear();
}

void g_delete_mapRec_stat_month( defmapRec_stat_month &mapRec_stat_month )
{
	if( mapRec_stat_month.empty() )
		return;

	for( defmapRec_stat_month::const_iterator it=mapRec_stat_month.begin(); it!=mapRec_stat_month.end(); ++it )
	{
		delete it->second;
	}

	mapRec_stat_month.clear();
}

void g_Print_lst_v1k( const defmapV1k &lst_v1k, const char *pinfo=NULL )
{
	const int v1kcount = lst_v1k.size();

	LOGMSG( "%s count=%d", pinfo?pinfo:"Print_lst_v1k", v1kcount );

	int i=1;
	for( defmapV1k::const_iterator it=lst_v1k.begin(); it!=lst_v1k.end(); ++it, ++i )
	{
		LOGMSG( "%s(%d/%d) \"%s\" v=%d", pinfo?pinfo:"Print_lst_v1k", i, v1kcount, g_TimeToStr(it->first).c_str(), it->second.v1k );
	}
}

std::string struRec_stat_day::GetBaseInfo() const
{
	char buf[256] = {0};
	snprintf( buf, sizeof(buf), "%s, time(%04d-%02d-%02d)", key.AddrObjKey.get_str().c_str(), key.dt_year, key.dt_month, key.dt_day );

	return std::string(buf);
}

void struRec_stat_day::Print( const bool print_lst_src_v1k ) const
{
	LOGMSG( "Print_Rec_stat_day %s", GetBaseInfo().c_str() );

	if( Stat.stat_valid  )
	{
		LOGMSG( "Print_Rec_stat_day valid=%d, avg(n=%d,v=%d)", Stat.stat_valid, Stat.stat_v1k_avg_num, Stat.stat_v1k_avg );
		LOGMSG( "Print_Rec_stat_day max(%s,%d), min(%s,%d)\r\n", g_TimeToStr(Stat.stat_v1k_max_dt).c_str(), Stat.stat_v1k_max, g_TimeToStr(Stat.stat_v1k_min_dt).c_str(), Stat.stat_v1k_min );
	}
	else
	{
		LOGMSG( "Print_Rec_stat_day valid=false" );
	}

	if( !print_lst_src_v1k )
		return;

	// 数据记录
	const time_t day_begin = g_struGSTime_To_UTCTime( struGSTime( key.dt_year, key.dt_month, key.dt_day ) );

	const std::bitset<24> bit_valid_A(v1k_valid_00_11A);
	const std::bitset<24> bit_valid_B(v1k_valid_12_23B);

	for( int i=0; i<48; ++i )
	{
		const time_t curv_time = day_begin + i*30*60;

		const bool v1k_valid = ( i<24 ) ? bit_valid_A.test( i ) : bit_valid_B.test( i-24 );

		LOGMSG( "Print_Rec_stat_day(%d/48) \"%s\" valid=%d, v=%d", i+1, g_TimeToStr(curv_time,defTimeToStrFmt_HourMin).c_str(), (int)v1k_valid, v1k[i] );
	}
}

std::string struRec_stat_month::GetBaseInfo() const
{
	char buf[256] = {0};
	snprintf( buf, sizeof(buf), "%s, time(%04d-%02d)", key.AddrObjKey.get_str().c_str(), key.dt_year, key.dt_month );

	return std::string(buf);
}

void struRec_stat_month::Print() const
{
	LOGMSG( "Print_Rec_stat_month %s", GetBaseInfo().c_str() );

	if( Stat.stat_valid  )
	{
		LOGMSG( "Print_Rec_stat_month valid=%d, avg(n=%d,v=%d)", Stat.stat_valid, Stat.stat_v1k_avg_num, Stat.stat_v1k_avg );
		LOGMSG( "Print_Rec_stat_month max(%s,%d), min(%s,%d)\r\n", g_TimeToStr(Stat.stat_v1k_max_dt).c_str(), Stat.stat_v1k_max, g_TimeToStr(Stat.stat_v1k_min_dt).c_str(), Stat.stat_v1k_min );
	}
	else
	{
		LOGMSG( "Print_Rec_stat_month valid=false" );
	}
}

std::string stru_QueryStat::GetBaseInfo() const
{
	char buf[256] = {0};
	snprintf( buf, sizeof(buf), "%s, time(%s)-(%s)", AddrObjKey.get_str().c_str(), g_TimeToStr(data_dt_begin).c_str(), g_TimeToStr(data_dt_end).c_str() );

	return std::string(buf);
}

void stru_QueryStat::Print() const
{
	const int v1kcount = lst_query_v1k.size();
	const uint32_t tm_begin = v1kcount>0 ? lst_query_v1k.begin()->second.data_dt:0;

	LOGMSG( "Print_QueryStat %s (tm_begin=%d)", GetBaseInfo().c_str(), tm_begin );

	if( Stat.stat_valid  )
	{
		LOGMSG( "Print_QueryStat valid=%d, v1kcount=%d, avg(n=%d,v=%d)", Stat.stat_valid, v1kcount, Stat.stat_v1k_avg_num, Stat.stat_v1k_avg );
		LOGMSG( "Print_QueryStat max(%s,%d), min(%s,%d)\r\n", g_TimeToStr(Stat.stat_v1k_max_dt).c_str(), Stat.stat_v1k_max, g_TimeToStr(Stat.stat_v1k_min_dt).c_str(), Stat.stat_v1k_min );
	}
	else
	{
		LOGMSG( "Print_QueryStat valid=false, v1kcount=%d", v1kcount );
	}

	int i=1;
	for( defmapV1k::const_iterator it=lst_query_v1k.begin(); it!=lst_query_v1k.end(); ++it, ++i )
	{
		LOGMSG( "Print_QueryStat(%d/%d) (%d)\"%s\" valid=%d, v=%d", i, (int)v1kcount, (uint32_t)it->second.data_dt-tm_begin, g_TimeToStr(it->second.data_dt).c_str(), (int)it->second.v1k_valid, it->second.v1k );
	}
}

void stru_QueryStat::AddStatToList()
{
	lst_query_v1k[Stat.stat_v1k_max_dt] = stru_QueryV1K( Stat.stat_v1k_max_dt, Stat.stat_valid?defDataFlag_Norm:defDataFlag_Invalid, Stat.stat_v1k_max );
	lst_query_v1k[Stat.stat_v1k_min_dt] = stru_QueryV1K( Stat.stat_v1k_min_dt, Stat.stat_valid?defDataFlag_Norm:defDataFlag_Invalid, Stat.stat_v1k_min );
}


//=====================================================
// DBSave

SQLite_DBDataSave::SQLite_DBDataSave( bool isReadOnly, const std::string &dbname, const struGSTime &dt, bool DoAutoCreate )
	: db(NULL), m_isReadOnly(isReadOnly), m_dtDBSave(dt), m_is_stat_month(false),
	m_usecount(0), m_lastUseTs(0)
{
	const DWORD dwstart = timeGetTime();
	//jyc20170227 delete
	//const std::string strsub_full = g_iotstore_createdir( defIotStoreDir_Data );

	m_dbname = dbname;
	//m_path = strsub_full + "\\" + m_dbname;
	m_path = ROOTDIR + m_dbname;

	try
	{
		db = new SQLite::Database( m_path.c_str(), m_isReadOnly ? SQLITE_OPEN_READONLY : (DoAutoCreate?SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE:SQLITE_OPEN_READWRITE) );

		UseDbTransAction dbta( db );

		db->exec( "CREATE TABLE if not exists [data_db_ver] ( [id] INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL, [val] INTEGER  NULL, [name] VARCHAR(64) NULL, [strval] VARCHAR(64) NULL )" );

		SQLite::Statement query( *db, "SELECT val FROM data_db_ver limit 1" );
		if( !query.executeStep() )
		{
			db->exec( "INSERT INTO data_db_ver(val,name,strval) VALUES(15082600,'db_ver_main','')" );

			time_t curtime = time(NULL);

			db->exec( (std::string("INSERT INTO data_db_ver(val,name,strval) VALUES(0,'dbname','")+ m_dbname +"')").c_str() );
			db->exec( (std::string("INSERT INTO data_db_ver(val,name,strval) VALUES("+util::int2string(curtime)+",'create','")+ g_TimeToStr(curtime) +"')").c_str() );

			db->exec( "INSERT INTO data_db_ver(val,name,strval) VALUES(15082600,'db_ver_stat_day','')" );
			db->exec( "INSERT INTO data_db_ver(val,name,strval) VALUES(15082600,'db_ver_stat_month','')" );
		}

		// 历史数据
#if 1
		db->exec( "CREATE TABLE if not exists [histdata] ( \r\n"\
			"[id] INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL, \r\n"\
			"[data_dt] INTEGER  NULL, \r\n"\
			"[dev_type] INTEGER  NULL, \r\n"\
			"[dev_id] INTEGER  NULL, \r\n"\
			"[address_type] INTEGER  NULL, \r\n"\
			"[address_id] INTEGER  NULL, \r\n"\
			"[data_dt_str] VARCHAR(25)  NULL, \r\n"\
			"[dataflag] INTEGER  NULL, \r\n"\
			"[strvalue] VARCHAR(25)  NULL, \r\n"\
			"[value1k] INTEGER  NULL)" );
#else
		db->exec( "CREATE TABLE if not exists [histdata] ( \r\n"\
			"[id] INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL, \r\n"\
			"[data_dt] INTEGER  NULL, \r\n"\
			"[dev_type] INTEGER  NULL, \r\n"\
			"[dev_id] INTEGER  NULL, \r\n"\
			"[address_type] INTEGER  NULL, \r\n"\
			"[address_id] INTEGER  NULL, \r\n"\
			"[data_dt_str] VARCHAR(25)  NULL, \r\n"\
			"[dataflag] INTEGER  NULL, \r\n"\
			"[strvalue] VARCHAR(25)  NULL, \r\n"\
			"[value1k] INTEGER  NULL, \r\n"\
			"[name] VARCHAR(255)  NULL)" );
#endif

		// 日统计
		db->exec( "CREATE TABLE if not exists [stat_day] ( \r\n"\
			"[id] INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL, \r\n"\
			"[dt_day] INTEGER  NULL, \r\n"\
			"[dev_type] INTEGER  NULL, \r\n"\
			"[dev_id] INTEGER  NULL, \r\n"\
			"[address_type] INTEGER  NULL, \r\n"\
			"[address_id] INTEGER  NULL, \r\n"\

			// 统计
			"[stat_valid] INTEGER  NULL, \r\n"\
			"[stat_v1k_max_dt] INTEGER  NULL, \r\n"\
			"[stat_v1k_max] INTEGER  NULL, \r\n"\
			"[stat_v1k_min_dt] INTEGER  NULL, \r\n"\
			"[stat_v1k_min] INTEGER  NULL, \r\n"\
			"[stat_v1k_avg] INTEGER  NULL, \r\n"\
			"[stat_v1k_avg_num] INTEGER  NULL, \r\n"\

			// 48个整点和半点值 A的24位前24个值，B的24位后24个值，1有效0无效
			"[v1k_valid_00_11A] INTEGER  NULL, \r\n"\
			"[v1k_valid_12_23B] INTEGER  NULL, \r\n"\

			"[v1k_0000] INTEGER  NULL, \r\n"\
			"[v1k_0030] INTEGER  NULL, \r\n"\
			"[v1k_0100] INTEGER  NULL, \r\n"\
			"[v1k_0130] INTEGER  NULL, \r\n"\
			"[v1k_0200] INTEGER  NULL, \r\n"\
			"[v1k_0230] INTEGER  NULL, \r\n"\
			"[v1k_0300] INTEGER  NULL, \r\n"\
			"[v1k_0330] INTEGER  NULL, \r\n"\
			"[v1k_0400] INTEGER  NULL, \r\n"\
			"[v1k_0430] INTEGER  NULL, \r\n"\
			"[v1k_0500] INTEGER  NULL, \r\n"\
			"[v1k_0530] INTEGER  NULL, \r\n"\

			"[v1k_0600] INTEGER  NULL, \r\n"\
			"[v1k_0630] INTEGER  NULL, \r\n"\
			"[v1k_0700] INTEGER  NULL, \r\n"\
			"[v1k_0730] INTEGER  NULL, \r\n"\
			"[v1k_0800] INTEGER  NULL, \r\n"\
			"[v1k_0830] INTEGER  NULL, \r\n"\
			"[v1k_0900] INTEGER  NULL, \r\n"\
			"[v1k_0930] INTEGER  NULL, \r\n"\
			"[v1k_1000] INTEGER  NULL, \r\n"\
			"[v1k_1030] INTEGER  NULL, \r\n"\
			"[v1k_1100] INTEGER  NULL, \r\n"\
			"[v1k_1130] INTEGER  NULL, \r\n"\

			"[v1k_1200] INTEGER  NULL, \r\n"\
			"[v1k_1230] INTEGER  NULL, \r\n"\
			"[v1k_1300] INTEGER  NULL, \r\n"\
			"[v1k_1330] INTEGER  NULL, \r\n"\
			"[v1k_1400] INTEGER  NULL, \r\n"\
			"[v1k_1430] INTEGER  NULL, \r\n"\
			"[v1k_1500] INTEGER  NULL, \r\n"\
			"[v1k_1530] INTEGER  NULL, \r\n"\
			"[v1k_1600] INTEGER  NULL, \r\n"\
			"[v1k_1630] INTEGER  NULL, \r\n"\
			"[v1k_1700] INTEGER  NULL, \r\n"\
			"[v1k_1730] INTEGER  NULL, \r\n"\

			"[v1k_1800] INTEGER  NULL, \r\n"\
			"[v1k_1830] INTEGER  NULL, \r\n"\
			"[v1k_1900] INTEGER  NULL, \r\n"\
			"[v1k_1930] INTEGER  NULL, \r\n"\
			"[v1k_2000] INTEGER  NULL, \r\n"\
			"[v1k_2030] INTEGER  NULL, \r\n"\
			"[v1k_2100] INTEGER  NULL, \r\n"\
			"[v1k_2130] INTEGER  NULL, \r\n"\
			"[v1k_2200] INTEGER  NULL, \r\n"\
			"[v1k_2230] INTEGER  NULL, \r\n"\
			"[v1k_2300] INTEGER  NULL, \r\n"\
			"[v1k_2330] INTEGER  NULL)" );

		// 月统计
		db->exec( "CREATE TABLE if not exists [stat_month] ( \r\n"\
			"[id] INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL, \r\n"\
			
			"[dev_type] INTEGER  NULL, \r\n"\
			"[dev_id] INTEGER  NULL, \r\n"\
			"[address_type] INTEGER  NULL, \r\n"\
			"[address_id] INTEGER  NULL, \r\n"\

			// 统计
			"[stat_valid] INTEGER  NULL, \r\n"\
			"[stat_v1k_max_dt] INTEGER  NULL, \r\n"\
			"[stat_v1k_max] INTEGER  NULL, \r\n"\
			"[stat_v1k_min_dt] INTEGER  NULL, \r\n"\
			"[stat_v1k_min] INTEGER  NULL, \r\n"\
			"[stat_v1k_avg] INTEGER  NULL, \r\n"\
			"[stat_v1k_avg_num] INTEGER  NULL)" );

		// 索引 - 历史数据
		db->exec( "CREATE INDEX if not exists [IDX_HISTDATA_] ON [histdata]( \r\n"\
			"[data_dt]  ASC, \r\n"\
			"[dev_type]  ASC, \r\n"\
			"[dev_id]  ASC, \r\n"\
			"[address_type]  ASC, \r\n"\
			"[address_id]  ASC)" );

		// 索引 - 日统计
		db->exec( "CREATE INDEX if not exists [IDX_STAT_DAY_] ON [stat_day]( \r\n"\
			"[dt_day]  ASC, \r\n"\
			"[dev_type]  ASC, \r\n"\
			"[dev_id]  ASC, \r\n"\
			"[address_type]  ASC, \r\n"\
			"[address_id]  ASC)" );


		//...
		m_is_stat_month = DBIsBeing_stat_month( struRec_stat_month(struRec_stat_day::struKey()) );
		DBIsBeing_stat_day_getlist( struRec_stat_day(struRec_stat_day::struKey()), m_lst_is_stat_day );

		print_is_stat();

		//LOGMSGEX( defLOGNAME, defLOG_INFO, "Open DBSave usetime=%dms, AutoCreate=%d, ReadOnly=%d, \"%s\"\r\n", timeGetTime()-dwstart, (int)DoAutoCreate, (int)m_isReadOnly, m_path.c_str() );
	}
	catch(...)
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "Open DBSave failed! usetime=%dms, ReadOnly=%d, \"%s\"\r\n", timeGetTime()-dwstart, (int)m_isReadOnly, m_path.c_str() );

		if(db)
		{
			delete db;
			db = NULL;
		}
	}
}

SQLite_DBDataSave::~SQLite_DBDataSave(void)
{
	if(db)
	{
		delete db;
		db = NULL;
	}
}

void SQLite_DBDataSave::print_is_stat() const
{
	std::string str = "print_is_stat(";
	str += m_dbname;
	str += ")";

	char buf[32] = {0};

	snprintf( buf, sizeof(buf), " m=%d d(size=%d)(", (int)is_stat_month(), m_lst_is_stat_day.size() );
	str += buf;

	for( defmap_is_stat_day::const_iterator it=m_lst_is_stat_day.begin(); it!=m_lst_is_stat_day.end(); ++it )
	{
		snprintf( buf, sizeof(buf), "%d,", (int)*it );
		str += buf;
	}

	str += ")";

	LOGMSG( str.c_str() );
}

bool SQLite_DBDataSave::is_all_stat_day_finished() const
{
	int maxday = 31;
	struGSTime curdt;
	g_struGSTime_GetCurTime( curdt );
	if( !g_IsSameMonth( m_dtDBSave, curdt ) )
	{
		maxday = g_GetMaxDay( m_dtDBSave.Year, m_dtDBSave.Month );
	}
	else
	{
		const time_t curUTCTime = g_GetUTCTime();
		const time_t prevdayUTCTime = curUTCTime-24*60*60;
		struGSTime prevdayGSTime;
		if( g_UTCTime_To_struGSTime( prevdayUTCTime, prevdayGSTime ) )
		{
			maxday = prevdayGSTime.Day;
		}
	}

	for( int day=1; day<=maxday; ++day )
	{
		if( !is_stat_day(day) )
		{
			return false;
		}
	}

	return true;
}

bool SQLite_DBDataSave::isCan_DoStat_month() const
{
	struGSTime curdt;
	g_struGSTime_GetCurTime( curdt );

	if( curdt.Year == m_dtDBSave.Year
		&& curdt.Month == m_dtDBSave.Month
		)
	{
		return false;
	}

	return true;
}

bool SQLite_DBDataSave::isCan_DoStat_day( const unsigned char day ) const
{
	struGSTime curdt;
	g_struGSTime_GetCurTime( curdt );

	if( curdt.Year == m_dtDBSave.Year
		&& curdt.Month == m_dtDBSave.Month
		&& day >= curdt.Day
		)
	{
		return false;
	}

	return true;
}

void SQLite_DBDataSave::insertdata( const defvecDataSave &vecDataSave )
{
	if( vecDataSave.empty() )
		return;

	if( !this->db )
	{
		LOGMSG( "DBSaveInsert Failed db=NULL!(%s) DB(%d,%d)\r\n", this->m_dbname.c_str(), m_dtDBSave.Year, m_dtDBSave.Month );
		return;
	}

	if( this->m_isReadOnly )
	{
		LOGMSG( "DBSaveInsert Failed isReadOnly!(%s) DB(%d,%d) db=NULL\r\n", this->m_dbname.c_str(), m_dtDBSave.Year, m_dtDBSave.Month );
		return;
	}

	const DWORD dwstart = timeGetTime();

	UseDbTransAction dbta( db );

	int i = 1;
	const int count = vecDataSave.size();
	for( defvecDataSave::const_iterator it=vecDataSave.begin(); it!=vecDataSave.end(); ++it, ++i )
	{
		const std::string strdt = g_TimeToStr((*it)->data_dt);

		try
		{
#if 1
			SQLite::Statement  query(*db,"INSERT INTO histdata VALUES(NULL,:data_dt,:dev_type,:dev_id,:address_type,:address_id,:data_dt_str,:dataflag,:strvalue,:value1k)");

			int col = 1;
			query.bind(col++, (int)(*it)->data_dt);
			query.bind(col++, (int)(*it)->AddrObjKey.dev_type);
			query.bind(col++, (*it)->AddrObjKey.dev_id);
			query.bind(col++, (int)(*it)->AddrObjKey.address_type);
			query.bind(col++, (*it)->AddrObjKey.address_id);
			query.bind(col++, strdt);
			query.bind(col++, (int)(*it)->dataflag);
			query.bind(col++, (*it)->strvalue);
			query.bind(col++, (int)(atof((*it)->strvalue.c_str())*1000));
			query.exec();
#else
			SQLite::Statement  query(*db,"INSERT INTO histdata VALUES(NULL,:data_dt,:dev_type,:dev_id,:address_type,:address_id,:data_dt_str,:dataflag,:strvalue,:value1k,:name)");

			int col = 1;
			query.bind(col++, (int)(*it)->data_dt);
			query.bind(col++, (int)(*it)->AddrObjKey.dev_type);
			query.bind(col++, (*it)->AddrObjKey.dev_id);
			query.bind(col++, (int)(*it)->AddrObjKey.address_type);
			query.bind(col++, (*it)->AddrObjKey.address_id);
			query.bind(col++, strdt);
			query.bind(col++, (int)(*it)->dataflag);
			query.bind(col++, (*it)->strvalue);
			query.bind(col++, (int)(atof((*it)->strvalue.c_str())*1000));
			query.bind(col++, (*it)->name);
			query.exec();
#endif
		}
		catch(...)
		{
			LOGMSG( "DBSaveInsert(%d/%d) Failed!(%s,%s) %s, flag=%d, value=%s\r\n", i, count, (*it)->name.c_str(), strdt.c_str(), (*it)->AddrObjKey.get_str().c_str(), (*it)->dataflag, (*it)->strvalue.c_str() );
			return;
		}

		LOGMSG( "DBSaveInsert(%d/%d)(%s,%s) %s, flag=%d, val=%s, tm=%dms", i, count, (*it)->name.c_str(), strdt.c_str(), (*it)->AddrObjKey.get_str().c_str(), (*it)->dataflag, (*it)->strvalue.c_str(), timeGetTime()-dwstart );
	}

	dbta.Commit();

	LOGMSG( "DBSaveInsert count=%d BatchEnd (%s) DB(%d,%d) tm=%dms\r\n", count, this->m_dbname.c_str(), m_dtDBSave.Year, m_dtDBSave.Month, timeGetTime()-dwstart );
}

// 检查统计，未统计的进行统计
bool SQLite_DBDataSave::CheckAndDoStat_day()
{
	if( is_all_stat_day_finished() && is_stat_month() )
		return true;

	struGSTime curdt;
	g_struGSTime_GetCurTime( curdt );
	if( !g_IsSameMonth( m_dtDBSave, curdt ) )
	{
		DoStat_day_all();
	}
	else
	{
		const time_t curUTCTime = g_GetUTCTime();
		const time_t prevdayUTCTime = curUTCTime-24*60*60;
		struGSTime prevdayGSTime;
		if( g_UTCTime_To_struGSTime( prevdayUTCTime, prevdayGSTime ) )
		{
			const short stat_daymax = prevdayGSTime.Day;
			DoStat_day_all_specmax( stat_daymax );
		}
	}

	if( !is_stat_month() && isCan_DoStat_month() )
	{
		DoStat_month_all();
	}

	return true;
}

// 针对这个库里的所有对象的所有日统计：对于未进行统计都执行日统计，已统计的不再执行
defGSReturn SQLite_DBDataSave::DoStat_day_all()
{
	if( !db )
	{
		return defGSReturn_DBNoExist;
	}

	const unsigned char maxday = g_GetMaxDay( m_dtDBSave.Year, m_dtDBSave.Month );

	return DoStat_day_all_specmax( maxday );
}

// 从1开始至maxday的未进行统计都执行日统计，已统计的不再执行
defGSReturn SQLite_DBDataSave::DoStat_day_all_specmax( const unsigned char maxday, const unsigned char specstart )
{
	if( !db )
	{
		return defGSReturn_DBNoExist;
	}

	bool hasstat = false;
	for( int day=specstart; day<=maxday; ++day )
	{
		if( !is_stat_day(day) )
		{
			hasstat = true;
		}
	}

	if( !hasstat )
	{
		return defGSReturn_Success;
	}

	const DWORD dwstart = timeGetTime();

	UseDbTransAction dbta( db );

	uint32_t countDoStat = 0;
	for( int day=specstart; day<=maxday; ++day )
	{
		if( !is_stat_day(day) )
		{
			countDoStat++;

			const DWORD dwstart = timeGetTime();

			DoStat_day_specday( day );

			//LOGMSG( "DataStoreMgr DoStat_day_specday(%d) usetime=%dms\r\n", day, timeGetTime()-dwstart );
		}
	}

	dbta.Commit();

	if( countDoStat>0 )
	{
		LOGMSG( "DataStoreMgr DoStat_day_all_specmax(%d-%d) countDoStat=%d, usetime=%dms\r\n", specstart, maxday, countDoStat, timeGetTime()-dwstart );
	}

	return defGSReturn_Success;
}

// 指定某一天，对这一天的所有对象进行日统计
defGSReturn SQLite_DBDataSave::DoStat_day_specday( const unsigned char day )
{
	DWORD dwstart = timeGetTime();
	const DWORD dwstart_base = dwstart;
	
	if( !db )
	{
		return defGSReturn_DBNoExist;
	}

	const unsigned char maxday = g_GetMaxDay( m_dtDBSave.Year, m_dtDBSave.Month );
	if( day <1 || day>maxday )
	{
		LOGMSG( "DataStoreMgr DoStat_day errday! day=%d, maxday=%d\r\n", day, maxday );
		return defGSReturn_ErrParam;
	}

	if( is_stat_day(day) )
	{
		LOGMSG( "DataStoreMgr DoStat_day(day=%d) is_stat_day\r\n", day );
		return defGSReturn_Success;
	}

	if( IsReadOnly() )
	{
		LOGMSG( "DataStoreMgr DoStat_day IsReadOnly err!(day=%d)\r\n", day );
		return defGSReturn_IsLock;
	}

	if( !isCan_DoStat_day(day) )
	{
		return defGSReturn_ErrParam;
	}

	defmapRec_stat_day mapRec_stat_day;
	const defGSReturn ret = DBLoad_histdata_AllObj_ForDay( day, mapRec_stat_day );
	if( macGSFailed(ret) )
	{
		g_delete_mapRec_stat_day( mapRec_stat_day );
		return ret;
	}

	dwstart = timeGetTime();

	for( defmapRec_stat_day::const_iterator it=mapRec_stat_day.begin(); it!=mapRec_stat_day.end(); ++it )
	{
		struRec_stat_day *pRec_stat_day = it->second;
		DoStat_day_specday_specone( *pRec_stat_day );
		DBSave_stat_day( *pRec_stat_day );
		//delete pRec_stat_day;
	}

	// 写入已完成日统计记录标志
	if( DBSave_stat_day( struRec_stat_day(struRec_stat_day::struKey(m_dtDBSave.Year, m_dtDBSave.Month, day)) ) )
	{
		m_lst_is_stat_day.insert( day );
	}

	//mapRec_stat_day.clear();
	g_delete_mapRec_stat_day( mapRec_stat_day );

	LOGMSG( "DataStoreMgr DoStat_day_specday(%d) all end usetime=%dms\r\n", day, timeGetTime()-dwstart_base );
	return defGSReturn_Success;
}

// 分析得到时间点所在值
bool SQLite_DBDataSave::GetValueForSpecTime_lst( const time_t spec_data_dt, const defmapV1k &lst_v1k, stru_V1K &get_v1k, const time_t dt_valid_range_prev, const time_t dt_valid_range_next )
{
	stru_V1K right_v1k = 0;
	g_getRightV1k( spec_data_dt, lst_v1k, right_v1k, dt_valid_range_next );

	if( spec_data_dt==right_v1k.data_dt )
	{
		get_v1k = right_v1k;
		return true;
	}
	else
	{
		stru_V1K left_v1k = 0;
		g_getLeftV1k( spec_data_dt, lst_v1k, left_v1k, dt_valid_range_prev );

		if( spec_data_dt==left_v1k.data_dt )
		{
			get_v1k = left_v1k;
			return true;
		}
		else if( 0==right_v1k.data_dt && 0==left_v1k.data_dt )
		{
		}
		else if( 0==right_v1k.data_dt )
		{
			get_v1k = left_v1k;
			return true;
		}
		else if( 0==left_v1k.data_dt )
		{
			get_v1k = right_v1k;
			return true;
		}
		else
		{
			bool getleft = true;
			// 左右都有且时间都不是完全相等的情况下

			// 时间很接近
			const time_t left_span = spec_data_dt - left_v1k.data_dt;
			const time_t right_span = right_v1k.data_dt - spec_data_dt;
			if( left_span<60 || right_span<60  )
			{
				if( left_span <= right_span ) // 时间接近的优先原则
				{
					getleft = true;
				}
				else
				{
					getleft = false;
				}
			}
			else if( left_v1k.data_dt ) // 前值优先原则
			{
				getleft = true;
			}
			else
			{
				getleft = false;
			}

			if( getleft )
			{
				get_v1k = left_v1k;
			}
			else
			{
				get_v1k = right_v1k;
			}

			return true;
		}
	}

	return false;
}

// 分析得到时间范围内的统计
void SQLite_DBDataSave::GetOneStatForSpecTime_lst( const time_t data_dt_begin, const time_t data_dt_end, const defmapV1k &lst_src_v1k, struStat &Stat )
{
	const defmapV1k::const_iterator itBegin = lst_src_v1k.lower_bound( data_dt_begin );	// 不小于data_dt，即>=data_dt
	if( itBegin == lst_src_v1k.end() )
		return;

	const defmapV1k::const_iterator itEnd = lst_src_v1k.upper_bound( data_dt_end );	// 大于data_dt
	
	double sum = 0.0f;
	for( defmapV1k::const_iterator it=itBegin; it!=itEnd; ++it )
	{
		if( !it->second.v1k_valid )
			continue;

		const time_t curTime = it->first;
		const int curValue = it->second.v1k;

		// 总数累计，后面计算平均值
		sum += it->second.v1k;
		Stat.stat_v1k_avg_num++;

		g_AnalyseStat( Stat, curTime, curValue, data_dt_begin, data_dt_end );
	}

	// 计算平均值
	Stat.calc_avg( sum );
}

// 对一个对象做日统计
// retime_maxmin: 重新调整最大最小值时间
bool SQLite_DBDataSave::DoStat_day_specday_specone( struRec_stat_day &Rec_stat_day, const bool retime_maxmin, const time_t data_dt_begin, const time_t data_dt_end )
{
	if( Rec_stat_day.lst_src_v1k.empty() )
	{
		return false;
	}

	const time_t curUTCTime = g_GetUTCTime();

	struGSTime statdt( Rec_stat_day.key.dt_year, Rec_stat_day.key.dt_month, Rec_stat_day.key.dt_day );

	double sum = 0.0f;
	for( defmapV1k::const_iterator it=Rec_stat_day.lst_src_v1k.begin(); it!=Rec_stat_day.lst_src_v1k.end(); ++it )
	{
		if( !it->second.v1k_valid )
			continue;

		const time_t curTime = it->first;
		const int curValue = it->second.v1k;

		// 总数累计，后面计算平均值
		sum += it->second.v1k;
		Rec_stat_day.Stat.stat_v1k_avg_num++;

		g_AnalyseStat( Rec_stat_day.Stat, curTime, curValue, data_dt_begin, data_dt_end );
	}

	// 计算平均值
	Rec_stat_day.Stat.calc_avg( sum );

	const time_t statBaseTime = g_struGSTime_To_UTCTime( statdt );
	time_t curstatTime = statBaseTime;

	std::bitset<24> bit_valid_A(0);
	std::bitset<24> bit_valid_B(0);

	for( int i=0; i<48; ++i, curstatTime+=(30*60) )
	{
		// 记录得到的原始时间和值
		stru_V1K src_get_v1k;

		const bool isvalid = curstatTime>curUTCTime ? false : GetValueForSpecTime_lst( curstatTime, Rec_stat_day.lst_src_v1k, src_get_v1k ) && src_get_v1k.v1k_valid;

		Rec_stat_day.v1k[i] = src_get_v1k.v1k;

		// set flag
		if( i<24 )
		{
			bit_valid_A.set( i, isvalid );
		}
		else
		{
			bit_valid_B.set( i-24, isvalid );
		}

		// 最大最小值是否在原始时间点上，与原始值相等
		if( retime_maxmin && src_get_v1k.data_dt && src_get_v1k.v1k_valid )
		{
			// 最大值比较
			if( Rec_stat_day.Stat.stat_v1k_max_dt )
			{
				// 值相等并且时间更早
				if( src_get_v1k.v1k == Rec_stat_day.Stat.stat_v1k_max && curstatTime<Rec_stat_day.Stat.stat_v1k_max_dt )
				{
					Rec_stat_day.Stat.stat_v1k_max_dt = curstatTime;//src_get_data_dt;
					Rec_stat_day.Stat.stat_v1k_max = src_get_v1k.v1k;
				}
			}

			// 最小值比较
			if( Rec_stat_day.Stat.stat_v1k_min_dt )
			{
				// 值相等并且时间更早
				if( src_get_v1k.v1k == Rec_stat_day.Stat.stat_v1k_min && curstatTime<Rec_stat_day.Stat.stat_v1k_min_dt )
				{
					Rec_stat_day.Stat.stat_v1k_min_dt = curstatTime;//src_get_data_dt;
					Rec_stat_day.Stat.stat_v1k_min = src_get_v1k.v1k;
				}
			}
		}
	}

	Rec_stat_day.v1k_valid_00_11A = bit_valid_A.to_ulong();
	Rec_stat_day.v1k_valid_12_23B = bit_valid_B.to_ulong();

	return true;
}

// 分析原始数据列表得到小时统计列表
void SQLite_DBDataSave::DoStat_hour_lst( const time_t data_dt_begin, const time_t data_dt_end, const defmapV1k &lst_src_v1k, defmapstruStat &lst_stathour, const int spanhour )
{
	const time_t span = spanhour*60*60;
	for( time_t curtime=data_dt_begin; curtime<=data_dt_end; curtime+=span )
	{
		const time_t hour_begin = curtime;
		const time_t hour_end = curtime + span - 1;

		struStat Stat;
		GetOneStatForSpecTime_lst( hour_begin, hour_end, lst_src_v1k, Stat );
		if( Stat.stat_valid )
		{
			struGSTime dt;
			g_UTCTime_To_struGSTime( curtime, dt );
			lst_stathour[dt] = Stat;
		}
	}
}

// 分析原始数据列表得到分钟统计列表
void SQLite_DBDataSave::DoStat_minute_lst( const time_t data_dt_begin, const time_t data_dt_end, const defmapV1k &lst_src_v1k, defmapstruStat &lst_statminute, const int spanminute )
{
	const time_t curUTCTime = g_GetUTCTime();

	const time_t span = spanminute*60;
	for( time_t curtime=data_dt_begin; curtime<=data_dt_end; curtime+=span )
	{
		const time_t minute_begin = curtime;
		const time_t minute_end = curtime + span - 1;

		struStat Stat;
		GetOneStatForSpecTime_lst( minute_begin, minute_end, lst_src_v1k, Stat );
		if( Stat.stat_valid )
		{
			struGSTime dt;
			g_UTCTime_To_struGSTime( curtime, dt );
			lst_statminute[dt] = Stat;
		}
		else
		{
			// 记录得到的原始时间和值
			stru_V1K src_get_v1k;

			if( GetValueForSpecTime_lst( curtime, lst_src_v1k, src_get_v1k ) && src_get_v1k.v1k_valid )
			{
				struStat Stat;
				Stat.stat_valid = 1;
				Stat.stat_v1k_max_dt = curtime;
				Stat.stat_v1k_max = src_get_v1k.v1k;
				Stat.stat_v1k_min_dt = curtime;
				Stat.stat_v1k_min = src_get_v1k.v1k;
				Stat.stat_v1k_avg = src_get_v1k.v1k;
				Stat.stat_v1k_avg_num = 1;

				struGSTime dt;
				g_UTCTime_To_struGSTime( curtime, dt );
				lst_statminute[dt] = Stat;
			}
		}
	}
}

// 月统计
defGSReturn SQLite_DBDataSave::DoStat_month_all()
{
	if( !db )
	{
		return defGSReturn_DBNoExist;
	}

	const DWORD dwstart = timeGetTime();

	LOGMSG( "DataStoreMgr DoStat_month(%d-%d,%s)\r\n", m_dtDBSave.Year, m_dtDBSave.Month, m_dbname.c_str() );

	if( is_stat_month() )
	{
		LOGMSG( "DataStoreMgr DoStat_month(%d) is_stat_month\r\n", m_dtDBSave.Month );
		return defGSReturn_Success;
	}

	if( IsReadOnly() )
	{
		LOGMSG( "DataStoreMgr DoStat_month(%d) IsReadOnly\r\n", m_dtDBSave.Month );
		return defGSReturn_IsLock;
	}

	if( !isCan_DoStat_month() )
	{
		return defGSReturn_ErrParam;
	}

	defmapRec_stat_day mapRec_stat_day;
	const defGSReturn ret = DBLoad_stat_day_full( mapRec_stat_day );
	if( macGSFailed(ret) )
	{
		g_delete_mapRec_stat_day( mapRec_stat_day );
		return ret;
	}

	// 遍历这个月的所有日统计，二次计算得出月统计
	defmapRec_stat_month mapRec_stat_month;
	for( defmapRec_stat_day::const_iterator itD=mapRec_stat_day.begin(); itD!=mapRec_stat_day.end(); ++itD )
	{
		const struRec_stat_day *pDay = itD->second;

		if( !pDay->Stat.stat_valid )
			continue;

		struRec_stat_month::struKey key(pDay->key);

		struRec_stat_month *pMonth = NULL;
		defmapRec_stat_month::const_iterator itM = mapRec_stat_month.find( key );
		if( itM != mapRec_stat_month.end() )
		{
			pMonth = itM->second;
		}
		else
		{
			pMonth = new struRec_stat_month(key);
			mapRec_stat_month[key] = pMonth;
		}

		if( !pMonth )
			continue;

		// 总数累计，后面计算平均值
		pMonth->temp_v1k_sum += double(pDay->Stat.stat_v1k_avg) * pDay->Stat.stat_v1k_avg_num;
		pMonth->Stat.stat_v1k_avg_num += pDay->Stat.stat_v1k_avg_num;

		g_AnalyseStat( pMonth->Stat, pDay->Stat );
	}

	UseDbTransAction dbta( db );

	for( defmapRec_stat_month::const_iterator itM=mapRec_stat_month.begin(); itM!=mapRec_stat_month.end(); ++itM )
	{
		struRec_stat_month *pMonth = itM->second;

		// 计算平均值
		pMonth->Stat.calc_avg( pMonth->temp_v1k_sum );

		DBSave_stat_month( *pMonth );
	}

	// 写入已完成月统计记录标志
	if( DBSave_stat_month( struRec_stat_month(struRec_stat_month::struKey(m_dtDBSave.Year, m_dtDBSave.Month )) ) )
	{
		m_is_stat_month = true;
	}

	dbta.Commit();

	LOGMSG( "DataStoreMgr DoStat_month_all End usetime=%dms\r\n", timeGetTime()-dwstart );

	return defGSReturn_Success;
}

// 获取指定时间的值
defGSReturn SQLite_DBDataSave::QueryValueForSpecTime( const GSIOTAddrObjKey &AddrObjKey, const time_t spec_data_dt, stru_V1K &get_v1k )
{
	get_v1k.reset();

	const int c_timerange = 60*60;
	const time_t data_dt_begin = spec_data_dt - c_timerange;
	const time_t data_dt_end = spec_data_dt + c_timerange;

	defmapV1k lst_v1k;

	const defGSReturn ret = DBLoad_histdata_SpecObj_ForTime( AddrObjKey, data_dt_begin, data_dt_end, lst_v1k );
	if( macGSFailed(ret) )
	{
		LOGMSG( "QueryValueForSpecTime %s Failed! ret=%d\r\n", AddrObjKey.get_str().c_str(), ret );
		return ret;
	}
	
	if( GetValueForSpecTime_lst( spec_data_dt, lst_v1k, get_v1k ) )
	{
		return defGSReturn_Success;
	}

	return defGSReturn_DBNoRec; // 没有符合条件的记录
}

// 载入指定对象指定日的原始v1k数据
defGSReturn SQLite_DBDataSave::DBLoad_histdata_SpecObj_ForDay( const GSIOTAddrObjKey &AddrObjKey, const unsigned char day, defmapV1k &lst_src_v1k ) const
{
	const time_t data_dt_begin = g_struGSTime_To_UTCTime( struGSTime( m_dtDBSave.Year, m_dtDBSave.Month, day ) );
	const time_t data_dt_end = data_dt_begin + 24*60*60;

	return DBLoad_histdata_SpecObj_ForTime( AddrObjKey, data_dt_begin, data_dt_end, lst_src_v1k );
}

// 载入指定对象指定时间段的原始v1k数据，分别存储在mapRec_stat_day的各自对象里
defGSReturn SQLite_DBDataSave::DBLoad_histdata_SpecObj_mapRecStatDay_ForTime( const GSIOTAddrObjKey &AddrObjKey, const time_t data_dt_begin, const time_t data_dt_end, defmapRec_stat_day &mapRec_stat_day ) const
{
	if( !this->db )
	{
		return defGSReturn_DBNoExist;
	}

	char sqlbuf[1024] = {0};

	snprintf( sqlbuf, sizeof(sqlbuf), "select data_dt, dev_type, dev_id, address_type, address_id, dataflag, value1k from histdata where data_dt>=%lu and data_dt<=%lu and dev_type=%d and dev_id=%d and address_type=%d and address_id=%d", 
		(uint32_t)data_dt_begin, (uint32_t)data_dt_end, AddrObjKey.dev_type, AddrObjKey.dev_id, AddrObjKey.address_type, AddrObjKey.address_id );

	try
	{
		SQLite::Statement query( *this->db, sqlbuf );
		while( query.executeStep() )
		{
			int col = 0;
			int data_dt = query.getColumn(0).getInt();
			defDataFlag_ dataflag = (defDataFlag_)query.getColumn(5).getInt();
			int v1k = query.getColumn(6).getInt();

			struGSTime dt;
			g_UTCTime_To_struGSTime( data_dt, dt );

			if( !g_IsSameMonth( m_dtDBSave, dt ) )
			{
				continue;
			}

			struRec_stat_day *pDay = NULL;

			struRec_stat_day::struKey key( dt.Year, dt.Month, dt.Day, AddrObjKey.dev_type, AddrObjKey.dev_id, AddrObjKey.address_type, AddrObjKey.address_id );
			defmapRec_stat_day::const_iterator it = mapRec_stat_day.find( key );
			if( it != mapRec_stat_day.end() )
			{
				pDay = it->second;
			}
			else
			{
				pDay = new struRec_stat_day(key);
				mapRec_stat_day[key] = pDay;
			}

			if( pDay )
			{
				pDay->lst_src_v1k[data_dt] = stru_V1K(data_dt,dataflag,v1k);
			}
		}
	}
	catch(...)
	{
		return defGSReturn_Err;
	}

	return defGSReturn_Success;
}

// 载入指定对象指定时间段的原始v1k数据
defGSReturn SQLite_DBDataSave::DBLoad_histdata_SpecObj_ForTime( const GSIOTAddrObjKey &AddrObjKey, const time_t data_dt_begin, const time_t data_dt_end, defmapV1k &lst_src_v1k, const bool doQueryMax, const int spanrate, const bool rateForType ) const
{
	if( !this->db )
	{
		return defGSReturn_DBNoExist;
	}

	char sqlbuf[1024] = {0};

	snprintf( sqlbuf, sizeof(sqlbuf), "select data_dt, dev_type, dev_id, address_type, address_id, dataflag, value1k from histdata where data_dt>=%lu and data_dt<%lu and dev_type=%d and dev_id=%d and address_type=%d and address_id=%d", 
		(uint32_t)data_dt_begin, (uint32_t)data_dt_end, AddrObjKey.dev_type, AddrObjKey.dev_id, AddrObjKey.address_type, AddrObjKey.address_id );

	try
	{
		bool hasprev = false;
		int prev_v1k = 0;

		if( spanrate && !lst_src_v1k.empty() )
		{
			defmapV1k::const_iterator it = lst_src_v1k.end();
			--it;

			hasprev = true;
			prev_v1k = it->second.v1k;
		}

		SQLite::Statement query( *this->db, sqlbuf );
		while( query.executeStep() )
		{
			int col = 0;
			int data_dt = query.getColumn(0).getInt();
			defDataFlag_ dataflag = (defDataFlag_)query.getColumn(5).getInt();
			int v1k = query.getColumn(6).getInt();

			// 去除变化率低的数据
			if( spanrate )
			{
				if( hasprev )
				{
					if( rateForType && g_AddrTypeValue_hasTypeValue(AddrObjKey.address_type) )
					{
						if( abs( g_AddrTypeValue_getTypeValue(AddrObjKey.address_type,v1k)-g_AddrTypeValue_getTypeValue(AddrObjKey.address_type,prev_v1k) ) < spanrate )
						{
							continue;
						}
					}
					else
					{
						if( abs(v1k-prev_v1k) < spanrate )
						{
							continue;
						}
					}
				}

				hasprev = true;
				prev_v1k = v1k;
			}

			lst_src_v1k[data_dt] = stru_V1K(data_dt,dataflag,v1k);

			if( doQueryMax )
			{
				const int SYS_QueryLstMax = RUNCODE_Get(defCodeIndex_SYS_QueryLstMax);
				if( lst_src_v1k.size() > SYS_QueryLstMax )
				{
					return defGSReturn_ResLimit;
				}
			}
		}
	}
	catch(...)
	{
		return defGSReturn_Err;
	}

	return defGSReturn_Success;
}

// 读取指定日的所有对象的原始数据
defGSReturn SQLite_DBDataSave::DBLoad_histdata_AllObj_ForDay( const unsigned char day, defmapRec_stat_day &mapRec_stat_day ) const
{
	if( !this->db )
	{
		return defGSReturn_DBNoExist;
	}

	DWORD dwstart = timeGetTime();

	char sqlbuf[1024] = {0};

	const time_t data_dt_begin = g_struGSTime_To_UTCTime( struGSTime( m_dtDBSave.Year, m_dtDBSave.Month, day ) );
	const time_t data_dt_end = data_dt_begin + 24*60*60;

	snprintf( sqlbuf, sizeof(sqlbuf), "select data_dt, dev_type, dev_id, address_type, address_id, dataflag, value1k from histdata where data_dt>=%lu and data_dt<%lu", 
		(uint32_t)data_dt_begin, (uint32_t)data_dt_end );

	try
	{
		SQLite::Statement query( *this->db, sqlbuf );

		LOGMSG( "DataStoreMgr DBLoad_Day_SrcV1k query usetime=%dms\r\n", timeGetTime()-dwstart );dwstart = timeGetTime();

		uint32_t count = 0;
		while( query.executeStep() )
		{
			count++;
			
			int col = 0;

			int data_dt = query.getColumn(col++).getInt();
			IOTDeviceType dev_type = (IOTDeviceType)query.getColumn(col++).getInt();
			int dev_id = query.getColumn(col++).getInt();
			IOTDeviceType address_type = (IOTDeviceType)query.getColumn(col++).getInt();
			int address_id = query.getColumn(col++).getInt();

			defDataFlag_ dataflag = (defDataFlag_)query.getColumn(col++).getInt();
			int v1k = query.getColumn(col++).getInt();
			
			struRec_stat_day *pDay = NULL;
			
			struRec_stat_day::struKey key( m_dtDBSave.Year, m_dtDBSave.Month, day, dev_type, dev_id, address_type, address_id );
			defmapRec_stat_day::const_iterator it = mapRec_stat_day.find( key );
			if( it != mapRec_stat_day.end() )
			{
				pDay = it->second;
			}
			else
			{
				pDay = new struRec_stat_day(key);
				mapRec_stat_day[key] = pDay;
			}

			if( pDay )
			{
				pDay->lst_src_v1k[data_dt] = stru_V1K(data_dt,dataflag,v1k);
			}
		}

		LOGMSG( "DataStoreMgr DBLoad_Day_SrcV1k LoadEnd count=%d usetime=%dms\r\n", count, timeGetTime()-dwstart );dwstart = timeGetTime();
	}
	catch(...)
	{
		LOGMSG( "DataStoreMgr DBLoad_Day_SrcV1k Failed! usetime=%dms\r\n", timeGetTime()-dwstart );dwstart = timeGetTime();
		return defGSReturn_Err;
	}

	return defGSReturn_Success;
}

// 将一条日统计数据库记录读取到结构
defGSReturn SQLite_DBDataSave::DBReadOneRec_stat_day( SQLite::Statement &query, struRec_stat_day &Rec_stat_day ) const
{
	if( !Rec_stat_day.lst_src_v1k.empty() )
	{
		Rec_stat_day.lst_src_v1k.clear();
	}

	int col = 1;

	Rec_stat_day.key.dt_day = query.getColumn(col++).getInt();
	Rec_stat_day.key.AddrObjKey.dev_type = (IOTDeviceType)query.getColumn(col++).getInt();
	Rec_stat_day.key.AddrObjKey.dev_id = query.getColumn(col++).getInt();
	Rec_stat_day.key.AddrObjKey.address_type = (IOTDeviceType)query.getColumn(col++).getInt();
	Rec_stat_day.key.AddrObjKey.address_id = query.getColumn(col++).getInt();

	Rec_stat_day.Stat.stat_valid = query.getColumn(col++).getInt();
	Rec_stat_day.Stat.stat_v1k_max_dt = query.getColumn(col++).getInt();
	Rec_stat_day.Stat.stat_v1k_max = query.getColumn(col++).getInt();
	Rec_stat_day.Stat.stat_v1k_min_dt = query.getColumn(col++).getInt();
	Rec_stat_day.Stat.stat_v1k_min = query.getColumn(col++).getInt();
	Rec_stat_day.Stat.stat_v1k_avg = query.getColumn(col++).getInt();
	Rec_stat_day.Stat.stat_v1k_avg_num = query.getColumn(col++).getInt();

	Rec_stat_day.v1k_valid_00_11A = query.getColumn(col++).getInt();
	Rec_stat_day.v1k_valid_12_23B = query.getColumn(col++).getInt();

	for( int i=0; i<48; ++i )
	{
		Rec_stat_day.v1k[i] = query.getColumn(col++).getInt();
	}

	return defGSReturn_Success;
}

// 载入一条指定对象指定日的日统计记录
defGSReturn SQLite_DBDataSave::DBLoad_stat_day_SpecObj_SpecDay( struRec_stat_day &Rec_stat_day ) const
{
	if( !this->db )
	{
		return defGSReturn_DBNoExist;
	}

	if( Rec_stat_day.key.dt_year != m_dtDBSave.Year )
	{
		return defGSReturn_ErrParam;
	}

	if( Rec_stat_day.key.dt_month != m_dtDBSave.Month )
	{
		return defGSReturn_ErrParam;
	}

	try
	{
		char sqlbuf[1024] = {0};
		snprintf( sqlbuf, sizeof(sqlbuf), "select * from stat_day where dt_day=%d and address_id=%d and dev_id=%d and address_type=%d and dev_type=%d LIMIT 1", 
			Rec_stat_day.key.dt_day, Rec_stat_day.key.AddrObjKey.address_id, Rec_stat_day.key.AddrObjKey.dev_id, Rec_stat_day.key.AddrObjKey.address_type, Rec_stat_day.key.AddrObjKey.dev_type );

		SQLite::Statement query( *this->db, sqlbuf );
		if( query.executeStep() )
		{
			if( defGSReturn_Success == DBReadOneRec_stat_day( query, Rec_stat_day ) )
				return defGSReturn_Success;
		}
	}
	catch(...)
	{
		return defGSReturn_Err;
	}

	return defGSReturn_Err;
}

// 载入所有指定对象指定日范围的日统计记录
defGSReturn SQLite_DBDataSave::DBLoad_stat_day_SpecObj_DayRange( const GSIOTAddrObjKey &AddrObjKey, const unsigned char day_begin, const unsigned char day_end, defmapRec_stat_day &mapRec_stat_day ) const
{
	if( !this->db )
	{
		return defGSReturn_DBNoExist;
	}

	try
	{
		struRec_stat_day Rec_stat_day( struRec_stat_day::struKey( m_dtDBSave.Year, m_dtDBSave.Month ) );

		char sqlbuf[1024] = {0};
		snprintf( sqlbuf, sizeof(sqlbuf), "select * from stat_day where dt_day>=%d and dt_day<=%d and address_id=%d and dev_id=%d and address_type=%d and dev_type=%d", 
			day_begin, day_end, AddrObjKey.address_id, AddrObjKey.dev_id, AddrObjKey.address_type, AddrObjKey.dev_type );

		SQLite::Statement query( *this->db, sqlbuf );

		while( query.executeStep() )
		{
			if( defGSReturn_Success != DBReadOneRec_stat_day( query, Rec_stat_day ) )
				continue;

			struRec_stat_day *pRec_stat_day = NULL;
			defmapRec_stat_day::const_iterator it = mapRec_stat_day.find( Rec_stat_day.key );
			if( it == mapRec_stat_day.end() )
			{
				pRec_stat_day = new struRec_stat_day(Rec_stat_day.key);
				mapRec_stat_day[Rec_stat_day.key] = pRec_stat_day;
			}
			else
			{
				continue;
			}

			if( !pRec_stat_day )
				continue;

			Rec_stat_day.moveto( *pRec_stat_day );
		}
	}
	catch(...)
	{
		return defGSReturn_Err;
	}

	return defGSReturn_Success;
}

// 载入这个月的所有对象的日统计
defGSReturn SQLite_DBDataSave::DBLoad_stat_day_full( defmapRec_stat_day &mapRec_stat_day ) const
{
	if( !this->db )
	{
		return defGSReturn_DBNoExist;
	}

	try
	{
		struRec_stat_day Rec_stat_day( struRec_stat_day::struKey( m_dtDBSave.Year, m_dtDBSave.Month ) );

		SQLite::Statement query( *this->db, "select * from stat_day" );
		while( query.executeStep() )
		{
			if( defGSReturn_Success != DBReadOneRec_stat_day( query, Rec_stat_day ) )
				continue;
			
			struRec_stat_day *pRec_stat_day = NULL;
			defmapRec_stat_day::const_iterator it = mapRec_stat_day.find( Rec_stat_day.key );
			if( it == mapRec_stat_day.end() )
			{
				pRec_stat_day = new struRec_stat_day(Rec_stat_day.key);
				mapRec_stat_day[Rec_stat_day.key] = pRec_stat_day;
			}
			else
			{
				continue;
			}

			if( !pRec_stat_day )
				continue;
			
			Rec_stat_day.moveto( *pRec_stat_day );
		}
	}
	catch(...)
	{
		return defGSReturn_Err;
	}

	return defGSReturn_Success;
}

// 将一条月统计数据库记录读取到结构
defGSReturn SQLite_DBDataSave::DBReadOneRec_stat_month( SQLite::Statement &query, struRec_stat_month &Rec_stat_month ) const
{
	int col = 1;

	Rec_stat_month.key.AddrObjKey.dev_type = (IOTDeviceType)query.getColumn(col++).getInt();
	Rec_stat_month.key.AddrObjKey.dev_id = query.getColumn(col++).getInt();
	Rec_stat_month.key.AddrObjKey.address_type = (IOTDeviceType)query.getColumn(col++).getInt();
	Rec_stat_month.key.AddrObjKey.address_id = query.getColumn(col++).getInt();

	Rec_stat_month.Stat.stat_valid = query.getColumn(col++).getInt();
	Rec_stat_month.Stat.stat_v1k_max_dt = query.getColumn(col++).getInt();
	Rec_stat_month.Stat.stat_v1k_max = query.getColumn(col++).getInt();
	Rec_stat_month.Stat.stat_v1k_min_dt = query.getColumn(col++).getInt();
	Rec_stat_month.Stat.stat_v1k_min = query.getColumn(col++).getInt();
	Rec_stat_month.Stat.stat_v1k_avg = query.getColumn(col++).getInt();
	Rec_stat_month.Stat.stat_v1k_avg_num = query.getColumn(col++).getInt();

	return defGSReturn_Success;
}

// 载入一条指定对象指定月的月统计记录
defGSReturn SQLite_DBDataSave::DBLoad_stat_month_SpecObj_SpecMonth( struRec_stat_month &Rec_stat_month ) const
{
	if( !this->db )
	{
		return defGSReturn_DBNoExist;
	}

	if( Rec_stat_month.key.dt_year != m_dtDBSave.Year )
	{
		return defGSReturn_ErrParam;
	}

	if( Rec_stat_month.key.dt_month != m_dtDBSave.Month )
	{
		return defGSReturn_ErrParam;
	}

	try
	{
		char sqlbuf[1024] = {0};
		snprintf( sqlbuf, sizeof(sqlbuf), "select * from stat_month where address_id=%d and dev_id=%d and address_type=%d and dev_type=%d LIMIT 1", 
			Rec_stat_month.key.AddrObjKey.address_id, Rec_stat_month.key.AddrObjKey.dev_id, Rec_stat_month.key.AddrObjKey.address_type, Rec_stat_month.key.AddrObjKey.dev_type );

		SQLite::Statement query( *this->db, sqlbuf );
		if( query.executeStep() )
		{
			if( defGSReturn_Success == DBReadOneRec_stat_month( query, Rec_stat_month ) )
				return defGSReturn_Success;
		}
	}
	catch(...)
	{
		return defGSReturn_Err;
	}

	return defGSReturn_DBNoRec;
}

// 载入这个月的所有对象的月统计
defGSReturn SQLite_DBDataSave::DBLoad_stat_month_full( defmapRec_stat_month &mapRec_stat_month ) const
{
	if( !this->db )
	{
		return defGSReturn_DBNoExist;
	}

	try
	{
		struRec_stat_month Rec_stat_month( struRec_stat_month::struKey( m_dtDBSave.Year, m_dtDBSave.Month ) );

		SQLite::Statement query( *this->db, "select * from stat_month" );
		while( query.executeStep() )
		{
			if( defGSReturn_Success != DBReadOneRec_stat_month( query, Rec_stat_month ) )
				continue;

			struRec_stat_month *pRec_stat_month = NULL;
			defmapRec_stat_month::const_iterator it = mapRec_stat_month.find( Rec_stat_month.key );
			if( it == mapRec_stat_month.end() )
			{
				pRec_stat_month = new struRec_stat_month(Rec_stat_month.key);
				mapRec_stat_month[Rec_stat_month.key] = pRec_stat_month;
			}
			else
			{
				continue;
			}

			if( !pRec_stat_month )
				continue;

			Rec_stat_month.moveto( *pRec_stat_month );
		}
	}
	catch(...)
	{
		return defGSReturn_Err;
	}

	return defGSReturn_Success;
}

// 指定日统计是否已存在
bool SQLite_DBDataSave::DBIsBeing_stat_day( const struRec_stat_day &Rec_stat_day )
{
	if( !this->db )
	{
		return false;
	}

	char sqlbuf[1024] = {0};
	snprintf( sqlbuf, sizeof(sqlbuf), "select * from stat_day where dt_day=%d and address_id=%d and dev_id=%d and address_type=%d and dev_type=%d LIMIT 1", 
		Rec_stat_day.key.dt_day, Rec_stat_day.key.AddrObjKey.address_id, Rec_stat_day.key.AddrObjKey.dev_id, Rec_stat_day.key.AddrObjKey.address_type, Rec_stat_day.key.AddrObjKey.dev_type );

	SQLite::Statement query( *db, sqlbuf );
	if( query.executeStep() )
	{
		return true;
	}

	return false;
}

// 日统计是否已存在 获取指定对象的所有日统计是否已存在标志
bool SQLite_DBDataSave::DBIsBeing_stat_day_getlist( const struRec_stat_day &Rec_stat_day, defmap_is_stat_day &lst_is_stat_day )
{
	if( !this->db )
	{
		return false;
	}

	char sqlbuf[1024] = {0};
	snprintf( sqlbuf, sizeof(sqlbuf), "select * from stat_day where address_id=%d and dev_id=%d and address_type=%d and dev_type=%d", 
		Rec_stat_day.key.AddrObjKey.address_id, Rec_stat_day.key.AddrObjKey.dev_id, Rec_stat_day.key.AddrObjKey.address_type, Rec_stat_day.key.AddrObjKey.dev_type );

	try
	{
		SQLite::Statement query( *this->db, sqlbuf );
		while( query.executeStep() )
		{
			int col = 1;

			lst_is_stat_day.insert( query.getColumn(col++).getInt() );
		}
	}
	catch(...)
	{
		return false;
	}

	return true;
}

// 月统计是否已存在
bool SQLite_DBDataSave::DBIsBeing_stat_month( const struRec_stat_month &Rec_stat_month )
{
	if( !this->db )
	{
		return false;
	}

	char sqlbuf[1024] = {0};
	snprintf( sqlbuf, sizeof(sqlbuf), "select * from stat_month where address_id=%d and dev_id=%d and address_type=%d and dev_type=%d LIMIT 1", 
		Rec_stat_month.key.AddrObjKey.address_id, Rec_stat_month.key.AddrObjKey.dev_id, Rec_stat_month.key.AddrObjKey.address_type, Rec_stat_month.key.AddrObjKey.dev_type );

	SQLite::Statement query( *db, sqlbuf );
	if( query.executeStep() )
	{
		return true;
	}

	return false;
}

// 保存日统计数据
bool SQLite_DBDataSave::DBSave_stat_day( const struRec_stat_day &Rec_stat_day )
{
	if( !this->db )
	{
		return false;
	}

	if( DBIsBeing_stat_day( Rec_stat_day ) )
	{
		//int id = query.getColumn(0);
		//SQLite::Statement query(*this->db,"UPDATE ");
		//query.bind(1, );
		//query.exec();
	}
	else
	{
		SQLite::Statement query( *this->db, "INSERT INTO stat_day VALUES(NULL,"\
		":dt_day,:dev_type,:dev_id,:address_type,:address_id,"\
		":stat_valid,:stat_v1k_max_dt,:stat_v1k_max,:stat_v1k_min_dt,:stat_v1k_min,:stat_v1k_avg,:stat_v1k_avg_num,"\
		":v1k_valid_00_11A,:v1k_valid_12_23B,"\
		":v1k_0000,:v1k_0030,:v1k_0100,:v1k_0130,:v1k_0200,:v1k_0230,:v1k_0300,:v1k_0330,:v1k_0400,:v1k_0430,:v1k_0500,:v1k_0530,"\
		":v1k_0600,:v1k_0630,:v1k_0700,:v1k_0730,:v1k_0800,:v1k_0830,:v1k_0900,:v1k_0930,:v1k_1000,:v1k_1030,:v1k_1100,:v1k_1130,"\
		":v1k_1200,:v1k_1230,:v1k_1300,:v1k_1330,:v1k_1400,:v1k_1430,:v1k_1500,:v1k_1530,:v1k_1600,:v1k_1630,:v1k_1700,:v1k_1730,"\
		":v1k_1800,:v1k_1830,:v1k_1900,:v1k_1930,:v1k_2000,:v1k_2030,:v1k_2100,:v1k_2130,:v1k_2200,:v1k_2230,:v1k_2300,:v1k_2330)" );

		int col = 1;
		query.bind( col++, Rec_stat_day.key.dt_day );
		query.bind( col++, Rec_stat_day.key.AddrObjKey.dev_type );
		query.bind( col++, Rec_stat_day.key.AddrObjKey.dev_id );
		query.bind( col++, Rec_stat_day.key.AddrObjKey.address_type );
		query.bind( col++, Rec_stat_day.key.AddrObjKey.address_id );

		query.bind( col++, Rec_stat_day.Stat.stat_valid );
		query.bind( col++, Rec_stat_day.Stat.stat_v1k_max_dt );
		query.bind( col++, Rec_stat_day.Stat.stat_v1k_max );
		query.bind( col++, Rec_stat_day.Stat.stat_v1k_min_dt );
		query.bind( col++, Rec_stat_day.Stat.stat_v1k_min );
		query.bind( col++, Rec_stat_day.Stat.stat_v1k_avg );
		query.bind( col++, Rec_stat_day.Stat.stat_v1k_avg_num );

		query.bind( col++, Rec_stat_day.v1k_valid_00_11A );
		query.bind( col++, Rec_stat_day.v1k_valid_12_23B );

		for( int i=0; i<48; ++i )
		{
			query.bind( col++, Rec_stat_day.v1k[i] );
		}

		query.exec();
	}

	return true;
}

// 保存月统计数据
bool SQLite_DBDataSave::DBSave_stat_month( const struRec_stat_month &Rec_stat_month )
{
	if( !this->db )
	{
		return false;
	}

	if( DBIsBeing_stat_month( Rec_stat_month ) )
	{
		//int id = query.getColumn(0);
		//SQLite::Statement query(*this->db,"UPDATE ");
		//query.bind(1, );
		//query.exec();
	}
	else
	{
		SQLite::Statement query( *this->db, "INSERT INTO stat_month VALUES(NULL,"\
			":dev_type,:dev_id,:address_type,:address_id,"\
			":stat_valid,:stat_v1k_max_dt,:stat_v1k_max,:stat_v1k_min_dt,:stat_v1k_min,:stat_v1k_avg,:stat_v1k_avg_num)" );

		int col = 1;
		query.bind( col++, Rec_stat_month.key.AddrObjKey.dev_type );
		query.bind( col++, Rec_stat_month.key.AddrObjKey.dev_id );
		query.bind( col++, Rec_stat_month.key.AddrObjKey.address_type );
		query.bind( col++, Rec_stat_month.key.AddrObjKey.address_id );

		query.bind( col++, Rec_stat_month.Stat.stat_valid );
		query.bind( col++, Rec_stat_month.Stat.stat_v1k_max_dt );
		query.bind( col++, Rec_stat_month.Stat.stat_v1k_max );
		query.bind( col++, Rec_stat_month.Stat.stat_v1k_min_dt );
		query.bind( col++, Rec_stat_month.Stat.stat_v1k_min );
		query.bind( col++, Rec_stat_month.Stat.stat_v1k_avg );
		query.bind( col++, Rec_stat_month.Stat.stat_v1k_avg_num );

		query.exec();
	}

	return true;
}


//=====================================================
// DataStoreMgr

CDataStoreMgr::CDataStoreMgr(void)
{
	m_lastdo_DBMgr_Check = 0;
	CheckCreateDB( g_GetUTCTime() );
}

CDataStoreMgr::~CDataStoreMgr(void)
{
	//...释放m_lstDBDataSaveMgr
}

std::string CDataStoreMgr::CreateDBNameForSpecDt( const time_t &t )
{
	struGSTime dt;
	if( !g_UTCTime_To_struGSTime( t, dt ) )
		return std::string("");

	return CreateDBNameForSpecDt( dt );
}

std::string CDataStoreMgr::CreateDBNameForSpecDt( const struGSTime &dt )
{
	if( !dt.IsValidYearMonthOnly() )
		return std::string("");

	char dbname[256];
/* jyc20170227 modify
#if defined(defTEST_UseTestChangeDb)
	//snprintf( dbname, sizeof(dbname), "test7b_%04d%02d%02d.db", dt.Year, dt.Month, dt.Day );
	snprintf( dbname, sizeof(dbname), "test7b_%04d%02d.db", dt.Year, dt.Month );
#else
	snprintf( dbname, sizeof(dbname), "gsdata%04d%02d.db", dt.Year, dt.Month );
	//snprintf( dbname, sizeof(dbname), "test7b_%04d%02d.db", dt.Year, dt.Month );
#endif
*/
	snprintf( dbname, sizeof(dbname), "gsdata%04d%02d.db", dt.Year, dt.Month );

	return std::string(dbname);
}

std::string CDataStoreMgr::CreateDBNameForCur()
{
	return CreateDBNameForSpecDt( g_GetUTCTime() );
}

bool CDataStoreMgr::IsSameDBSave( const time_t utctimeA, const time_t utctimeB )
{
	return g_IsSameMonth( utctimeA, utctimeB );
}

bool CDataStoreMgr::IsSameDBSave( const struGSTime &dtA, const struGSTime &dtB )
{
	return g_IsSameMonth( dtA, dtB );
}

// 综合整理成查询结果数据
// create_lst_query_v1k: 创建时间点数据链表；
// spanhour: 取值间隔小时值，其中0为半小时；
// spanrate: 取值变化率，变化率小于此值时过滤掉；
// rateForType: 变化率判断按监测点类型计算判断，例如风速按风速级别；
// remove_invalid: 移除无效数据，但会保留过渡的无效数据作为过渡标志。
defGSReturn CDataStoreMgr::Stat_mapRec_stat_day_To_QueryStat( const defmapRec_stat_day &mapRec_stat_day, stru_QueryStat &ResultStat, const bool create_lst_query_v1k, const int spanhour, const int spanrate, const bool rateForType, const bool remove_invalid, const bool doQueryMax )
{
	time_t first_day_begin_time = 0;
	bool hasvalid = false;
	bool prev_valid = true;
	stru_QueryV1K prev_QueryV1K;
	for( defmapRec_stat_day::const_iterator it=mapRec_stat_day.begin(); it!=mapRec_stat_day.end(); ++it )
	{
		const struRec_stat_day *pDay = it->second;

		if( !pDay->Stat.stat_valid )
		{
			continue;
		}

		// 统计
		// 总数累计，后面计算平均值
		ResultStat.temp_v1k_sum += double(pDay->Stat.stat_v1k_avg) * pDay->Stat.stat_v1k_avg_num;
		ResultStat.Stat.stat_v1k_avg_num += pDay->Stat.stat_v1k_avg_num;

		g_AnalyseStat( ResultStat.Stat, pDay->Stat );

		// 数据记录
		const time_t day_begin = g_struGSTime_To_UTCTime( struGSTime( pDay->key.dt_year, pDay->key.dt_month, pDay->key.dt_day ) );

		if( !first_day_begin_time )
		{
			first_day_begin_time = day_begin;
		}

		if( !create_lst_query_v1k )
			continue;

		const std::bitset<24> bit_valid_A(pDay->v1k_valid_00_11A);
		const std::bitset<24> bit_valid_B(pDay->v1k_valid_12_23B);

		int spannum = spanhour*2;
		if( spannum<=0 ) spannum = 1;
		for( int i=0; i<48; i+=spannum )
		{
			const time_t curv_time = day_begin + i*30*60;

			if( curv_time < ResultStat.data_dt_begin )
				continue;

			if( curv_time > ResultStat.data_dt_end )
				continue;

			const bool v1k_valid = ( i<24 ) ? bit_valid_A.test( i ) : bit_valid_B.test( i-24 );

			// 第1个有效数据之前的都不要
			if( v1k_valid )
			{
				if( !hasvalid )
				{
					hasvalid = true;
				}
			}
			else
			{
				if( !hasvalid )
				{
					continue;
				}
			}

			bool doAdd = true;
			if( remove_invalid )
			{
				if( !prev_valid && !v1k_valid )
				{
					doAdd = false;
				}
			}

			stru_QueryV1K curQueryV1K( curv_time, v1k_valid?defDataFlag_Norm:defDataFlag_Invalid, pDay->v1k[i] );

			// 去除变化率低的数据
			if( doAdd && spanrate )
			{
				if( prev_QueryV1K.data_dt )
				{
					if( prev_QueryV1K.v1k_valid == curQueryV1K.v1k_valid )
					{
						if( rateForType && g_AddrTypeValue_hasTypeValue(ResultStat.AddrObjKey.address_type) )
						{
							if( abs( g_AddrTypeValue_getTypeValue(ResultStat.AddrObjKey.address_type,curQueryV1K.v1k)-g_AddrTypeValue_getTypeValue(ResultStat.AddrObjKey.address_type,prev_QueryV1K.v1k) ) < spanrate )
							{
								continue;
							}
						}
						else
						{
							if( abs(curQueryV1K.v1k-prev_QueryV1K.v1k) < spanrate )
							{
								doAdd = false;
							}
						}
					}
				}
			}

			if( doAdd )
			{
				ResultStat.lst_query_v1k[curv_time] = curQueryV1K;
				prev_QueryV1K = curQueryV1K;

				if( doQueryMax )
				{
					const int SYS_QueryLstMax = RUNCODE_Get(defCodeIndex_SYS_QueryLstMax);
					if( ResultStat.lst_query_v1k.size() > SYS_QueryLstMax )
					{
						return defGSReturn_ResLimit;
					}
				}
			}

			prev_valid = v1k_valid;
		}
	}

	// 补充中间整天没有的数据
	if( create_lst_query_v1k && hasvalid )
	{
		QueryStatFixInvalidFullDay( ResultStat, mapRec_stat_day, first_day_begin_time );
	}

	// 计算平均值
	ResultStat.Stat.calc_avg( ResultStat.temp_v1k_sum );

	return defGSReturn_Success;
}

void CDataStoreMgr::QueryStatFixInvalidFullDay( stru_QueryStat &ResultStat, const defmapRec_stat_day &mapRec_stat_day, const time_t first_day_begin_time )
{
	// 补充中间整天没有的数据
	const int maxday = ( ResultStat.data_dt_end - ResultStat.data_dt_begin )/(24*60*60) + 2;

	for( int i=1; i<maxday; ++i ) // 最多这么多次
	{
		const time_t curv_time = first_day_begin_time + i*24*60*60;

		struGSTime curv_dt;
		g_UTCTime_To_struGSTime( curv_time, curv_dt );

		struRec_stat_day::struKey key ( ResultStat.AddrObjKey, curv_dt.Year, curv_dt.Month, curv_dt.Day );

		if( curv_time < ResultStat.data_dt_end
			&& mapRec_stat_day.find( key ) == mapRec_stat_day.end()
			)
		{
			ResultStat.lst_query_v1k[curv_time] = stru_QueryV1K( curv_time );
		}
	}
}

// 综合整理成查询结果数据，分钟级间隔，需分析原始数据
// spanmin: 取值间隔分钟值，1-60
defGSReturn CDataStoreMgr::Stat_lst_src_v1k_To_QueryStat_ForSpanmin( const defmapV1k &lst_src_v1k, stru_QueryStat &ResultStat, const int spanmin, const int spanrate, const bool rateForType, const bool remove_invalid, const bool doQueryMax, const bool retime_maxmin )
{
	if( lst_src_v1k.empty() )
	{
		ResultStat.Stat.stat_valid = 0;
		return defGSReturn_Success;
	}

	const time_t curUTCTime = g_GetUTCTime();

	// 先统计
	for( defmapV1k::const_iterator it = lst_src_v1k.begin(); it!=lst_src_v1k.end(); ++it )
	{
		if( !it->second.v1k_valid )
			continue;

		const time_t curTime = it->first;
		const int curValue = it->second.v1k;

		// 统计
		// 总数累计，后面计算平均值
		ResultStat.temp_v1k_sum += curValue;
		ResultStat.Stat.stat_v1k_avg_num++;

		g_AnalyseStat( ResultStat.Stat, curTime, curValue, ResultStat.data_dt_begin, ResultStat.data_dt_end );
	}

	// 计算平均值
	ResultStat.Stat.calc_avg( ResultStat.temp_v1k_sum );


	// 后计算间隔值
	bool hasvalid = false;
	bool prev_valid = true;
	stru_QueryV1K prev_QueryV1K;

	int spannum = spanmin*60;
	if( spannum<=0 ) spannum = 15*60;

	time_t stat_dt_end = ResultStat.data_dt_end;
	if( stat_dt_end > curUTCTime )
		stat_dt_end = curUTCTime;

	for( time_t curstatTime=ResultStat.data_dt_begin; curstatTime<stat_dt_end; curstatTime+=spannum )
	{
		// 记录得到的原始时间和值
		stru_V1K src_get_v1k;

		const bool v1k_valid = curstatTime>curUTCTime ? false : SQLite_DBDataSave::GetValueForSpecTime_lst( curstatTime, lst_src_v1k, src_get_v1k ) && src_get_v1k.v1k_valid;

		// 第1个有效数据之前的都不要
		if( v1k_valid )
		{
			if( !hasvalid )
			{
				hasvalid = true;
			}
		}
		else
		{
			if( !hasvalid )
			{
				continue;
			}
		}

		bool doAdd = true;
		if( remove_invalid )
		{
			if( !prev_valid && !v1k_valid )
			{
				doAdd = false;
			}
		}

		stru_QueryV1K curQueryV1K( curstatTime, src_get_v1k.v1k_valid, src_get_v1k.v1k );

		// 去除变化率低的数据
		if( doAdd && spanrate )
		{
			if( prev_QueryV1K.data_dt )
			{
				if( prev_QueryV1K.v1k_valid == curQueryV1K.v1k_valid )
				{
					if( rateForType && g_AddrTypeValue_hasTypeValue(ResultStat.AddrObjKey.address_type) )
					{
						if( abs( g_AddrTypeValue_getTypeValue(ResultStat.AddrObjKey.address_type,curQueryV1K.v1k)-g_AddrTypeValue_getTypeValue(ResultStat.AddrObjKey.address_type,prev_QueryV1K.v1k) ) < spanrate )
						{
							continue;
						}
					}
					else
					{
						if( abs(curQueryV1K.v1k-prev_QueryV1K.v1k) < spanrate )
						{
							doAdd = false;
						}
					}
				}
			}
		}

		if( doAdd )
		{
			ResultStat.lst_query_v1k[curstatTime] = curQueryV1K;
			prev_QueryV1K = curQueryV1K;

			if( doQueryMax )
			{
				const int SYS_QueryLstMax = RUNCODE_Get(defCodeIndex_SYS_QueryLstMax);
				if( ResultStat.lst_query_v1k.size() > SYS_QueryLstMax )
				{
					return defGSReturn_ResLimit;
				}
			}
		}

		prev_valid = v1k_valid;

		// 最大最小值是否在原始时间点上，与原始值相等
		if( retime_maxmin && src_get_v1k.data_dt && src_get_v1k.v1k_valid )
		{
			// 最大值比较
			if( ResultStat.Stat.stat_v1k_max_dt )
			{
				// 值相等并且时间更早
				if( src_get_v1k.v1k == ResultStat.Stat.stat_v1k_max && curstatTime<ResultStat.Stat.stat_v1k_max_dt )
				{
					ResultStat.Stat.stat_v1k_max_dt = curstatTime;//src_get_data_dt;
					ResultStat.Stat.stat_v1k_max = src_get_v1k.v1k;
				}
			}

			// 最小值比较
			if( ResultStat.Stat.stat_v1k_min_dt )
			{
				// 值相等并且时间更早
				if( src_get_v1k.v1k == ResultStat.Stat.stat_v1k_min && curstatTime<ResultStat.Stat.stat_v1k_min_dt )
				{
					ResultStat.Stat.stat_v1k_min_dt = curstatTime;//src_get_data_dt;
					ResultStat.Stat.stat_v1k_min = src_get_v1k.v1k;
				}
			}
		}
	}

	return defGSReturn_Success;
}

// 综合整理成查询结果数据
bool CDataStoreMgr::Stat_mapRec_stat_month_To_QueryStat( const defmapRec_stat_month &mapRec_stat_month, stru_QueryStat &ResultStat )
{
	time_t first_day_begin_time = 0;
	bool hasvalid = false;
	bool prev_valid = true;
	for( defmapRec_stat_month::const_iterator it=mapRec_stat_month.begin(); it!=mapRec_stat_month.end(); ++it )
	{
		const struRec_stat_month *pMonth = it->second;

		if( !pMonth->Stat.stat_valid )
		{
			continue;
		}

		// 统计
		// 总数累计，后面计算平均值
		ResultStat.temp_v1k_sum += double(pMonth->Stat.stat_v1k_avg) * pMonth->Stat.stat_v1k_avg_num;
		ResultStat.Stat.stat_v1k_avg_num += pMonth->Stat.stat_v1k_avg_num;

		g_AnalyseStat( ResultStat.Stat, pMonth->Stat );
	}
	
	// 计算平均值
	ResultStat.Stat.calc_avg( ResultStat.temp_v1k_sum );

	return true;
}

void CDataStoreMgr::CheckCreateDB( const time_t utctime )
{
	struGSTime dt;
	g_UTCTime_To_struGSTime( utctime, dt );

	AutoRelease_getDBDataSave autodb( this );
	SQLite_DBDataSave *pDBSave = autodb.get( dt, true, "CheckCreateDB", false ); 
}

// 获取指定日期的数据库实例
SQLite_DBDataSave* CDataStoreMgr::GetDBSave_lock( const struGSTime &dt, const bool DoAutoCreate, const char *pinfo, const bool check_file_exists )
{
	const std::string dbname = CDataStoreMgr::CreateDBNameForSpecDt( dt );

	if( !dbname.empty() )
	{
		SQLite_DBDataSave *stat_DBDataSave = DBMgr_get( dbname );

		if( !stat_DBDataSave )
		{
			//jyc20170227 delete
			//const std::string strsub_full = g_iotstore_createdir( defIotStoreDir_Data );
			
			bool doCreateDB = true;
			if( check_file_exists )
			{
				//const std::string path = strsub_full + "\\" + dbname;
				const std::string path = ROOTDIR + dbname; //jyc20170227 modify

				if( !file_exists(path.c_str()) )
				{
					doCreateDB = false;
					LOGMSG( "DataStoreMgr(%s) DBFile Not Exists, \"%s\"\r\n", pinfo?pinfo:"", dbname.c_str() );
				}
			}

			if( doCreateDB )
			{
				stat_DBDataSave = new SQLite_DBDataSave( false, dbname, dt, DoAutoCreate );
				if( stat_DBDataSave && stat_DBDataSave->get_db() )
				{
					if( !DBMgr_set( stat_DBDataSave ) )
					{
						delete stat_DBDataSave;
						stat_DBDataSave = NULL;
					}

					return DBMgr_get( dbname );
				}
				else
				{
					delete stat_DBDataSave;
					stat_DBDataSave = NULL;
				}
			}
		}

		return stat_DBDataSave;
	}

	return NULL;
}

bool CDataStoreMgr::DBMgr_set( SQLite_DBDataSave *pDBDataSave )
{
	if( !pDBDataSave )
		return false;

	gloox::util::MutexGuard( this->m_mutex_lstDBDataSaveMgr );

	std::map<std::string,SQLite_DBDataSave*>::const_iterator it = m_lstDBDataSaveMgr.find( pDBDataSave->get_dbname() );
	if( it != m_lstDBDataSaveMgr.end() )
	{
		return false;
	}

	m_lstDBDataSaveMgr.insert( std::make_pair( pDBDataSave->get_dbname(), pDBDataSave ) );
	LOGMSG( "DataSaveMgr_set insert new, size=%d\r\n", m_lstDBDataSaveMgr.size() );

	return true;
}

// 锁定按dt获取
SQLite_DBDataSave* CDataStoreMgr::DBMgr_get( const struGSTime &dt )
{
	gloox::util::MutexGuard( this->m_mutex_lstDBDataSaveMgr );

	return DBMgr_get_nolock( dt );
}

// 锁定按dbname获取
SQLite_DBDataSave* CDataStoreMgr::DBMgr_get( const std::string &dbname )
{
	gloox::util::MutexGuard( this->m_mutex_lstDBDataSaveMgr );

	return DBMgr_get_nolock( dbname );
}

void CDataStoreMgr::DBMgr_release( SQLite_DBDataSave *pDBDataSave )
{
	if( !pDBDataSave )
		return ;

	gloox::util::MutexGuard( this->m_mutex_lstDBDataSaveMgr );

	pDBDataSave->m_usecount--;
}

bool CDataStoreMgr::DBMgr_is_stat_day( const struGSTime &dt )
{
	gloox::util::MutexGuard( this->m_mutex_lstDBDataSaveMgr );

	SQLite_DBDataSave *pDBDataSave = DBMgr_get_nolock( dt );
	if( pDBDataSave )
	{
		return pDBDataSave->is_stat_day( dt.Day );
	}

	return false;
}

SQLite_DBDataSave* CDataStoreMgr::DBMgr_get_nolock( const struGSTime &dt )
{
	const std::string dbname = CDataStoreMgr::CreateDBNameForSpecDt( dt );

	return DBMgr_get_nolock( dbname );
}

SQLite_DBDataSave* CDataStoreMgr::DBMgr_get_nolock( const std::string &dbname )
{
	std::map<std::string,SQLite_DBDataSave*>::const_iterator it = m_lstDBDataSaveMgr.find( dbname );
	if( it != m_lstDBDataSaveMgr.end() )
	{
		it->second->m_usecount++;
		it->second->m_lastUseTs = timeGetTime();

		if( 0==it->second->m_lastUseTs )
		{
			it->second->m_lastUseTs++;
		}

		return it->second;
	}

	return NULL;
}

// 打印的运行对象信息
void CDataStoreMgr::DBMgr_Print()
{
	gloox::util::MutexGuard( this->m_mutex_lstDBDataSaveMgr );

	const uint32_t curts = timeGetTime();

	const size_t Count = m_lstDBDataSaveMgr.size();

	//LOGMSGEX( defLOGNAME, defLOG_INFO, "CDataStoreMgr::DBMgr_Print Count=%d\r\n", Count );

	if( !m_lstDBDataSaveMgr.empty() )
	{
		int i = 1;
		for( std::map<std::string,SQLite_DBDataSave*>::iterator it = m_lstDBDataSaveMgr.begin(); it!=m_lstDBDataSaveMgr.end(); ++it, ++i )
		{
			/*LOGMSGEX( defLOGNAME, defLOG_INFO, "(%d/%d)(%s) useCount=%d, lastUse=%u(%u)",
				i, Count,
				it->second->get_dbname().c_str(),
				it->second->m_usecount,
				it->second->m_lastUseTs,
				curts-it->second->m_lastUseTs
				);*/
		}
	}
}

int getuse_SYS_DBDataFileMax()
{
	const int SYS_DBDataFileMax = RUNCODE_Get( defCodeIndex_SYS_DBDataFileMax );

	return ( SYS_DBDataFileMax<3 ? 3:SYS_DBDataFileMax );
}

void CDataStoreMgr::DBMgr_Check( const bool CheckNow )
{
	if( CheckNow )
	{
		m_lastdo_DBMgr_Check = 0;
	}

	const DWORD dwstart = timeGetTime();

	const int SYS_DBDataFileMax = getuse_SYS_DBDataFileMax();

	if( m_lastdo_DBMgr_Check && timeGetTime()-m_lastdo_DBMgr_Check < 6*60*60*1000 )
	{
		LOGMSG( "DataStoreMgr::DBMgr_Check() max=%d\r\n", SYS_DBDataFileMax );
		return;
	}

	std::map<std::string,std::string> mapDBFile; //<name,path>
	//jyc20170227 notice
	const std::string strsub_full = std::string( defIotStoreDir_Main ) + "\\" + defIotStoreDir_Data;

	struGSTime dt;
	g_UTCTime_To_struGSTime( g_GetUTCTime(), dt );

	for( int i=0; i<50; ++i, dt.SubtractSelfMonth() )
	{
		const std::string dbname = CDataStoreMgr::CreateDBNameForSpecDt( dt );
		//const std::string path = strsub_full + "\\" + dbname;
		const std::string path = ROOTDIR + dbname; //jyc20170227 modify

		if( file_exists( path.c_str() ) )
		{
			mapDBFile[dbname] = path;
			//LOGMSG( "DataStoreMgr::DBMgr_Check() found %s", dbname.c_str() );
		}
	}

	LOGMSG( "DataStoreMgr::DBMgr_Check() found=%d, max=%d\r\n", mapDBFile.size(), SYS_DBDataFileMax );

	if( mapDBFile.size() <= SYS_DBDataFileMax )
	{
		m_lastdo_DBMgr_Check = timeGetTime();
		return;
	}

	//
	gloox::util::MutexGuard( this->m_mutex_lstDBDataSaveMgr );


	int sno = mapDBFile.size();
	std::map<std::string,std::string>::iterator it = mapDBFile.begin();
	while( mapDBFile.size() > SYS_DBDataFileMax )
	{
		if( it == mapDBFile.end() )
			break;

		sno--;

		std::map<std::string,SQLite_DBDataSave*>::iterator itUse = m_lstDBDataSaveMgr.find( it->first );
		if( itUse != m_lstDBDataSaveMgr.end() )
		{
			if( 0 != itUse->second->m_usecount )
			{
				if( sno > SYS_DBDataFileMax )//多更多时才接着分析
				{
					++it;
					continue;
				}
				else
				{
					break;
				}
			}
			else
			{
				delete (itUse->second);
				m_lstDBDataSaveMgr.erase( itUse );
			}
		}

		// delete file
		const BOOL ret = g_DeleteFileEx( it->second.c_str() );
		LOGMSG( "DBMgr_DoDeleteList() delete %s db=%s, dbmax=%d", ret?"success":"failed", it->second.c_str(), SYS_DBDataFileMax );
		mapDBFile.erase( it );
		it = mapDBFile.begin();
	}

	if( mapDBFile.size() <= SYS_DBDataFileMax )
	{
		m_lastdo_DBMgr_Check = timeGetTime();
	}

	LOGMSG( "DataStoreMgr::DBMgr_Check() End, usetime=%dms\r\n", timeGetTime()-dwstart );
}

// 检查统计
void CDataStoreMgr::CheckStat()
{
	LOGMSG( "DataStoreMgr::CheckStat()\r\n" );

	const time_t curUTCTime = g_GetUTCTime();
	struGSTime curGSTime;
	if( !g_UTCTime_To_struGSTime( curUTCTime, curGSTime ) )
	{
		return;
	}

	const int curGSTimemin = curGSTime.Hour*100 + curGSTime.Minute;
	if( curGSTimemin<110 || curGSTimemin>255 ) // 01:10 - 02:55 在这个时间段内统计
		return;

	DBMgr_Check();

	// 计算出前一天
	const time_t prevdayUTCTime = curUTCTime-24*60*60;
	struGSTime prevdayGSTime;
	if( !g_UTCTime_To_struGSTime( prevdayUTCTime, prevdayGSTime ) )
	{
		return;
	}

	if( 1==curGSTime.Day )
	{
		AutoRelease_getDBDataSave autodb( this );
		SQLite_DBDataSave *pDBSave = autodb.get( prevdayGSTime, false, "CheckStat", true );

		if( pDBSave )
		{
			// 先统计天
			if( IsSameDBSave( prevdayGSTime, pDBSave->get_dt() ) )
			{
				pDBSave->DoStat_day_all();
			}

			// 后统计月
			if( !pDBSave->is_stat_month() )
			{
				const DWORD dwstart = timeGetTime();

				if( !pDBSave->DoStat_month_all() )
				{
					LOGMSG( "DataStoreMgr DoStat_month_all failed! usetime=%dms\r\n", timeGetTime()-dwstart );
				}
			}
		}
	}
	else
	{
		if( !DBMgr_is_stat_day( prevdayGSTime ) )
		{
			AutoRelease_getDBDataSave autodb( this );
			SQLite_DBDataSave *pDBSave = autodb.get( prevdayGSTime, false, "CheckStat", true );
			if( pDBSave )
			{
				const short stat_daymax = prevdayGSTime.Day;// 统计的天，0不统计
				pDBSave->DoStat_day_all_specmax( stat_daymax );
			}
		}
	}
}

void CDataStoreMgr::insertdata( const defvecDataSave &vecDataSave )
{
	if( vecDataSave.empty() )
		return;

	struGSTime dt;
	g_UTCTime_To_struGSTime( vecDataSave[0]->data_dt, dt );

	AutoRelease_getDBDataSave autodb( this );
	SQLite_DBDataSave *pDBSave = autodb.get( dt, true, "insertdata", false );

	if( pDBSave )
	{
		pDBSave->insertdata( vecDataSave );
	}
}

// return do save
bool CDataStoreMgr::insertdata_CheckSaveInvalid( const GSIOTAddrObjKey &AddrObjKey, const bool valid )
{
	// 有效都存
	if( valid )
	{
		if( !m_mapCheckInvalid.empty() )
		{
			// 移除之前无效记录
			//std::map<GSIOTAddrObjKey,int>::const_iterator it = m_mapCheckInvalid.find( AddrObjKey );
			std::map<GSIOTAddrObjKey,int>::iterator it = m_mapCheckInvalid.find( AddrObjKey );
			if( it != m_mapCheckInvalid.end() )
			{
				m_mapCheckInvalid.erase( it );
			}
		}

		return false;
	}

	std::map<GSIOTAddrObjKey,int>::iterator it = m_mapCheckInvalid.find( AddrObjKey );
	if( it != m_mapCheckInvalid.end() )
	{
		if( it->second<99 )
			it->second++;

		if( 9==it->second )
			return true;
	}
	else
	{
		m_mapCheckInvalid[AddrObjKey] = 1;
	}

	return false;
}

// 查询分析临时统计当前天
defGSReturn CDataStoreMgr::QueryAnalyse_stat_NowDay( struRec_stat_day &Rec_stat_day )
{
	const DWORD dwstart = timeGetTime();

	const time_t curUTCTime = g_GetUTCTime();
	struGSTime curdt;
	g_UTCTime_To_struGSTime( curUTCTime, curdt );

	AutoRelease_getDBDataSave autodb( this );
	SQLite_DBDataSave *pDBSave = autodb.get( curdt, false, "QueryAnalyse_stat_NowDay", true );
	if( !pDBSave )
	{
		return defGSReturn_DBNoExist;
	}

	const defGSReturn ret = pDBSave->DBLoad_histdata_SpecObj_ForDay( Rec_stat_day.key.AddrObjKey, curdt.Day, Rec_stat_day.lst_src_v1k );
	if( macGSFailed(ret) )
	{
		LOGMSG( "QueryAnalyse_stat_NowDay%s\r\n", Rec_stat_day.GetBaseInfo().c_str() );
		return ret;
	}

	SQLite_DBDataSave::DoStat_day_specday_specone( Rec_stat_day );

	LOGMSG( "QueryAnalyse_stat_NowDay usetime=%dms, %s\r\n", timeGetTime()-dwstart, Rec_stat_day.GetBaseInfo().c_str() );

	return defGSReturn_Success;
}

// 查询分析临时统计当前月
defGSReturn CDataStoreMgr::QueryAnalyse_stat_NowMonth( struRec_stat_month &Rec_stat_month, defmapRec_stat_day &mapRec_stat_day, const bool getday, const bool doCreateWhenNotFound )
{
	const DWORD dwstart = timeGetTime();

	const time_t curUTCTime = g_GetUTCTime();
	struGSTime curdt;
	g_UTCTime_To_struGSTime( curUTCTime, curdt );

	AutoRelease_getDBDataSave autodb( this );
	SQLite_DBDataSave *pDBSave = autodb.get( curdt, false, "QueryAnalyse_stat_NowMonth", true );
	if( !pDBSave )
	{
		return defGSReturn_DBNoExist;
	}

	if( doCreateWhenNotFound )
	{
		pDBSave->CheckAndDoStat_day();
	}

	defGSReturn ret = pDBSave->DBLoad_stat_day_SpecObj_DayRange( Rec_stat_month.key.AddrObjKey, 1, curdt.Day, mapRec_stat_day );
	if( macGSFailed(ret) )
	{
		if( !g_isNoDBRec(ret) )
		{
			g_delete_mapRec_stat_day( mapRec_stat_day );
			LOGMSG( "QueryAnalyse_stat_NowMonth LoadDB err, %s\r\n", Rec_stat_month.GetBaseInfo().c_str() );
			return ret;
		}
	}

	struRec_stat_day *pRec_stat_day = new struRec_stat_day( struRec_stat_day::struKey( Rec_stat_month.key.AddrObjKey, curdt.Year, curdt.Month, curdt.Day ) );

	ret = QueryAnalyse_stat_NowDay( *pRec_stat_day );
	if( macGSFailed(ret) )
	{
		delete pRec_stat_day;

		if( !g_isNoDBRec(ret) )
		{
			g_delete_mapRec_stat_day( mapRec_stat_day );
			LOGMSG( "QueryAnalyse_stat_NowMonth LoadDB_today err, %s\r\n", Rec_stat_month.GetBaseInfo().c_str() );
			return ret;
		}
	}
	else
	{
		mapRec_stat_day[pRec_stat_day->key] = pRec_stat_day;
	}

	// 统计
	for( defmapRec_stat_day::const_iterator itD=mapRec_stat_day.begin(); itD!=mapRec_stat_day.end(); ++itD )
	{
		const struRec_stat_day *pDay = itD->second;

		if( !pDay->Stat.stat_valid )
			continue;
		
		// 总数累计，后面计算平均值
		Rec_stat_month.temp_v1k_sum += double(pDay->Stat.stat_v1k_avg) * pDay->Stat.stat_v1k_avg_num;
		Rec_stat_month.Stat.stat_v1k_avg_num += pDay->Stat.stat_v1k_avg_num;

		g_AnalyseStat( Rec_stat_month.Stat, pDay->Stat );
	}
	
	Rec_stat_month.Stat.calc_avg( Rec_stat_month.temp_v1k_sum );

	if( !getday )
	{
		g_delete_mapRec_stat_day( mapRec_stat_day );
	}

	LOGMSG( "QueryAnalyse_stat_NowMonth usetime=%dms, %s\r\n", timeGetTime()-dwstart, Rec_stat_month.GetBaseInfo().c_str() );

	return defGSReturn_Success;
}

// 查询 从某个近期时间到现在的数据情况
defGSReturn CDataStoreMgr::QueryStatData_ForTimeToNow( const time_t data_dt_begin, stru_QueryStat &ResultStat, const bool create_lst_query_v1k, const int spanhour, const int spanrate, const bool rateForType, const bool remove_invalid )
{
	const time_t NowUTCTime = g_GetUTCTime();

	defmapRec_stat_day mapRec_stat_day;
	const defGSReturn ret = QueryStatData_ForTime( data_dt_begin, NowUTCTime, ResultStat, mapRec_stat_day, false, create_lst_query_v1k, spanhour, spanrate, rateForType, remove_invalid );
	g_delete_mapRec_stat_day( mapRec_stat_day );
	return ret;
}

// 查询 从某段指定时间的数据情况统计
defGSReturn CDataStoreMgr::QueryStatData_ForTime( const time_t data_dt_begin, const time_t data_dt_end, stru_QueryStat &ResultStat, defmapRec_stat_day &mapRec_stat_day, const bool create_stathour, const bool create_lst_query_v1k, const int spanhour, const int spanrate, const bool rateForType, const bool remove_invalid )
{
	const int c_maxday = 55;

	const DWORD dwstart = timeGetTime();

	const time_t curUTCTime = g_GetUTCTime();

	ResultStat.data_dt_begin = data_dt_begin;
	ResultStat.data_dt_end = data_dt_end;
	if( ResultStat.data_dt_begin > curUTCTime )
	{
		LOGMSG( "Query_SpecObj_ForTimeToNow begin>curtime err, %s\r\n", ResultStat.GetBaseInfo().c_str() );
		return defGSReturn_OverTimeRange;
	}

	if( ResultStat.data_dt_end > curUTCTime )
	{
		ResultStat.data_dt_end = curUTCTime;
	}

	if( ResultStat.data_dt_begin > ResultStat.data_dt_end )
	{
		LOGMSG( "Query_SpecObj_ForTimeToNow begin>end err, %s\r\n", ResultStat.GetBaseInfo().c_str() );
		return defGSReturn_ErrParam;
	}

	// 跨度太长 超过n天
	if( ResultStat.data_dt_end - ResultStat.data_dt_begin > c_maxday*24*60*60 )
	{
		LOGMSG( "Query_SpecObj_ForTimeToNow time long>%dday err, %s\r\n", c_maxday, ResultStat.GetBaseInfo().c_str() );
		return defGSReturn_OverTimeRange;
	}

	struGSTime GSTimeBegin;
	if( !g_UTCTime_To_struGSTime( ResultStat.data_dt_begin, GSTimeBegin ) )
	{
		LOGMSG( "Query_SpecObj_ForTimeToNow time begin err, %s\r\n", ResultStat.GetBaseInfo().c_str() );
		return defGSReturn_Err;
	}

	struGSTime GSTimeEnd;
	if( !g_UTCTime_To_struGSTime( ResultStat.data_dt_end, GSTimeEnd ) )
	{
		LOGMSG( "Query_SpecObj_ForTimeToNow time end err, %s\r\n", ResultStat.GetBaseInfo().c_str() );
		return defGSReturn_Err;
	}

	AutoRelease_getDBDataSave autodbA( this );
	SQLite_DBDataSave *pDBSaveA = autodbA.get( GSTimeBegin, false, "Query_SpecObj_ForTimeToNow_A", true );

	AutoRelease_getDBDataSave autodbB( this );
	SQLite_DBDataSave *pDBSaveB = NULL;

	// 是否跨月
	if( !IsSameDBSave(GSTimeBegin, GSTimeEnd) )
	{
		pDBSaveB = autodbB.get( GSTimeEnd, false, "Query_SpecObj_ForTimeToNow_B", true );
	}

	if( !pDBSaveA && !pDBSaveB )
	{
		LOGMSG( "Query_SpecObj_ForTimeToNow not found DB, %s\r\n", ResultStat.GetBaseInfo().c_str() );
		return defGSReturn_DBNoExist;
	}

	// 载入时间范围内数据，由于时间范围可自由选择提供，最前面和最后面的可能不是完整的一天

	if( pDBSaveA )
	{
		const defGSReturn ret = pDBSaveA->DBLoad_histdata_SpecObj_mapRecStatDay_ForTime( ResultStat.AddrObjKey, ResultStat.data_dt_begin-defValidRange_prev, ResultStat.data_dt_end+defValidRange_next, mapRec_stat_day );
		if( macGSFailed(ret) )
		{
			g_delete_mapRec_stat_day( mapRec_stat_day );
			LOGMSG( "Query_SpecObj_ForTimeToNow LoadDB_A err, %s\r\n", ResultStat.GetBaseInfo().c_str() );
			return ret;
		}
	}

	if( pDBSaveB )
	{
		const defGSReturn ret = pDBSaveB->DBLoad_histdata_SpecObj_mapRecStatDay_ForTime( ResultStat.AddrObjKey, ResultStat.data_dt_begin-defValidRange_prev, ResultStat.data_dt_end+defValidRange_next, mapRec_stat_day );
		if( macGSFailed(ret) )
		{
			g_delete_mapRec_stat_day( mapRec_stat_day );
			LOGMSG( "Query_SpecObj_ForTimeToNow LoadDB_B err, %s\r\n", ResultStat.GetBaseInfo().c_str() );
			return ret;
		}
	}

	// 对每一天内的数据分别统计计算分析
	for( defmapRec_stat_day::const_iterator it=mapRec_stat_day.begin(); it!=mapRec_stat_day.end(); ++it )
	{
		struRec_stat_day *pRec_stat_day = it->second;
		SQLite_DBDataSave::DoStat_day_specday_specone( *pRec_stat_day, true, data_dt_begin, data_dt_end );

		if( create_stathour )
		{
			const time_t day_begin = g_struGSTime_To_UTCTime( struGSTime(pRec_stat_day->key.dt_year, pRec_stat_day->key.dt_month, pRec_stat_day->key.dt_day) );
			const time_t day_end = g_struGSTime_To_UTCTime( struGSTime(pRec_stat_day->key.dt_year, pRec_stat_day->key.dt_month, pRec_stat_day->key.dt_day, 23, 59, 59, 999) );

			SQLite_DBDataSave::DoStat_hour_lst( day_begin, day_end, pRec_stat_day->lst_src_v1k, pRec_stat_day->lst_stathour );
		}
	}

	const defGSReturn ret = Stat_mapRec_stat_day_To_QueryStat( mapRec_stat_day, ResultStat, create_lst_query_v1k, spanhour, spanrate, rateForType, remove_invalid, true );

	//g_delete_mapRec_stat_day( mapRec_stat_day );

	ResultStat.data_dt_begin = data_dt_begin;
	ResultStat.data_dt_end = data_dt_end;

	LOGMSG( "Query_SpecObj_ForTimeToNow ret=%d, usetime=%dms, %s, spanhour=%dh, spanrate=%d, rateForType=%d\r\n", ret, timeGetTime()-dwstart, ResultStat.GetBaseInfo().c_str(), spanhour, spanrate, rateForType );

	return ret;
}

// 查询 从某个近期时间到现在的数据情况，分钟级间隔，需分析原始数据
defGSReturn CDataStoreMgr::QueryStatData_ForTimeToNow_ForSpanmin( const time_t data_dt_begin, stru_QueryStat &ResultStat, const int spanmin, const int spanrate, const bool rateForType, const bool remove_invalid )
{
	const time_t NowUTCTime = g_GetUTCTime();

	return QueryStatData_ForTime_ForSpanmin( data_dt_begin, NowUTCTime, ResultStat, spanmin, spanrate, rateForType, remove_invalid );
}

// 查询 从某段指定时间的数据情况统计，分钟级间隔，需分析原始数据
defGSReturn CDataStoreMgr::QueryStatData_ForTime_ForSpanmin( const time_t data_dt_begin, const time_t data_dt_end, stru_QueryStat &ResultStat, const int spanmin, const int spanrate, const bool rateForType, const bool remove_invalid )
{
	const bool create_lst_query_v1k = true;
	if( 30==spanmin || 60==spanmin )
	{
		defmapRec_stat_day mapRec_stat_day;
		const defGSReturn ret = QueryStatData_ForTime( data_dt_begin, data_dt_end, ResultStat, mapRec_stat_day, false, create_lst_query_v1k, spanmin/60, spanrate, rateForType, remove_invalid );
		g_delete_mapRec_stat_day( mapRec_stat_day );
		return ret;
	}

	const DWORD dwstart = timeGetTime();

	ResultStat.data_dt_begin = data_dt_begin;
	ResultStat.data_dt_end = data_dt_end;

	defmapV1k lst_v1k;
	defGSReturn ret = QuerySrcValueLst_ForTimeRange( ResultStat.AddrObjKey, ResultStat.data_dt_begin, ResultStat.data_dt_end, lst_v1k, 0, false );
	if( macGSFailed(ret) )
	{
		lst_v1k.clear();
		LOGMSG( "QueryStatData_ForTime_ForSpanmin LoadDB_B err, %s\r\n", ResultStat.AddrObjKey.get_str().c_str() );
		return ret;
	}

	return Stat_lst_src_v1k_To_QueryStat_ForSpanmin( lst_v1k, ResultStat, spanmin, spanrate, rateForType, remove_invalid, true, true );
}

// 查询 从某段指定时间的数据情况统计，分钟统计
defGSReturn CDataStoreMgr::QueryStatMinute_ForTime( stru_QueryStat &ResultStat, defmapstruStat &lst_statminute, const int spanminute )
{
	if( ResultStat.data_dt_end == ResultStat.data_dt_begin )
	{
		ResultStat.data_dt_end = ResultStat.data_dt_begin + 59*60 + 59;
	}

	const int c_maxhour = 3;
	if( ResultStat.data_dt_end - ResultStat.data_dt_begin > c_maxhour*60*60 )
	{
		LOGMSG( "QueryStatMinute_ForTime time long>%dh err, %s\r\n", c_maxhour, ResultStat.AddrObjKey.get_str().c_str() );
		return defGSReturn_OverTimeRange;
	}

	const time_t curUTCTime = g_GetUTCTime();

	const DWORD dwstart = timeGetTime();
	
	defmapV1k lst_v1k;
	defGSReturn ret = QuerySrcValueLst_ForTimeRange( ResultStat.AddrObjKey, ResultStat.data_dt_begin-defValidRange_prev, ResultStat.data_dt_end+defValidRange_next, lst_v1k, 0, false );
	if( macGSFailed( ret ) )
	{
		lst_v1k.clear();
		LOGMSG( "QueryStatMinute_ForTime err, %s\r\n", ResultStat.AddrObjKey.get_str().c_str() );
		return ret;
	}

	SQLite_DBDataSave::DoStat_minute_lst( ResultStat.data_dt_begin, ResultStat.data_dt_end>curUTCTime?curUTCTime:ResultStat.data_dt_end, lst_v1k, lst_statminute, spanminute );

	for( defmapstruStat::const_iterator it = lst_statminute.begin(); it!=lst_statminute.end(); ++it )
	{
		// 统计
		// 总数累计，后面计算平均值
		ResultStat.temp_v1k_sum += double( it->second.stat_v1k_avg ) * it->second.stat_v1k_avg_num;
		ResultStat.Stat.stat_v1k_avg_num += it->second.stat_v1k_avg_num;

		g_AnalyseStat( ResultStat.Stat, it->second );
	}

	// 计算平均值
	ResultStat.Stat.calc_avg( ResultStat.temp_v1k_sum );

	return ret;
}

// 按指定 起始天至结束天 范围 查询日统计
defGSReturn CDataStoreMgr::QueryStatDayRec_ForDayRange( const time_t day_begin, const time_t day_end, stru_QueryStat &ResultStat, defmapRec_stat_day &mapRec_stat_day, const bool doAnalyseNowDay, const bool doCreateWhenNotFound, const bool create_lst_query_v1k, const int spanhour, const int spanrate, const bool rateForType, const bool remove_invalid, const bool doStat )
{
	const DWORD dwstart = timeGetTime();

	const time_t curUTCTime = g_GetUTCTime();
	struGSTime curDay;
	g_UTCTime_To_struGSTime( curUTCTime, curDay );

	g_delete_mapRec_stat_day( mapRec_stat_day );

	ResultStat.data_dt_begin = day_begin;
	ResultStat.data_dt_end = day_end;
	if( ResultStat.data_dt_begin > curUTCTime )
	{
		LOGMSG( "QueryStatDayRec_ForDayRange begin>curtime err, %s\r\n", ResultStat.GetBaseInfo().c_str() );
		return defGSReturn_OverTimeRange;
	}

	if( g_IsSameDay( curUTCTime, day_end ) )
	{
		ResultStat.data_dt_end = curUTCTime;
	}

	if( ResultStat.data_dt_end > curUTCTime )
	{
		ResultStat.data_dt_end = curUTCTime;
	}

	if( ResultStat.data_dt_begin > ResultStat.data_dt_end )
	{
		LOGMSG( "QueryStat_ForDayRange begin>end err, %s\r\n", ResultStat.GetBaseInfo().c_str() );
		return defGSReturn_ErrParam;
	}

	struGSTime GSTimeBegin;
	g_UTCTime_To_struGSTime( ResultStat.data_dt_begin, GSTimeBegin );

	struGSTime GSTimeEnd;
	g_UTCTime_To_struGSTime( ResultStat.data_dt_end, GSTimeEnd );
	
	// 遍历获取每个月的每天的日统计数据
	struGSTime curDBMonthTime(GSTimeBegin);

	bool hasDB = false;
	while( g_CompareMonth( curDBMonthTime, curDay ) <= 0 )
	{
		AutoRelease_getDBDataSave autodb( this );
		SQLite_DBDataSave *pDBSave = autodb.get( curDBMonthTime, false, "QueryStat_ForDayRange", true );
		if( pDBSave )
		{
			hasDB = true;

			if( doCreateWhenNotFound )
			{
				pDBSave->CheckAndDoStat_day();
			}

			const defGSReturn ret = pDBSave->DBLoad_stat_day_SpecObj_DayRange( ResultStat.AddrObjKey, g_IsSameMonth( curDBMonthTime, GSTimeBegin )?curDBMonthTime.Day:1, g_IsSameMonth( curDBMonthTime, GSTimeEnd )?GSTimeEnd.Day:31, mapRec_stat_day );
			if( macGSFailed(ret) )
			{
				if( !g_isNoDBRec(ret) )
				{
					g_delete_mapRec_stat_day( mapRec_stat_day );
					LOGMSG( "QueryStat_ForDayRange LoadDB err, %s\r\n", ResultStat.GetBaseInfo().c_str() );
					return ret;
				}
			}
		}
		else
		{
			if( hasDB )
			{
				LOGMSG( "QueryStat_ForDayRange not found DB, %s\r\n", g_TimeToStr( curDBMonthTime, defTimeToStrFmt_YearMonth ).c_str() );
			}
		}

		if( g_CompareMonth( curDBMonthTime, GSTimeEnd ) >= 0 )
		{
			break;
		}

		curDBMonthTime.AddSelfMonth();
	}

	// 是否>=今天
	if( doAnalyseNowDay && g_CompareDay( GSTimeEnd, curDay ) >= 0 )
	{
		struRec_stat_day *pRec_stat_day = new struRec_stat_day( struRec_stat_day::struKey( ResultStat.AddrObjKey, curDay.Year, curDay.Month, curDay.Day ) );

		const defGSReturn ret = QueryAnalyse_stat_NowDay( *pRec_stat_day );
		if( macGSFailed(ret) )
		{
			delete pRec_stat_day;

			if( !g_isNoDBRec(ret) )
			{
				g_delete_mapRec_stat_day( mapRec_stat_day );
				LOGMSG( "QueryStat_ForDayRange LoadDB_today err, %s\r\n", ResultStat.GetBaseInfo().c_str() );
				return ret;
			}
		}
		else
		{
			mapRec_stat_day[pRec_stat_day->key] = pRec_stat_day;
		}
	}
	
	if( doStat )
	{
		Stat_mapRec_stat_day_To_QueryStat( mapRec_stat_day, ResultStat, create_lst_query_v1k, spanhour, spanrate, rateForType, remove_invalid );
	}

	//g_delete_mapRec_stat_day( mapRec_stat_day );

	LOGMSG( "QueryStat_ForDayRange usetime=%dms, %s, spanhour=%dh, spanrate=%d, rateForType=%d\r\n", timeGetTime()-dwstart, ResultStat.GetBaseInfo().c_str(), spanhour, spanrate, rateForType );

	ResultStat.data_dt_begin = day_begin;
	ResultStat.data_dt_end = day_end;

	return defGSReturn_Success;
}

// 查询指定对象的起始至结束时间的各个月统计
defGSReturn CDataStoreMgr::QueryStatMonthRec_ForMonthRange( const time_t month_begin, const time_t month_end, stru_QueryStat &ResultStat, defmapRec_stat_month &mapRec_stat_month, defmapRec_stat_day &mapRec_stat_day, const bool getday, const bool doAnalyseNowMonth, const bool doCreateWhenNotFound )
{
	const DWORD dwstart = timeGetTime();

	const time_t curUTCTime = g_GetUTCTime();
	struGSTime curdt;
	g_UTCTime_To_struGSTime( curUTCTime, curdt );

	g_delete_mapRec_stat_month( mapRec_stat_month );

	ResultStat.data_dt_begin = month_begin;
	ResultStat.data_dt_end = month_end;
	if( ResultStat.data_dt_begin > curUTCTime )
	{
		LOGMSG( "QueryStatMonthRec_ForMonthRange begin>curtime err, %s\r\n", ResultStat.GetBaseInfo().c_str() );
		return defGSReturn_OverTimeRange;
	}

	if( g_IsSameDay( curUTCTime, month_end ) )
	{
		ResultStat.data_dt_end = curUTCTime;
	}

	if( ResultStat.data_dt_end > curUTCTime )
	{
		ResultStat.data_dt_end = curUTCTime;
	}

	if( ResultStat.data_dt_begin > ResultStat.data_dt_end )
	{
		LOGMSG( "QueryStatMonthRec_ForMonthRange begin>end err, %s\r\n", ResultStat.GetBaseInfo().c_str() );
		return defGSReturn_ErrParam;
	}

	struGSTime GSTimeBegin;
	g_UTCTime_To_struGSTime( ResultStat.data_dt_begin, GSTimeBegin );

	struGSTime GSTimeEnd;
	g_UTCTime_To_struGSTime( ResultStat.data_dt_end, GSTimeEnd );

	// 遍历获取每个月的月统计数据
	struGSTime curDBMonthTime(GSTimeBegin);

	bool hasDB = false;
	while( g_CompareMonth( curDBMonthTime, curdt ) < 0 )
	{
		AutoRelease_getDBDataSave autodb( this );
		SQLite_DBDataSave *pDBSave = autodb.get( curDBMonthTime, false, "QueryStatMonthRec_ForMonthRange", true );
		if( pDBSave )
		{
			hasDB = true;

			if( doCreateWhenNotFound )
			{
				pDBSave->CheckAndDoStat_day();
			}
			
			struRec_stat_month Rec_stat_month( struRec_stat_month::struKey(ResultStat.AddrObjKey,curDBMonthTime.Year,curDBMonthTime.Month) );
			const defGSReturn ret = pDBSave->DBLoad_stat_month_SpecObj_SpecMonth( Rec_stat_month );
			if( macGSFailed(ret) )
			{
				if( !g_isNoDBRec(ret) )
				{
					g_delete_mapRec_stat_month( mapRec_stat_month );
					g_delete_mapRec_stat_day( mapRec_stat_day );
					LOGMSG( "QueryStatMonthRec_ForMonthRange LoadDB err, %s\r\n", ResultStat.GetBaseInfo().c_str() );
					return ret;
				}
			}
			else
			{
				struRec_stat_month *pMonth = new struRec_stat_month(Rec_stat_month);
				mapRec_stat_month[pMonth->key] = pMonth;
			}

			if( getday )
			{
				const defGSReturn ret = pDBSave->DBLoad_stat_day_SpecObj_DayRange( ResultStat.AddrObjKey, 1, 31, mapRec_stat_day );
				if( macGSFailed(ret) )
				{
					if( !g_isNoDBRec(ret) )
					{
						g_delete_mapRec_stat_month( mapRec_stat_month );
						g_delete_mapRec_stat_day( mapRec_stat_day );
						LOGMSG( "QueryStatMonthRec_ForMonthRange LoadDB:DayRange err, %s\r\n", Rec_stat_month.GetBaseInfo().c_str() );
						return ret;
					}
				}
			}
		}
		else
		{
			if( hasDB )
			{
				LOGMSG( "QueryStatMonthRec_ForMonthRange not found DB, %s\r\n", g_TimeToStr( curDBMonthTime, defTimeToStrFmt_YearMonth ).c_str() );
			}
		}

		if( g_CompareMonth( curDBMonthTime, GSTimeEnd ) >= 0 )
		{
			break;
		}

		curDBMonthTime.AddSelfMonth();
	}

	// 是否>=本月
	if( doAnalyseNowMonth && g_CompareMonth( GSTimeEnd, curdt ) >= 0 )
	{
		struRec_stat_month *pRec_stat_month = new struRec_stat_month( struRec_stat_month::struKey(ResultStat.AddrObjKey,curDBMonthTime.Year,curDBMonthTime.Month) );

		const defGSReturn ret = QueryAnalyse_stat_NowMonth( *pRec_stat_month, mapRec_stat_day, getday, doCreateWhenNotFound );
		if( macGSFailed(ret) )
		{
			delete pRec_stat_month;

			if( !g_isNoDBRec(ret) )
			{
				g_delete_mapRec_stat_month( mapRec_stat_month );
				g_delete_mapRec_stat_day( mapRec_stat_day );
				LOGMSG( "QueryStatMonthRec_ForMonthRange LoadDB:curMonth err, %s\r\n", ResultStat.GetBaseInfo().c_str() );
				return ret;
			}
		}
		else
		{
			mapRec_stat_month[pRec_stat_month->key] = pRec_stat_month;
		}
	}

	Stat_mapRec_stat_month_To_QueryStat( mapRec_stat_month, ResultStat );

	//g_delete_mapRec_stat_day( mapRec_stat_day );

	LOGMSG( "QueryStatMonthRec_ForMonthRange usetime=%dms, %s\r\n", timeGetTime()-dwstart, ResultStat.GetBaseInfo().c_str() );

	ResultStat.data_dt_begin = month_begin;
	ResultStat.data_dt_end = month_end;

	return defGSReturn_Success;
}

// 查询指定对象指定日统计
defGSReturn CDataStoreMgr::QuerySpecRec_stat_day( struRec_stat_day &Rec_stat_day, const bool doCreateWhenNotFound, const bool doAnalyseNowDay )
{
	const DWORD dwstart = timeGetTime();
	const struGSTime querydt( Rec_stat_day.key.dt_year, Rec_stat_day.key.dt_month, Rec_stat_day.key.dt_day );

	AutoRelease_getDBDataSave autodb( this );
	SQLite_DBDataSave *pDBSave = autodb.get( querydt, false, "QueryRec_stat_day", true );
	
	if( !pDBSave )
	{
		LOGMSG( "QueryRec_stat_day not found DB, %s\r\n", Rec_stat_day.GetBaseInfo().c_str() );
		return defGSReturn_DBNoExist;
	}

	if( doCreateWhenNotFound && !pDBSave->is_stat_day(Rec_stat_day.key.dt_day) )
	{
		pDBSave->CheckAndDoStat_day();
	}

	const defGSReturn ret = pDBSave->DBLoad_stat_day_SpecObj_SpecDay( Rec_stat_day );
	if( macGSFailed(ret) )
	{
		if( doAnalyseNowDay && g_IsNowDay(querydt) )
		{
			return QueryAnalyse_stat_NowDay( Rec_stat_day );
		}
		else
		{
			LOGMSG( "QueryRec_stat_day DBLoad err, %s\r\n", Rec_stat_day.GetBaseInfo().c_str() );
			return ret;
		}
	}

	LOGMSG( "QueryRec_stat_day usetime=%dms, %s\r\n", timeGetTime()-dwstart, Rec_stat_day.GetBaseInfo().c_str() );

	return defGSReturn_Success;
}

// 查询指定对象指定月统计
defGSReturn CDataStoreMgr::QuerySpecRec_stat_month( struRec_stat_month &Rec_stat_month, const bool doCreateWhenNotFound, const bool doAnalyseNowMonth )
{
	const DWORD dwstart = timeGetTime();
	const struGSTime querydt( Rec_stat_month.key.dt_year, Rec_stat_month.key.dt_month, 1 );

	AutoRelease_getDBDataSave autodb( this );
	SQLite_DBDataSave *pDBSave = autodb.get( querydt, false, "QueryRec_stat_month", true );

	if( !pDBSave )
	{
		LOGMSG( "QueryRec_stat_month not found DB, %s\r\n", Rec_stat_month.GetBaseInfo().c_str() );
		return defGSReturn_DBNoExist;
	}

	if( doCreateWhenNotFound && !pDBSave->is_stat_month() && pDBSave->isCan_DoStat_month() )
	{
		pDBSave->CheckAndDoStat_day();
	}

	const defGSReturn ret = pDBSave->DBLoad_stat_month_SpecObj_SpecMonth( Rec_stat_month );
	if( macGSFailed(ret) )
	{
		if( doAnalyseNowMonth && g_IsNowMonth(querydt) )
		{
			defmapRec_stat_day mapRec_stat_day;
			return QueryAnalyse_stat_NowMonth( Rec_stat_month, mapRec_stat_day, false, doCreateWhenNotFound );
		}
		else
		{
			LOGMSG( "QueryRec_stat_month DBLoad err, ret=%s, %s\r\n", g_Trans_GSReturn(ret).c_str(), Rec_stat_month.GetBaseInfo().c_str() );
			return ret;
		}
	}

	LOGMSG( "QueryRec_stat_month usetime=%dms, %s\r\n", timeGetTime()-dwstart, Rec_stat_month.GetBaseInfo().c_str() );

	return defGSReturn_Success;
}

// 获取指定对象的指定时间的值
defGSReturn CDataStoreMgr::QueryValueForSpecTime( const GSIOTAddrObjKey &AddrObjKey, const time_t spec_data_dt, stru_V1K &get_v1k )
{
	const DWORD dwstart = timeGetTime();

	struGSTime dt;
	g_UTCTime_To_struGSTime( spec_data_dt, dt );

	AutoRelease_getDBDataSave autodb( this );
	SQLite_DBDataSave *pDBSave = autodb.get( dt, false, "QueryValueForSpecTime", true );

	if( !pDBSave )
	{
		LOGMSG( "QueryValueForSpecTime not found DB, %s\r\n", AddrObjKey.get_str().c_str() );
		return defGSReturn_DBNoExist;
	}
	
	const defGSReturn ret = pDBSave->QueryValueForSpecTime( AddrObjKey, spec_data_dt, get_v1k );
	if( macGSFailed(ret) )
	{
		LOGMSG( "QueryValueForSpecTime err, ret=%s, %s\r\n", g_Trans_GSReturn(ret).c_str(), AddrObjKey.get_str().c_str() );
		return ret;
	}

	LOGMSG( "QueryValueForSpecTime usetime=%dms, get_v1k=%d, %s, %s\r\n", timeGetTime()-dwstart, get_v1k, g_TimeToStr(spec_data_dt).c_str(), AddrObjKey.get_str().c_str() );

	return defGSReturn_Success;
}

// 指定时间范围内的原始值
defGSReturn CDataStoreMgr::QuerySrcValueLst_ForTimeRange( const GSIOTAddrObjKey &AddrObjKey, const time_t data_dt_begin, const time_t data_dt_end, defmapV1k &lst_v1k, const int spanrate, const bool rateForType )
{
	const int c_maxday = 55;

	const DWORD dwstart = timeGetTime();

	const time_t curUTCTime = g_GetUTCTime();

	time_t query_data_dt_begin = data_dt_begin;
	time_t query_data_dt_end = data_dt_end;
	if( query_data_dt_begin > curUTCTime )
	{
		LOGMSG( "QuerySrcValueLst_ForTimeRange begin>curtime err, %s\r\n", AddrObjKey.get_str().c_str() );
		return defGSReturn_OverTimeRange;
	}

	if( query_data_dt_end > curUTCTime )
	{
		query_data_dt_end = curUTCTime;
	}

	if( query_data_dt_begin > query_data_dt_end )
	{
		LOGMSG( "QuerySrcValueLst_ForTimeRange begin>end err, %s\r\n", AddrObjKey.get_str().c_str() );
		return defGSReturn_ErrParam;
	}

	// 跨度太长 超过n天
	if( query_data_dt_end - query_data_dt_begin > c_maxday*24*60*60 )
	{
		LOGMSG( "QuerySrcValueLst_ForTimeRange time long>%dday err, %s\r\n", c_maxday, AddrObjKey.get_str().c_str() );
		return defGSReturn_OverTimeRange;
	}

	struGSTime GSTimeBegin;
	if( !g_UTCTime_To_struGSTime( query_data_dt_begin, GSTimeBegin ) )
	{
		LOGMSG( "QuerySrcValueLst_ForTimeRange time begin err, %s\r\n", AddrObjKey.get_str().c_str() );
		return defGSReturn_Err;
	}

	struGSTime GSTimeEnd;
	if( !g_UTCTime_To_struGSTime( query_data_dt_end, GSTimeEnd ) )
	{
		LOGMSG( "QuerySrcValueLst_ForTimeRange time end err, %s\r\n", AddrObjKey.get_str().c_str() );
		return defGSReturn_Err;
	}

	AutoRelease_getDBDataSave autodbA( this );
	SQLite_DBDataSave *pDBSaveA = autodbA.get( GSTimeBegin, false, "QuerySrcValueLst_ForTimeRange_A", true );

	AutoRelease_getDBDataSave autodbB( this );
	SQLite_DBDataSave *pDBSaveB = NULL;

	// 是否跨月
	if( !IsSameDBSave(GSTimeBegin, GSTimeEnd) )
	{
		pDBSaveB = autodbB.get( GSTimeEnd, false, "QuerySrcValueLst_ForTimeRange_B", true );
	}

	if( !pDBSaveA && !pDBSaveB )
	{
		LOGMSG( "QuerySrcValueLst_ForTimeRange not found DB, %s\r\n", AddrObjKey.get_str().c_str() );
		return defGSReturn_DBNoExist;
	}

	// 载入时间范围内数据，由于时间范围可自由选择提供，最前面和最后面的可能不是完整的一天

	if( pDBSaveA )
	{
		const defGSReturn ret = pDBSaveA->DBLoad_histdata_SpecObj_ForTime( AddrObjKey, query_data_dt_begin, query_data_dt_end, lst_v1k, true, spanrate, rateForType );
		if( macGSFailed(ret) )
		{
			lst_v1k.clear();
			LOGMSG( "QuerySrcValueLst_ForTimeRange LoadDB_A err, %s\r\n", AddrObjKey.get_str().c_str() );
			return ret;
		}
	}

	if( pDBSaveB )
	{
		const defGSReturn ret = pDBSaveB->DBLoad_histdata_SpecObj_ForTime( AddrObjKey, query_data_dt_begin, query_data_dt_end, lst_v1k, true, spanrate, rateForType );
		if( macGSFailed(ret) )
		{
			lst_v1k.clear();
			LOGMSG( "QuerySrcValueLst_ForTimeRange LoadDB_B err, %s\r\n", AddrObjKey.get_str().c_str() );
			return ret;
		}
	}

	LOGMSG( "QuerySrcValueLst_ForTimeRange usetime=%dms, %s, time(%s)-(%s) lst_v1k.size=%d\r\n", timeGetTime()-dwstart, AddrObjKey.get_str().c_str(), g_TimeToStr(data_dt_begin).c_str(), g_TimeToStr(data_dt_end).c_str(), lst_v1k.size() );

	return defGSReturn_Success;
}

// 指定时间范围内的原始值
defGSReturn CDataStoreMgr::QuerySrcValueLst_ForTimeRange_QueryStat( stru_QueryStat &ResultStat, const int spanrate, const bool rateForType )
{
	const DWORD dwstart = timeGetTime();

	defmapV1k lst_v1k;
	defGSReturn ret = QuerySrcValueLst_ForTimeRange( ResultStat.AddrObjKey, ResultStat.data_dt_begin, ResultStat.data_dt_end, lst_v1k, spanrate, rateForType );
	if( macGSFailed(ret) )
	{
		lst_v1k.clear();
		LOGMSG( "QuerySrcValueLst_ForTimeRange_QueryStat LoadDB_B err, %s\r\n", ResultStat.AddrObjKey.get_str().c_str() );
		return ret;
	}

	// 统计
	for( defmapV1k::const_iterator it=lst_v1k.begin(); it!=lst_v1k.end(); ++it )
	{
		const time_t curTime = it->first;
		const int curValue = it->second.v1k;

		ResultStat.lst_query_v1k[curTime] = stru_QueryV1K( curTime, it->second.v1k_valid, curValue );

		if( !it->second.v1k_valid )
			continue;

		// 总数累计，后面计算平均值
		ResultStat.temp_v1k_sum += it->second.v1k;
		ResultStat.Stat.stat_v1k_avg_num++;

		g_AnalyseStat( ResultStat.Stat, curTime, curValue, ResultStat.data_dt_begin, ResultStat.data_dt_end );
	}

	// 计算平均值
	ResultStat.Stat.calc_avg( ResultStat.temp_v1k_sum );

	lst_v1k.clear();
	
	LOGMSG( "QuerySrcValueLst_ForTimeRange_QueryStat usetime=%dms, %s, time(%s)-(%s) lst_v1k.size=%d\r\n", timeGetTime()-dwstart, ResultStat.AddrObjKey.get_str().c_str(), g_TimeToStr(ResultStat.data_dt_begin).c_str(), g_TimeToStr(ResultStat.data_dt_end).c_str(), ResultStat.lst_query_v1k.size() );

	return defGSReturn_Success;
}

