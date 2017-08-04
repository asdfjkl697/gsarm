#pragma once

#include "SQLiteHelper.h"
#include "typedef.h"
#include "gloox/mutexguard.h"
#include "string.h"

#define defValidRange_prev	(35*60)
#define defValidRange_next	(10*60)

// ԭʼֵ�ṹ
struct stru_V1K
{
	time_t data_dt;			// ֵ��ʱ��
	defDataFlag_ v1k_valid;	// ��־ DataFlag
	int v1k;				// 1000����ֵ

	stru_V1K( const time_t in_data_dt=0, const defDataFlag_ in_v1k_valid=defDataFlag_Invalid, const int in_v1k=0 )
		: data_dt(in_data_dt), v1k_valid(in_v1k_valid), v1k(in_v1k)
	{
	}

	void reset()
	{
		data_dt = 0;
		v1k_valid = defDataFlag_Invalid;
		v1k = 0;
	}
};
#define stru_QueryV1K stru_V1K
typedef std::map<time_t,stru_V1K> defmapV1k;	 // ԭʼ����б� <utctime,stru_SrcV1K>

// ��ݱ��滺��ṹ
struct struDataSave
{
	time_t data_dt;				// ֵ�ɼ�ʱ��

	GSIOTAddrObjKey AddrObjKey;	// ��ݼ�¼�Ķ���key

	defDataFlag_ dataflag;		// ����ԭ��ȵȱ�־
	std::string strvalue;		// ֵ
	std::string name;			// ֵ�ɼ��������

	struDataSave( const time_t in_data_dt, const IOTDeviceType in_dev_type, const int in_dev_id, const IOTDeviceType in_address_type, const int in_address_id,
		const defDataFlag_ in_dataflag, const std::string &in_strvalue, const std::string &in_name )
		: data_dt(in_data_dt), AddrObjKey( in_dev_type, in_dev_id, in_address_type, in_address_id ),
		dataflag(in_dataflag), strvalue(in_strvalue), name(in_name)
	{
	}
};
typedef std::vector<struDataSave*> defvecDataSave;


// ��ݼ�¼
struct struRec_histdata
{
	time_t data_dt;				// ֵ��ʱ��

	GSIOTAddrObjKey AddrObjKey;	// ��¼�Ķ���key

	defDataFlag_ dataflag;		// ��־
	int value1k;				// 1000����ֵ
	//std::string data_dt_str;
	//std::string strvalue;
	//std::string name;
};
typedef std::map<time_t,struRec_histdata*> defmapRec_histdata;


// ͳ��
struct struStat
{
	struStat(
		const int in_stat_valid=0,
		const int in_stat_v1k_max_dt=0,
		const int in_stat_v1k_max=0,
		const int in_stat_v1k_min_dt=0,
		const int in_stat_v1k_min=0,
		const int in_stat_v1k_avg=0,
		const int in_stat_v1k_avg_num=0
		)
		: stat_valid(in_stat_valid),
		stat_v1k_max_dt(in_stat_v1k_max_dt),
		stat_v1k_max(in_stat_v1k_max),
		stat_v1k_min_dt(in_stat_v1k_min_dt),
		stat_v1k_min(in_stat_v1k_min),
		stat_v1k_avg(in_stat_v1k_avg),
		stat_v1k_avg_num(in_stat_v1k_avg_num)
	{
	}

	void calc_avg( const double sum )
	{
		stat_v1k_avg = ( stat_v1k_avg_num > 0 ) ? ( sum / stat_v1k_avg_num ) : 0;
	}

	int stat_valid;				// stat_v1k_��ͷ��ͳ������Ƿ���Ч��1��Ч0��Ч
	int stat_v1k_max_dt;		// ���ֵ����ʱ��
	int stat_v1k_max;			// ���ֵ
	int stat_v1k_min_dt;		// ��Сֵ����ʱ��
	int stat_v1k_min;			// ��Сֵ
	int stat_v1k_avg;			// ƽ��ֵ
	int stat_v1k_avg_num;		// ���ڼ���ƽ��ֵ��ԭʼ�������
};
typedef std::map<struGSTime,struStat> defmapstruStat;

