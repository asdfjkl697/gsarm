#include "XmppGSReport.h"
#include "gloox/util.h"
#include "../gsiot/GSIOTClient.h"

static bool g_getAddrObjKey( Tag *t, GSIOTAddrObjKey &AddrObjKey )
{
	if( t->hasAttribute("dtp") )
	{
		AddrObjKey.dev_type = (IOTDeviceType)atoi( t->findAttribute("dtp").c_str() );
	}

	if( t->hasAttribute("did") )
	{
		AddrObjKey.dev_id = atoi( t->findAttribute("did").c_str() );
	}

	if( t->hasAttribute("atp") )
	{
		AddrObjKey.address_type = (IOTDeviceType)atoi( t->findAttribute("atp").c_str() );
	}

	if( t->hasAttribute("aid") )
	{
		AddrObjKey.address_id = atoi( t->findAttribute("aid").c_str() );
	}

	return true;
}

static void g_tagAddrObjKey( Tag *tobj, const GSIOTAddrObjKey &AddrObjKey )
{
	tobj->addAttribute( "dtp", (int)AddrObjKey.dev_type );
	tobj->addAttribute( "did", (int)AddrObjKey.dev_id );
	tobj->addAttribute( "atp", (int)AddrObjKey.address_type );
	tobj->addAttribute( "aid", (int)AddrObjKey.address_id );
}

static bool g_getGSTime( Tag *t, struGSTime &dt, const bool isbegin )
{
	if( t->hasAttribute("y") )
	{
		dt.Year = atoi( t->findAttribute("y").c_str() );
	}

	if( t->hasAttribute("m") )
	{
		dt.Month = atoi( t->findAttribute("m").c_str() );
	}

	if( t->hasAttribute("d") )
	{
		dt.Day = atoi( t->findAttribute("d").c_str() );
	}

	if( t->hasAttribute("h") )
	{
		dt.Hour = atoi( t->findAttribute("h").c_str() );
	}
	else
	{
		dt.Hour = isbegin ? 0 : 23;
	}

	if( t->hasAttribute("n") )
	{
		dt.Minute = atoi( t->findAttribute("n").c_str() );
	}
	else
	{
		dt.Minute = isbegin ? 0 : 59;
	}
	
	if( t->hasAttribute("s") )
	{
		dt.Second = atoi( t->findAttribute("s").c_str() );
	}
	else
	{
		dt.Second = isbegin ? 0 : 59;
	}

	return true;
}

static void g_tagGSTime( Tag *tGSTime, const struGSTime &dt,
	const bool y=true, const bool m=true, const bool d=true,
	const bool h=true, const bool n=true )
{
	if( y ) tGSTime->addAttribute( "y", (int)dt.Year );
	if( m ) tGSTime->addAttribute( "m", (int)dt.Month );
	if( d ) tGSTime->addAttribute( "d", (int)dt.Day );
	if( h ) tGSTime->addAttribute( "h", (int)dt.Hour );
	if( n ) tGSTime->addAttribute( "n", (int)dt.Minute );
}

// v1k value
static void g_tagStat( Tag *t, const struStat &Stat, const IOTDeviceType type, const int getfortype, const bool canmerge=false )
{
	//if( Stat.stat_v1k_max_dt==Stat.stat_v1k_min_dt
	//	&& Stat.stat_v1k_max==Stat.stat_v1k_min )
	//{
	//	t->addAttribute( "all_t", (int)Stat.stat_v1k_max_dt );
	//	t->addAttribute( "all_v", g_V1kToStrUseValueForType( type, Stat.stat_v1k_max, getfortype, false ) );
	//	return;
	//}

	t->addAttribute( "max_t", (int)Stat.stat_v1k_max_dt);
	t->addAttribute( "max", g_V1kToStrUseValueForType(type,Stat.stat_v1k_max,getfortype,false) );

	t->addAttribute( "min_t", (int)Stat.stat_v1k_min_dt);
	t->addAttribute( "min", g_V1kToStrUseValueForType(type,Stat.stat_v1k_min,getfortype,false) );
	
	t->addAttribute( "avg", g_V1kToStrUseValueForType(type,Stat.stat_v1k_avg,getfortype,false,2) );
	t->addAttribute( "avg_n", Stat.stat_v1k_avg_num );
}

