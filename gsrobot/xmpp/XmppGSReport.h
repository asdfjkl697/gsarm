#pragma once
#include "../typedef.h"
#include "gloox/stanzaextension.h"
#include "gloox/tag.h"
#include "../DataStoreMgr.h"

using namespace gloox;

class XmppGSReport :
	public StanzaExtension
{
public:
	enum defRPMethod
	{
		defRPMethod_Unknown	= 0,
		defRPMethod_minute,			// ��������ϸ��ݲ�ѯ
		defRPMethod_hour,			// Сʱʱ�䷶Χ��ѯ
		defRPMethod_day,			// ��ʱ�䷶Χ��ѯ
		defRPMethod_month,			// ��ʱ�䷶Χ��ѯ
		defRPMethod_SepcTimeVal,	// ָ��ʱ���ֵ
	};

public:
	XmppGSReport( const struTagParam &TagParam );
	XmppGSReport( const Tag* tag );
	~XmppGSReport(void);

	// reimplemented from StanzaExtension
	virtual const std::string& filterString() const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* newInstance( const Tag* tag ) const
	{
		return new XmppGSReport( tag );
	}

	void PrintTag( const Tag* tag, const Stanza *stanza ) const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* clone() const
	{
		XmppGSReport *pnew = new XmppGSReport(NULL);
		*pnew = *this;
		return pnew;
	}

	// reimplemented from StanzaExt
	virtual Tag* tag() const;
	
public:
	std::map<std::string,std::string> m_srcparam; // <key,val>

	defRPMethod m_method;
	GSIOTAddrObjKey m_AddrObjKey;
	struGSTime m_dtBegin;
	struGSTime m_dtEnd;
	int m_Interval; // spanmin/spanhour/spanday
	int m_getstatday;
	int m_getstathour;
	int m_getstatminute;
	int m_getdatalist;
	int m_spanrate; // ǧ��ֵ��1=0.001
	int m_ratefortype;	// �������ͷ�����ݱ仯
	int m_getfortype;	// ��������ת��ֵ����

	struTagParam m_TagParam;
	stru_QueryStat m_ResultStat;
	int m_v1k_Value;
	defmapRec_stat_day m_mapRec_stat_day;
	defmapRec_stat_month m_mapRec_stat_month;
	defmapstruStat m_lst_stat;
	defGSReturn m_result;
	std::string m_retErrInfo;

	void CopyParam( const XmppGSReport &other )
	{
		m_srcparam = other.m_srcparam;

		m_method = other.m_method;
		m_AddrObjKey = other.m_AddrObjKey;
		m_dtBegin = other.m_dtBegin;
		m_dtEnd = other.m_dtEnd;
		m_Interval = other.m_Interval;
		m_getstatday = other.m_getstatday;
		m_getstathour = other.m_getstathour;
		m_getstatminute = other.m_getstatminute;
		m_getdatalist = other.m_getdatalist;
		m_spanrate = other.m_spanrate;
		m_ratefortype = other.m_ratefortype;
		m_getfortype = other.m_getfortype;
	}

	void ResetCodeDefault();

private:
	bool GetChildParamVal( Tag *tmgr, const std::string &key, int &val );
	bool GetChildParamVal_float( Tag *tmgr, const std::string &key, float &val );
	bool GetSrcParam( const std::string &key, std::string &val ) const;
};