// ��ͳ�ƽṹ
struct struRec_stat_day
{
	struct struKey
	{
		int dt_year;				// ͳ�Ƽ�¼������
		int dt_month;				// ͳ�Ƽ�¼������
		int dt_day;					// ͳ�Ƽ�¼������

		GSIOTAddrObjKey AddrObjKey;	// ͳ�Ƽ�¼�Ķ���key
		
		struKey( int in_dt_year=0, int in_dt_month=0, int in_dt_day=0, IOTDeviceType in_dev_type=IOT_DEVICE_Unknown, int in_dev_id=0, IOTDeviceType in_address_type=IOT_DEVICE_Unknown, int in_address_id=0 )
			: dt_year(in_dt_year), dt_month(in_dt_month), dt_day(in_dt_day), AddrObjKey( in_dev_type, in_dev_id, in_address_type, in_address_id )
		{
		}

		struKey( const GSIOTAddrObjKey &in_AddrObjKey, int in_dt_year=0, int in_dt_month=0, int in_dt_day=0 )
			: dt_year(in_dt_year), dt_month(in_dt_month), dt_day(in_dt_day), AddrObjKey( in_AddrObjKey )
		{
		}
	};

	struKey key;

	// ͳ��
	struStat Stat;

	// 48�����Ͱ��ֵ A��24λǰ24��ֵ��B��24λ��24��ֵ��1��Ч0��Ч
	int v1k_valid_00_11A;
	int v1k_valid_12_23B;
	int v1k[48];				// 48�����Ͱ��ֵ��˳����������00:00��00:30��01:00��01:30���ȵ�

	defmapV1k lst_src_v1k;		// ԭʼ����б� <utctime,value1k>
	defmapstruStat lst_stathour;	// Сʱ��ͳ�ƣ�h:0-23��0��ʾ0:00-0:59Сʱ�ڵ�ͳ�ƣ�1��ʾ1:00-1:59���ڵ�ͳ��...

	struRec_stat_day( const struKey &in_key )
		: key(in_key), 
		v1k_valid_00_11A(0), v1k_valid_12_23B(0)
	{
		memset( &v1k, 0, sizeof(v1k) );
	}

	void moveto( struRec_stat_day &other )
	{
		other.key = this->key;

		other.Stat = this->Stat;

		other.v1k_valid_00_11A = this->v1k_valid_00_11A;
		other.v1k_valid_12_23B = this->v1k_valid_12_23B;
		memcpy( &other.v1k, &this->v1k, sizeof(other.v1k) );

		this->lst_src_v1k.swap( other.lst_src_v1k );
		this->lst_stathour.swap( other.lst_stathour );
	}

	std::string GetBaseInfo() const;
	void Print( const bool print_lst_src_v1k=false ) const;
};
bool operator< ( const struRec_stat_day::struKey &key1, const struRec_stat_day::struKey &key2 );
typedef std::map<struRec_stat_day::struKey,struRec_stat_day*> defmapRec_stat_day;


// ��ͳ�ƽṹ
struct struRec_stat_month
{
	struct struKey
	{
		int dt_year;				// ͳ�Ƽ�¼������
		int dt_month;				// ͳ�Ƽ�¼������

		GSIOTAddrObjKey AddrObjKey;	// ͳ�Ƽ�¼�Ķ���key

		struKey( int in_dt_year=0, int in_dt_month=0, IOTDeviceType in_dev_type=IOT_DEVICE_Unknown, int in_dev_id=0, IOTDeviceType in_address_type=IOT_DEVICE_Unknown, int in_address_id=0 )
			: dt_year(in_dt_year), dt_month(in_dt_month), AddrObjKey( in_dev_type, in_dev_id, in_address_type, in_address_id )
		{
		}

		struKey( const GSIOTAddrObjKey &in_AddrObjKey, int in_dt_year=0, int in_dt_month=0 )
			: dt_year(in_dt_year), dt_month(in_dt_month), AddrObjKey( in_AddrObjKey )
		{
		}

		struKey( const struRec_stat_day::struKey &in_key )
			: dt_year(in_key.dt_year), dt_month(in_key.dt_month), AddrObjKey(in_key.AddrObjKey)
		{
		}
	};