//m_spanrate(1), m_getfortype(1)

void XmppGSReport::ResetCodeDefault()
{
	m_spanrate = RUNCODE_Get(defCodeIndex_SYS_ReportDef_spanrate);
	m_ratefortype = RUNCODE_Get(defCodeIndex_SYS_ReportDef_ratefortype);
	m_getfortype = RUNCODE_Get(defCodeIndex_SYS_ReportDef_getfortype);
}

XmppGSReport::XmppGSReport( const struTagParam &TagParam )
	:StanzaExtension(ExtIotReport), m_method(defRPMethod_Unknown), m_result(defGSReturn_Err), m_TagParam(TagParam), 
	m_v1k_Value(0), m_Interval(0), m_getstatday(1), m_getstathour(0), m_getstatminute(0), m_getdatalist(1)
{
	ResetCodeDefault();
}

XmppGSReport::XmppGSReport( const Tag* tag )
	:StanzaExtension(ExtIotReport), m_method(defRPMethod_Unknown), m_result(defGSReturn_Err),
	m_dtBegin(-1,-1,-1), m_dtEnd(-1,-1,-1), 
	m_v1k_Value(0), m_Interval(0), m_getstatday(1), m_getstathour(0), m_getstatminute(0), m_getdatalist(1)
{
	ResetCodeDefault();
	if( !tag || tag->name() != "gsiot" || tag->xmlns() != XMLNS_GSIOT_Report )
		return;

	Tag *tmgr = tag->findChild("report");

	if( !tmgr )
	{
		m_result = defGSReturn_ErrParam;
		return;
	}
	
	// method
	if(tmgr->hasAttribute("method"))
	{
		const std::string srcmethod = tmgr->findAttribute("method");
		m_srcparam[srcmethod] = srcmethod;
		std::string method = srcmethod;
		g_toLowerCase( method );
		if( method == "min" )
		{
			m_method = defRPMethod_minute;
		}
		else if( method == "hour" )
		{
			m_method = defRPMethod_hour;
		}
		else if( method == "day" )
		{
			m_method = defRPMethod_day;
		}
		else if( method == "month" )
		{
			m_method = defRPMethod_month;
		}
		else if( method == "spectimeval" )
		{
			m_method = defRPMethod_SepcTimeVal;
		}
		else
		{
			m_result = defGSReturn_UnSupport;
			m_retErrInfo = "method unsupport";
			return;
		}
	}

	// obj
	Tag *tObj = tmgr->findChild("obj");
	if( tObj )
	{
		g_getAddrObjKey( tObj, m_AddrObjKey );
	}

	// begin and end
	Tag *tBegin = tmgr->findChild("begin");
	if( tBegin )
	{
		g_getGSTime( tBegin, m_dtBegin, true );
	}
	else
	{
		m_result = defGSReturn_ErrParam;
		m_retErrInfo = "begin err";
		return;
	}

	if( !m_dtBegin.IsValid() )
	{
		m_result = defGSReturn_ErrParam;
		m_retErrInfo = "begin err";
		return;
	}

	Tag *tEnd = tmgr->findChild("end");
	if( tEnd )
	{
		m_srcparam["end"] = "";
		g_getGSTime( tEnd, m_dtEnd, false );
	}

	if( defRPMethod_SepcTimeVal != m_method )
	{
		if( !m_dtEnd.IsValid() )
		{
			m_result = defGSReturn_ErrParam;
			m_retErrInfo = "end err";
			return;
		}
	}

	// interval
	const bool pget_interval = GetChildParamVal( tmgr, "interval", m_Interval );
	switch(m_method)
	{
	case defRPMethod_minute:
		{
			if( !pget_interval )
			{
				m_Interval = 1;
			}

			if( 1!=m_Interval )
			{
				m_result = defGSReturn_ErrParam;
				m_retErrInfo = "interval err";
				return;
			}

			// fix time
			//m_dtBegin.Second = 0;
			//m_dtEnd.Second = 59;
		}
		break;

	case defRPMethod_hour:
		{
			if( !pget_interval )
			{
				m_Interval = 15;
			}

			if( m_Interval<1 || m_Interval>60 )
			{
				m_result = defGSReturn_ErrParam;
				m_retErrInfo = "interval err";
				return;
			}

			// fix time
			//m_dtBegin.Minute = 0;
			//m_dtBegin.Second = 0;
			//m_dtEnd.Minute = 59;
			//m_dtEnd.Second = 59;
		}
		break;

	case defRPMethod_day:
		{
			if( !pget_interval )
			{
				m_Interval = 1;
			}

			if( m_Interval<1 || m_Interval>24 )
			{
				m_result = defGSReturn_ErrParam;
				m_retErrInfo = "interval err";
				return;
			}
			
			// fix time
			//m_dtBegin.Hour = 0;
			//m_dtBegin.Minute = 0;
			//m_dtBegin.Second = 0;
			//m_dtEnd.Hour = 24;
			//m_dtEnd.Minute = 59;
			//m_dtEnd.Second = 59;
		}
		break;

	case defRPMethod_month:
		{
			if( !pget_interval )
			{
				m_Interval = 30;
			}

			if( 1!=m_Interval && 30!=m_Interval )
			{
				m_result = defGSReturn_ErrParam;
				m_retErrInfo = "interval err";
				return;
			}
		}
		break;

	case defRPMethod_SepcTimeVal:
		{
		}
		break;
	}

	// ex param
	float flt = 0;
	GetChildParamVal( tmgr, "getstatday", m_getstatday );
	GetChildParamVal( tmgr, "getstathour", m_getstathour );
	GetChildParamVal( tmgr, "getstatminute", m_getstatminute );
	GetChildParamVal( tmgr, "getdatalist", m_getdatalist );
	if( GetChildParamVal_float(tmgr, "spanrate", flt) ) m_spanrate = flt * 1000;
	GetChildParamVal( tmgr, "ratefortype", m_ratefortype );
	GetChildParamVal( tmgr, "getfortype", m_getfortype );

	m_result = defGSReturn_Success;
}

