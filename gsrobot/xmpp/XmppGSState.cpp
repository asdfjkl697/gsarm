#include "XmppGSState.h"
#include "../gsiot/GSIOTClient.h"
#include "gloox/util.h"
#include "../typedef.h"


static bool g_getAGTime( Tag *t, int &flag, int &bh, int &bm, int &eh, int &em )
{
	if( t->hasAttribute("f") )
	{
		flag = atoi( t->findAttribute("f").c_str() );
	}

	if( t->hasAttribute("bh") )
	{
		bh = atoi( t->findAttribute("bh").c_str() );
		if( bh<0 || bh>23 )
		{
			bh = 0;
			return false;
		}
	}

	if( t->hasAttribute("bm") )
	{
		bm = atoi( t->findAttribute("bm").c_str() );
		if( bm<0 || bm>59 )
		{
			bm = 0;
			return false;
		}
	}

	if( t->hasAttribute("eh") )
	{
		eh = atoi( t->findAttribute("eh").c_str() );
		if( eh<0 || eh>23 )
		{
			eh = 0;
			return false;
		}
	}

	if( t->hasAttribute("em") )
	{
		em = atoi( t->findAttribute("em").c_str() );
		if( em<0 || em>59 )
		{
			em = 0;
			return false;
		}
	}

	return true;
}

XmppGSState::XmppGSState( const struTagParam &TagParam, uint32_t state_board, uint32_t state_mobile, uint32_t state_events, GSAGCurState_ AGCurState, uint32_t state_starttime, const std::list<GSIOTDevice*>& deviceList )
	:StanzaExtension(ExtIotState), m_TagParam(TagParam), m_cmd(defStateCmd_Unknown), m_state_board(state_board), m_state_mobile(state_mobile), m_state_events(state_events), m_AGCurState(AGCurState), m_state_starttime(state_starttime), m_deviceList( deviceList )
{
}

XmppGSState::XmppGSState( const Tag* tag )
	:StanzaExtension(ExtIotState), m_cmd(defStateCmd_Unknown), m_state_board(1), m_state_mobile(1), m_state_events(1), m_AGCurState(GSAGCurState_AllArmed), m_state_starttime(0)
{
	if( !tag || tag->name() != "gsiot" || tag->xmlns() != XMLNS_GSIOT_STATE )
		return;

	Tag *tmgr = tag->findChild("state");

	if( tmgr )
	{
		// first
		if( tmgr->hasChild("reboot") )
		{
			m_cmd = defStateCmd_reboot;
		}

		if( tmgr->hasChild( "exitlearnmod" ) )
		{
			m_cmd = defStateCmd_exitlearnmod;
		}

		Tag *tstate_alarmguard = tmgr->findChild("alarmguard");
		if( tstate_alarmguard )
		{
			m_cmd = defStateCmd_alarmguard;

			const TagList& lw = tstate_alarmguard->children();
			TagList::const_iterator it = lw.begin();
			for( ; it != lw.end(); ++it )
			{
				Tag *tAGW = (*it);

				int w = -1;
				if( tAGW->name() == "w1" )
					w = 1;
				else if( tAGW->name() == "w2" )
					w = 2;
				else if( tAGW->name() == "w3" )
					w = 3;
				else if( tAGW->name() == "w4" )
					w = 4;
				else if( tAGW->name() == "w5" )
					w = 5;
				else if( tAGW->name() == "w6" )
					w = 6;
				else if( tAGW->name() == "w7" )
					w = 7;
				else
					continue;

				this->m_mapAGTime[w] = struAGTime();

				if( tAGW->hasAttribute("ad") )
				{
					this->m_mapAGTime[w].allday = atoi( tAGW->findAttribute("ad").c_str() );
				}
				
				//.resetallday//bool allinvalid = true;
				const TagList& l = tAGW->children();
				TagList::const_iterator it = l.begin();
				for( ; it != l.end(); ++it )
				{
					Tag *tChild = (*it);
				
					// <t  bh='' bm='' eh='' em='' />
					if( tChild->name() == "t" )
					{
						int flag = 1;
						int start_h = 0;
						int start_m = 0;
						int stop_h = 0;
						int stop_m = 0;

						g_getAGTime( tChild, flag, start_h, start_m, stop_h, stop_m );

						const int agTime_b = start_h*100 + start_m;
						const int agTime_e = stop_h*100 + stop_m;

						//.resetallday//if( allinvalid ) allinvalid = allinvalid && macAlarmGuardTime_InvaildAG(flag,agTime_b,agTime_e);
						
						if( start_h!=stop_h || start_m!=stop_m)
						{
							const int agTime = flag*100000000 + agTime_b*10000 + agTime_e;
							this->m_mapAGTime[w].vecagTime.push_back( agTime );
						}
						else
						{
							this->m_mapAGTime[w].vecagTime.push_back( int(0) );
						}
					}
				}
			}
		}

		Tag *tstate_events = tmgr->findChild("events");
		if( tstate_events )
		{
			m_state_events = atoi( tstate_events->cdata().c_str() );
			m_cmd = defStateCmd_events;
		}
	}
}