	struKey key;

	// ͳ��
	struStat Stat;
	double temp_v1k_sum;		// ����ͳ��ƽ��ֵ����ʱ����		

	struRec_stat_month( const struKey &in_key )
		: key(in_key), temp_v1k_sum(0.0f)
	{
	}

	void moveto( struRec_stat_month &other )
	{
		other.key = this->key;

		other.Stat = this->Stat;
	}

	std::string GetBaseInfo() const;
	void Print() const;
};
bool operator< ( const struRec_stat_month::struKey &key1, const struRec_stat_month::struKey &key2 );
typedef std::map<struRec_stat_month::struKey,struRec_stat_month*> defmapRec_stat_month;

// ͳ�Ʋ�ѯ�ṹ
struct stru_QueryStat
{
	GSIOTAddrObjKey AddrObjKey;
	
	time_t data_dt_begin;		// ��ѯ�����ʼʱ��
	time_t data_dt_end;			// ��ѯ��ݽ���ʱ��

	// ͳ��
	struStat Stat;
	double temp_v1k_sum;		// ����ͳ��ƽ��ֵ����ʱ����
	
	defmapV1k lst_query_v1k;

	stru_QueryStat()
		: data_dt_begin(0), data_dt_end(0), temp_v1k_sum(0.0f)
	{
	}

	std::string GetBaseInfo() const;
	void Print() const;
	void AddStatToList();
};

typedef std::set<unsigned char> defmap_is_stat_day; // ��ͳ���Ƿ��Ѵ��ڼ�¼�б�

bool g_isNoDBRec( const defGSReturn ret );
void g_AnalyseStat( struStat &Stat, const time_t curTime, const int curValue, const time_t data_dt_begin=0, const time_t data_dt_end=0 );
void g_AnalyseStat( struStat &Stat, const struStat &substat, const time_t data_dt_begin=0, const time_t data_dt_end=0 );
bool g_getLeftV1k( const time_t data_dt, const defmapV1k &lst_v1k, stru_V1K &get_v1k, const time_t dt_valid_range );
bool g_getRightV1k( const time_t data_dt, const defmapV1k &lst_v1k, stru_V1K &get_v1k, const time_t dt_valid_range );
bool g_is_stat_day_spec( const defmap_is_stat_day &lst_is_stat_day, unsigned char day );
void g_delete_vecDataSave( defvecDataSave &vecDataSave );
void g_delete_mapRec_stat_day( defmapRec_stat_day &mapRec_stat_day );
void g_delete_mapRec_stat_month( defmapRec_stat_month &mapRec_stat_month );
void g_Print_lst_v1k( const defmapV1k &lst_v1k, const char *pinfo );

//=====================================================
// DBDataSave

class CDataStoreMgr;
class SQLite_DBDataSave
{
public:
	friend class CDataStoreMgr; //jyc20170505 add class
	SQLite_DBDataSave( bool isReadOnly, const std::string &dbname, const struGSTime &dt, bool DoAutoCreate );
	~SQLite_DBDataSave(void);

	static bool GetValueForSpecTime_lst( const time_t spec_data_dt, const defmapV1k &lst_v1k, stru_V1K &get_v1k, const time_t dt_valid_range_prev=defValidRange_prev, const time_t dt_valid_range_next=defValidRange_next );
	static void GetOneStatForSpecTime_lst( const time_t data_dt_begin, const time_t data_dt_end, const defmapV1k &lst_src_v1k, struStat &Stat );
	static bool DoStat_day_specday_specone( struRec_stat_day &Rec_stat_day, const bool retime_maxmin=true, const time_t data_dt_begin=0, const time_t data_dt_end=0 );
	static void DoStat_hour_lst( const time_t data_dt_begin, const time_t data_dt_end, const defmapV1k &lst_src_v1k, defmapstruStat &lst_stathour, const int spanhour=1 );
	static void DoStat_minute_lst( const time_t data_dt_begin, const time_t data_dt_end, const defmapV1k &lst_src_v1k, defmapstruStat &lst_statminute, const int spanminute=1 );