XmppGSReport::~XmppGSReport(void)
{
	g_delete_mapRec_stat_day( m_mapRec_stat_day );
	g_delete_mapRec_stat_month( m_mapRec_stat_month );
}

void XmppGSReport::PrintTag( const Tag* tag, const Stanza *stanza ) const
{
	if( tag ) GSIOTClient::XmppPrint( tag, "recv", stanza, false );
}

const std::string& XmppGSReport::filterString() const
{
	static const std::string filter = "/iq/gsiot[@xmlns='" + XMLNS_GSIOT_Report + "']";
	return filter;
}

Tag* XmppGSReport::tag() const
{
	std::string key;
	std::string strval;

	Tag* i = new Tag( "gsiot" );
	i->setXmlns( XMLNS_GSIOT_Report );

	Tag *tmgr = new Tag( i,"report" );

	key = "method";
	if( GetSrcParam( key, strval ) )
	{
		tmgr->addAttribute( key, strval );
	}

	if( m_TagParam.isValid && m_TagParam.isResult )
	{
		if( !macGSIsReturnNull(m_result) )
		{
			tmgr->addAttribute( "ret", util::int2string(m_result) );
			tmgr->addAttribute( "errinfo", m_retErrInfo );
		}
	}

	Tag *tobj = new Tag( tmgr, "obj" );
	g_tagAddrObjKey( tobj, m_AddrObjKey );

	Tag *tBegin = new Tag( tmgr, "begin" );
	g_tagGSTime( tBegin, m_dtBegin );

	key = "end";
	if( GetSrcParam( key, strval ) )
	{
		Tag *tEnd = new Tag( tmgr, "end" );
		g_tagGSTime( tEnd, m_dtEnd );
	}

	key = "interval";
	if( GetSrcParam( key, strval ) )
	{
		new Tag(tmgr, key, util::int2string(m_Interval));
		//new Tag( tmgr, key, strval );
	}

	key = "getstatday";
	if( GetSrcParam( key, strval ) )
	{
		new Tag(tmgr, key, util::int2string(m_getstatday));
		//new Tag( tmgr, key, strval );
	}

	key = "getstathour";
	if( GetSrcParam( key, strval ) )
	{
		new Tag(tmgr, key, util::int2string(m_getstathour));
		//new Tag( tmgr, key, strval );
	}
	
	key = "getstatminute";
	if( GetSrcParam( key, strval ) )
	{
		new Tag( tmgr, key, util::int2string( m_getstatminute ) );
		//new Tag( tmgr, key, strval );
	}

	key = "getdatalist";
	if( GetSrcParam( key, strval ) )
	{
		new Tag(tmgr, key, util::int2string(m_getdatalist));
		//new Tag( tmgr, key, strval );
	}

	key = "spanrate";
	if( GetSrcParam( key, strval ) )
	{
		new Tag(tmgr, key, g_V1kToStr(m_spanrate));
		//new Tag( tmgr, key, strval );
	}

	key = "ratefortype";
	if( GetSrcParam( key, strval ) )
	{
		new Tag(tmgr, key, util::int2string(m_ratefortype));
		//new Tag( tmgr, key, strval );
	}

	key = "getfortype";
	if( GetSrcParam( key, strval ) )
	{
		new Tag( tmgr, key, util::int2string( m_getfortype ) );
		//new Tag( tmgr, key, strval );
	}
	
	if( macGSFailed(m_result) )
	{
		return i;
	}

	Tag *tResult = new Tag( tmgr, "result" );
	
	if( defRPMethod_SepcTimeVal == m_method )
	{
		new Tag( tResult, "value", g_V1kToStrUseValueForType(m_AddrObjKey.address_type,m_v1k_Value,m_getfortype,false) );

		return i;
	}

	Tag *tStatbase = new Tag( tResult, "statbase" );
	if( tStatbase )
	{
		g_tagStat( tStatbase, m_ResultStat.Stat, m_AddrObjKey.address_type, m_getfortype );
	}

	if( defRPMethod_month==m_method )
	{
		Tag *tStatmonth = new Tag( tResult, "statmonth" );
		if( tStatmonth )
		{
			bool hasValid = false;
			for( defmapRec_stat_month::const_iterator it=m_mapRec_stat_month.begin(); it!=m_mapRec_stat_month.end(); ++it )
			{
				if( !hasValid )
				{
					if( !it->second->Stat.stat_valid )
						continue;
				}
				hasValid = true;

				Tag *tMonth = new Tag( tStatmonth, "m" );

				if( !tMonth )
					continue;

				g_tagGSTime( tMonth, struGSTime(it->second->key.dt_year,it->second->key.dt_month,1), true,true,false,false,false );

				tMonth->addAttribute( "a", util::int2string(it->second->Stat.stat_valid) );

				g_tagStat( tMonth, it->second->Stat, m_AddrObjKey.address_type, m_getfortype );
			}
		}
	}

	if( m_getstatday && (defRPMethod_month==m_method || defRPMethod_day==m_method) )
	{
		Tag *tStatday = new Tag( tResult, "statday" );
		if( tStatday )
		{
			bool hasValid = false;
			for( defmapRec_stat_day::const_iterator it=m_mapRec_stat_day.begin(); it!=m_mapRec_stat_day.end(); ++it )
			{
				if( !hasValid )
				{
					if( !it->second->Stat.stat_valid  )
						continue;
				}
				hasValid = true;

				Tag *tDay = new Tag( tStatday, "d" );

				if( !tDay )
					continue;

				g_tagGSTime( tDay, struGSTime(it->second->key.dt_year,it->second->key.dt_month,it->second->key.dt_day), true,true,true,false,false );

				tDay->addAttribute( "a", util::int2string(it->second->Stat.stat_valid) );

				g_tagStat( tDay, it->second->Stat, m_AddrObjKey.address_type, m_getfortype );

				// hour
				if( m_getstatday && defRPMethod_day==m_method )
				{
					for( defmapstruStat::const_iterator itH=it->second->lst_stathour.begin(); itH!=it->second->lst_stathour.end(); ++itH )
					{
						Tag *tHour = new Tag( tDay, "h" );

						if( !tHour )
							continue;

						g_tagGSTime( tHour, itH->first, false,false,false,true,false );

						tHour->addAttribute( "a", util::int2string(itH->second.stat_valid) );

						g_tagStat( tHour, itH->second, m_AddrObjKey.address_type, m_getfortype );
					}
				}
			}
		}
	}

	if( m_getstatminute && defRPMethod_hour==m_method )
	{
		Tag *tStatminute = new Tag( tResult, "statminute" );
		if( tStatminute )
		{
			bool hasValid = false;
			for( defmapstruStat::const_iterator it=m_lst_stat.begin(); it!=m_lst_stat.end(); ++it )
			{
				if( !hasValid )
				{
					if( !it->second.stat_valid )
						continue;
				}
				hasValid = true;

				Tag *tMin = new Tag( tStatminute, "n" );

				if( !tMin )
					continue;

				g_tagGSTime( tMin, it->first, false, false, false, false, true );

				tMin->addAttribute( "a", util::int2string( it->second.stat_valid ) );

				g_tagStat( tMin, it->second, m_AddrObjKey.address_type, m_getfortype, true );
			}
		}
	}

	if( m_getdatalist )
	{
		Tag *tDatalist = new Tag( tResult, "datalist" );
		if( tDatalist )
		{
			tDatalist->addAttribute( "num", util::int2string( m_ResultStat.lst_query_v1k.size() ) );

			bool hasValid = false;
			for( defmapV1k::const_iterator it=m_ResultStat.lst_query_v1k.begin(); it!=m_ResultStat.lst_query_v1k.end(); ++it )
			{
				if( !hasValid )
				{
					if( !it->second.v1k_valid )
						continue;
				}
				hasValid = true;

				Tag *tData = new Tag( tDatalist, "d" );

				if( !tData )
					continue;

				tData->addAttribute( "t", util::int2string(it->second.data_dt) );
				tData->addAttribute( "a", util::int2string(it->second.v1k_valid?1:0) );
				tData->addAttribute( "v", g_V1kToStrUseValueForType(m_AddrObjKey.address_type,it->second.v1k,m_getfortype,false) );
			}
		}
	}

	return i;
}

bool XmppGSReport::GetChildParamVal( Tag *tmgr, const std::string &key, int &val )
{
	Tag *pChild = tmgr->findChild(key);
	if( pChild )
	{
		const std::string srcdata = pChild->cdata();
		m_srcparam[key] = srcdata;
		val = atoi( srcdata.c_str() );
		return true;
	}

	return false;
}

bool XmppGSReport::GetChildParamVal_float( Tag *tmgr, const std::string &key, float &val )
{
	Tag *pChild = tmgr->findChild(key);
	if( pChild )
	{
		const std::string srcdata = pChild->cdata();
		m_srcparam[key] = srcdata;
		val = atof( srcdata.c_str() );
		return true;
	}

	return false;
}

bool XmppGSReport::GetSrcParam( const std::string &key, std::string &val ) const
{
	std::map<std::string,std::string>::const_iterator it = m_srcparam.find(key);
	if( it != m_srcparam.end() )
	{
		val = it->second;
		return true;
	}

	return false;
}