XmppGSState::~XmppGSState(void)
{
}

void XmppGSState::PrintTag( const Tag* tag, const Stanza *stanza ) const
{
	if( tag ) GSIOTClient::XmppPrint( tag, "recv", stanza, false );
}

const std::string& XmppGSState::filterString() const
{
	static const std::string filter = "/iq/gsiot[@xmlns='" + XMLNS_GSIOT_STATE + "']";
	return filter;
}

Tag* XmppGSState::tag() const
{
	char buf[64] = {0};

	Tag* i = new Tag( "gsiot" );
	i->setXmlns( XMLNS_GSIOT_STATE );

	Tag *tmgr = new Tag( i,"state" );

	Tag *tstate_board = new Tag( tmgr, "board", util::int2string(m_state_board) );
	//tstate_board->addAttribute( "readtype", 1 );

	Tag *tstate_mobile = new Tag( tmgr, "mobile", util::int2string(m_state_mobile) );
	//tstate_mobile->addAttribute( "readtype", 1 );

	Tag *tstate_events = new Tag( tmgr, "events", util::int2string(m_state_events) );
	//tstate_events->addAttribute( "readtype", 3 );
	tstate_events->addAttribute( "curstate", util::int2string(m_AGCurState) );
	
	Tag *tstate_starttime = new Tag( tmgr, "starttime", util::int2string(m_state_starttime) );
	//tstate_starttime->addAttribute( "readtype", 1 );

	new Tag( tmgr, "curtime", util::int2string(g_GetUTCTime()) );

	// alarmguard
	Tag *tstate_alarmguard = new Tag( tmgr, "alarmguard" );
	for( int nW=1; nW<=7; ++nW )
	{
		snprintf( buf, sizeof(buf), "w%d", nW );

		Tag *tstate_alarmguard_time = new Tag( tstate_alarmguard, buf );

		tstate_alarmguard_time->addAttribute( "ad", RUNCODE_Get(g_AlarmGuardTimeWNum2Index(nW)) );

		std::vector<uint32_t> vecFlag;
		std::vector<uint32_t> vecBegin;
		std::vector<uint32_t> vecEnd;
		g_GetAlarmGuardTime( g_AlarmGuardTimeWNum2Index(nW), vecFlag, vecBegin, vecEnd );

		// ������Чʱ������
		for( int i=0; i<defAlarmGuard_AGTimeCount; ++i )
		{
			if( !vecFlag[i] && vecBegin[i] == vecEnd[i] ) // ��־��Ч����ֵ��Ч�Ĳ��ϴ���ֻҪֵ��Ч���ܱ�־�Ƿ���Ч���ϴ�
			{
				continue;
			}

			const int Begin_hour = vecBegin[i]/100;
			const int Begin_minute = vecBegin[i]%100;

			const int End_hour = vecEnd[i]/100;
			const int End_minute = vecEnd[i]%100;

			// <t  bh='' bm='' eh='' em='' />
			Tag *tstate_agtime = new Tag( tstate_alarmguard_time, "t" );

			tstate_agtime->addAttribute( "f", int(vecFlag[i]) );

			tstate_agtime->addAttribute( "bh", Begin_hour );
			tstate_agtime->addAttribute( "bm", Begin_minute );

			tstate_agtime->addAttribute( "eh", End_hour );
			tstate_agtime->addAttribute( "em", End_minute );
		}
	}

	Tag *teventlist = new Tag( tmgr, "eventlist" );
	if( !m_deviceList.empty() )
	{
		std::list<GSIOTDevice*>::const_iterator it = m_deviceList.begin();
		for( ; it!=m_deviceList.end(); ++it )
		{
			int id = -1;
			defAGRunStateFinalST AGRunStateFinalST = defAGRunStateFinalST_UnAg;

			if( (*it)->getControl() && GSIOTDevice::IsSupportAlarm( (*it) ) )
			{
				switch( (*it)->getType() )
				{
				case IOT_DEVICE_Trigger:
					{
						TriggerControl *ctl = (TriggerControl*)(*it)->getControl();
						id = (*it)->getId();
						AGRunStateFinalST = ctl->GetAGRunStateFinalST();
					}
					break;

				case IOT_DEVICE_Camera:
					{

					}
					break;

				default:
					break;
				}
			}

			if( id > 0 )
			{
				Tag *teventdevice = new Tag(teventlist, "dev");
				teventdevice->addAttribute( "type", (*it)->getType() );
				teventdevice->addAttribute( "id", id );
				teventdevice->addAttribute( "st", AGRunStateFinalST );
			}
		}
	}

	return i;
}