	SQLite::Database* get_db() const
	{
		return db;
	}

	const struGSTime& get_dt() const
	{
		return m_dtDBSave;
	}

	const std::string& get_path() const
	{
		return m_path;
	}

	const std::string& get_dbname() const
	{
		return m_dbname;
	}

	bool IsReadOnly() const
	{
		return m_isReadOnly;
	}

	void SetReadOnly()
	{
		m_isReadOnly = true;
	}

	bool is_stat_month() const
	{
		return m_is_stat_month;
	}

	bool is_all_stat_day_finished() const;
	bool is_stat_day( unsigned char day ) const
	{
		return g_is_stat_day_spec( m_lst_is_stat_day, day );
	}

	bool isCan_DoStat_month() const;
	bool isCan_DoStat_day( const unsigned char day ) const;

	void print_is_stat() const;

	void insertdata( const defvecDataSave &vecDataSave );

	bool CheckAndDoStat_day();
	defGSReturn DoStat_day_all();
	defGSReturn DoStat_day_all_specmax( const unsigned char maxday, const unsigned char specstart=1 );
	defGSReturn DoStat_day_specday( const unsigned char day );
	defGSReturn DoStat_month_all();

	defGSReturn QueryValueForSpecTime( const GSIOTAddrObjKey &AddrObjKey, const time_t spec_data_dt, stru_V1K &get_v1k );

	defGSReturn DBLoad_histdata_SpecObj_ForDay( const GSIOTAddrObjKey &AddrObjKey, const unsigned char day, defmapV1k &lst_src_v1k ) const;
	defGSReturn DBLoad_histdata_SpecObj_ForTime( const GSIOTAddrObjKey &AddrObjKey, const time_t data_dt_begin, const time_t data_dt_end, defmapV1k &lst_src_v1k, const bool doQueryMax=false, const int spanrate=0, const bool rateForType=false ) const;
	defGSReturn DBLoad_histdata_SpecObj_mapRecStatDay_ForTime( const GSIOTAddrObjKey &AddrObjKey, const time_t data_dt_begin, const time_t data_dt_end, defmapRec_stat_day &mapRec_stat_day ) const;
	defGSReturn DBLoad_histdata_AllObj_ForDay( const unsigned char day, defmapRec_stat_day &mapRec_stat_day ) const;

	defGSReturn DBReadOneRec_stat_day( SQLite::Statement &query, struRec_stat_day &Rec_stat_day ) const;
	defGSReturn DBLoad_stat_day_SpecObj_SpecDay( struRec_stat_day &Rec_stat_day ) const;
	defGSReturn DBLoad_stat_day_SpecObj_DayRange( const GSIOTAddrObjKey &AddrObjKey, const unsigned char day_begin, const unsigned char day_end, defmapRec_stat_day &mapRec_stat_day ) const;
	defGSReturn DBLoad_stat_day_full( defmapRec_stat_day &mapRec_stat_day ) const;

	defGSReturn DBReadOneRec_stat_month( SQLite::Statement &query, struRec_stat_month &Rec_stat_month ) const;
	defGSReturn DBLoad_stat_month_SpecObj_SpecMonth( struRec_stat_month &Rec_stat_month ) const;
	defGSReturn DBLoad_stat_month_full( defmapRec_stat_month &mapRec_stat_month ) const;

	bool DBIsBeing_stat_day( const struRec_stat_day &Rec_stat_day );
	bool DBIsBeing_stat_day_getlist( const struRec_stat_day &Rec_stat_day, defmap_is_stat_day &lst_is_stat_day );
	bool DBIsBeing_stat_month( const struRec_stat_month &Rec_stat_month );
	bool DBSave_stat_day( const struRec_stat_day &Rec_stat_day );
	bool DBSave_stat_month( const struRec_stat_month &Rec_stat_month );

protected:
	SQLite::Database *db;
	struGSTime m_dtDBSave;	// ��ݴ洢ʱ�� ��+��
	bool m_isReadOnly;		// �Ƿ�ֻ����
	std::string m_path;		// ��ݿ��ļ�ȫ·��
	std::string m_dbname;	// ��ݿ��ļ����

	bool m_is_stat_month;			// �����Ƿ���ͳ��
	defmap_is_stat_day m_lst_is_stat_day;	// ��ͳ�Ƶ����������¼ ��1��ʾ1�գ�2��ʾ2�գ��ȵ�

	int m_usecount;			// ���ڱ�ʹ�ü���
	uint32_t m_lastUseTs;	// �ϴ�ʹ��ʱ���
};


class CDataStoreMgr
{
public:
	friend class AutoRelease_getDBDataSave;
	CDataStoreMgr(void);
	~CDataStoreMgr(void);

	static bool IsSameDBSave( const time_t utctimeA, const time_t utctimeB );
	static bool IsSameDBSave( const struGSTime &dtA, const struGSTime &dtB );
	static std::string CreateDBNameForSpecDt( const time_t &t );
	static std::string CreateDBNameForSpecDt( const struGSTime &dt );
	static std::string CreateDBNameForCur();
	static defGSReturn Stat_mapRec_stat_day_To_QueryStat( const defmapRec_stat_day &mapRec_stat_day, stru_QueryStat &ResultStat, const bool create_lst_query_v1k=true, const int spanhour=0, const int spanrate=0, const bool rateForType=false, const bool remove_invalid=true, const bool doQueryMax=false );
	static defGSReturn Stat_lst_src_v1k_To_QueryStat_ForSpanmin( const defmapV1k &lst_src_v1k, stru_QueryStat &ResultStat, const int spanmin=15, const int spanrate=0, const bool rateForType=false, const bool remove_invalid=true, const bool doQueryMax=false, const bool retime_maxmin=true );
	static void QueryStatFixInvalidFullDay( stru_QueryStat &ResultStat, const defmapRec_stat_day &mapRec_stat_day, const time_t first_day_begin_time );
	static bool Stat_mapRec_stat_month_To_QueryStat( const defmapRec_stat_month &mapRec_stat_month, stru_QueryStat &ResultStat );
	
	void insertdata( const defvecDataSave &vecDataSave );
	bool insertdata_CheckSaveInvalid( const GSIOTAddrObjKey &AddrObjKey, const bool valid );
	void CheckStat();

	defGSReturn QueryAnalyse_stat_NowDay( struRec_stat_day &Rec_stat_day );
	defGSReturn QueryAnalyse_stat_NowMonth( struRec_stat_month &Rec_stat_month, defmapRec_stat_day &mapRec_stat_day, const bool getday=false, const bool doCreateWhenNotFound=true );
	defGSReturn QueryStatData_ForTimeToNow( const time_t data_dt_begin, stru_QueryStat &ResultStat, const bool create_lst_query_v1k=true, const int spanhour=0, const int spanrate=0, const bool rateForType=false, const bool remove_invalid=true );
	defGSReturn QueryStatData_ForTime( const time_t data_dt_begin, const time_t data_dt_end, stru_QueryStat &ResultStat, defmapRec_stat_day &mapRec_stat_day, const bool create_stathour=false, const bool create_lst_query_v1k=true, const int spanhour=0, const int spanrate=0, const bool rateForType=false, const bool remove_invalid=true );
	defGSReturn QueryStatData_ForTimeToNow_ForSpanmin( const time_t data_dt_begin, stru_QueryStat &ResultStat, const int spanmin=0, const int spanrate=0, const bool rateForType=false, const bool remove_invalid=true );
	defGSReturn QueryStatData_ForTime_ForSpanmin( const time_t data_dt_begin, const time_t data_dt_end, stru_QueryStat &ResultStat, const int spanmin=0, const int spanrate=0, const bool rateForType=false, const bool remove_invalid=true );
	defGSReturn QueryStatMinute_ForTime( stru_QueryStat &ResultStat, defmapstruStat &lst_statminute, const int spanminute );
	defGSReturn QueryStatDayRec_ForDayRange( const time_t day_begin, const time_t day_end, stru_QueryStat &ResultStat, defmapRec_stat_day &mapRec_stat_day, const bool doAnalyseNowDay=true, const bool doCreateWhenNotFound=true, const bool create_lst_query_v1k=true, const int spanhour=0, const int spanrate=0, const bool rateForType=false, const bool remove_invalid=true, const bool doStat=true );
	defGSReturn QueryStatMonthRec_ForMonthRange( const time_t month_begin, const time_t month_end, stru_QueryStat &ResultStat, defmapRec_stat_month &mapRec_stat_month, defmapRec_stat_day &mapRec_stat_day, const bool getday=false, const bool doAnalyseNowMonth=true, const bool doCreateWhenNotFound=true );
	defGSReturn QuerySpecRec_stat_day( struRec_stat_day &Rec_stat_day, const bool doCreateWhenNotFound=true, const bool doAnalyseNowDay=true );
	defGSReturn QuerySpecRec_stat_month( struRec_stat_month &Rec_stat_month, const bool doCreateWhenNotFound=true, const bool doAnalyseNowMonth=true );

	defGSReturn QueryValueForSpecTime( const GSIOTAddrObjKey &AddrObjKey, const time_t spec_data_dt, stru_V1K &get_v1k );
	defGSReturn QuerySrcValueLst_ForTimeRange( const GSIOTAddrObjKey &AddrObjKey, const time_t data_dt_begin, const time_t data_dt_end, defmapV1k &lst_v1k, const int spanrate=0, const bool rateForType=false );
	defGSReturn QuerySrcValueLst_ForTimeRange_QueryStat( stru_QueryStat &ResultStat, const int spanrate=0, const bool rateForType=false );

	void DBMgr_Print();
	void DBMgr_Check( const bool CheckNow=false );
protected:
	SQLite_DBDataSave* GetDBSave_lock( const struGSTime &dt, const bool DoAutoCreate, const char *pinfo, const bool check_file_exists );
	bool DBMgr_set( SQLite_DBDataSave *pDBDataSave );
	SQLite_DBDataSave* DBMgr_get( const struGSTime &dt );
	SQLite_DBDataSave* DBMgr_get( const std::string &dbname );
	void DBMgr_release( SQLite_DBDataSave *pDBDataSave );
	bool DBMgr_is_stat_day( const struGSTime &dt );
private:
	SQLite_DBDataSave* DBMgr_get_nolock( const struGSTime &dt );
	SQLite_DBDataSave* DBMgr_get_nolock( const std::string &dbname );

	//void RemoveDB(struGSTime dt); //jyc20170425 add
	void CheckCreateDB( const time_t utctime );

private:
	gloox::util::Mutex m_mutex_lstDBDataSaveMgr;
	std::map<std::string,SQLite_DBDataSave*>	m_lstDBDataSaveMgr; // <CreateDBNameForSpecDt�õ����ļ���, ��ݿ�洢ʵ��ָ��>
	uint32_t m_lastdo_DBMgr_Check;

	std::map<GSIOTAddrObjKey,int> m_mapCheckInvalid; // ���һ�δ洢�Ƿ���Чֵ<key,count>
};

// �Զ��ͷŷ�װ��
class AutoRelease_getDBDataSave
{
public:
	AutoRelease_getDBDataSave( CDataStoreMgr *pMgr )
		: m_pMgr(pMgr), m_pDBSave(NULL)
	{}

	~AutoRelease_getDBDataSave()
	{
		release();
	}

	SQLite_DBDataSave* get( const struGSTime &dt, const bool DoAutoCreate, const char *pinfo, const bool check_file_exists )
	{
		if( !m_pMgr )
			return NULL;

		release();

		m_pDBSave = m_pMgr->GetDBSave_lock( dt, DoAutoCreate, pinfo, check_file_exists );
		return m_pDBSave;
	}

	void release()
	{
		if( !m_pMgr )
			return ;

		if( m_pDBSave )
		{
			m_pMgr->DBMgr_release( m_pDBSave );
			m_pDBSave = NULL;
		}
	}

	SQLite_DBDataSave* p() const
	{
		return m_pDBSave;
	}

private:
	AutoRelease_getDBDataSave& operator=( const AutoRelease_getDBDataSave& );
	SQLite_DBDataSave *m_pDBSave;
	CDataStoreMgr *m_pMgr;
};
