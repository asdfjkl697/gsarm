#include "GSIOTEvent.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include <string>
#include <functional>


struct sort_ControlEvent : std::greater < ControlEvent* >
{
	bool operator()(const ControlEvent *_X, const ControlEvent *_Y) const
	{
		return (_X->GetLevel() > _Y->GetLevel());
	}
};

GSIOTEvent::GSIOTEvent(void)
{
	LoadDB_event_sendsms();
	LoadDB_event_notice();
	LoadDB_event_autocontrol();
	LoadDB_event_eventthing();
	LoadDB_event_call();

	this->SortEvents();
}


GSIOTEvent::~GSIOTEvent(void)
{
	if( !m_event.empty() )
	{
		for( std::list<ControlEvent*>::iterator it=m_event.begin(); it!=m_event.end(); ++it )
		{
			delete *it;
		}
		m_event.clear();
	}
}

std::string GSIOTEvent::GetEventTypeToString(EventType type)
{
	switch(type)
	{
	case SMS_Event:
		return "短信发送";

	case EMAIL_Event:
		return "邮件发送";

	case NOTICE_Event:
		return "客户端通知";

	case CONTROL_Event:
		return "设备控制";

	case Eventthing_Event:
		return "触发器状态";
	}

	return "";
}

void GSIOTEvent::SortEvents_spec( std::list<ControlEvent*> &events )
{
	events.sort( sort_ControlEvent() );
}

void GSIOTEvent::SortEvents()
{
	SortEvents_spec( m_event );
}

bool GSIOTEvent::DeleteDeviceEvent( IOTDeviceType devtype, int id )
{
	char sqlbuf[256] = {0};

	snprintf( sqlbuf, sizeof(sqlbuf), "DELETE FROM event_sendsms where device_type=%d and device_id=%d", devtype, id );
	db->exec( sqlbuf );

	snprintf( sqlbuf, sizeof(sqlbuf), "DELETE FROM event_notice where device_type=%d and device_id=%d", devtype, id );
	db->exec( sqlbuf );
	
	snprintf( sqlbuf, sizeof(sqlbuf), "DELETE FROM event_autocontrol where device_type=%d and device_id=%d", devtype, id );
	db->exec( sqlbuf );

	snprintf( sqlbuf, sizeof(sqlbuf), "DELETE FROM event_eventthing where device_type=%d and device_id=%d", devtype, id );
	db->exec( sqlbuf );
	
	snprintf( sqlbuf, sizeof(sqlbuf), "DELETE FROM event_call where device_type=%d and device_id=%d", devtype, id );
	db->exec( sqlbuf );

	return true;
}

void GSIOTEvent::AddEvent(ControlEvent *evt)
{
	if(evt){
	    m_event.push_back(evt);
		this->SaveToDb(evt);
	}
}

void GSIOTEvent::DeleteEvent(ControlEvent *evt)
{
	char sqlbuf[256] = {0};

	if(evt){
		if(evt->GetID()>0){
			switch(evt->GetType())
			{
			case SMS_Event:
				{
					snprintf( sqlbuf, sizeof(sqlbuf), "DELETE FROM event_sendsms WHERE id=%d", evt->GetID() );
					break;
				}

			case EMAIL_Event:
				break;

			case NOTICE_Event:
				{
					snprintf( sqlbuf, sizeof(sqlbuf), "DELETE FROM event_notice WHERE id=%d", evt->GetID() );
				    break;
				}

			case CONTROL_Event:
				{
					snprintf( sqlbuf, sizeof(sqlbuf), "DELETE FROM event_autocontrol WHERE id=%d", evt->GetID() );
				    break;
				}

			case Eventthing_Event:
				{
					snprintf( sqlbuf, sizeof(sqlbuf), "DELETE FROM event_eventthing WHERE id=%d", evt->GetID() );
					break;
				}

			case CALL_Event:
				{
					snprintf( sqlbuf, sizeof(sqlbuf), "DELETE FROM event_call WHERE id=%d", evt->GetID() );
					break;
				}

			default:
				return ;
			}

			db->exec( sqlbuf );
			this->m_event.remove( evt );
			delete evt;
		}
	}
}

bool GSIOTEvent::ModifyEvent(ControlEvent *evt, const ControlEvent *evtsrc)
{
	if( !evt )
		return false;
	
	if( evtsrc 
		&& evt->GetType() == evtsrc->GetType() )
	{
		evt->SetLevel( evtsrc->GetLevel() );
		evt->SetForce( evtsrc->isForce() );
		evt->SetEnable( evtsrc->GetEnable() );
		evt->SetDoInterval( evtsrc->GetDoInterval() );
	}

	switch(evt->GetType())
	{
	case SMS_Event:
		{
			AutoSendSMSEvent *aevt = (AutoSendSMSEvent*)evt;

			if( evtsrc 
				&& evt->GetType() == evtsrc->GetType() )
			{
				const AutoSendSMSEvent *aevtsrc = (AutoSendSMSEvent*)evtsrc;

				aevt->SetPhone( aevtsrc->GetPhone() );
				aevt->SetSMS( aevtsrc->GetSMS() );
			}

			SQLite::Statement query( *this->db, "UPDATE event_sendsms SET level=?,enable=?,do_interval=?,"\
				"flag=?,send_count=?,phone=?,sms=? WHERE id=?" );

			int col = 1;
			query.bind(col++, (int)aevt->get_level_col_save());
			query.bind(col++, (int)aevt->GetEnable());
			query.bind(col++, (int)aevt->GetDoInterval());

			query.bind(col++, (int)aevt->GetFlag());
			query.bind(col++, (int)aevt->GetSendCount());
			query.bind(col++, aevt->GetPhone());
			query.bind(col++, aevt->GetSMS());

			query.bind(col++, (int)evt->GetID());
			query.exec();
			break;
		}

	case NOTICE_Event:
		{
			AutoNoticeEvent *aevt = (AutoNoticeEvent*)evt;

			if( evtsrc 
				&& evt->GetType() == evtsrc->GetType() )
			{
				const AutoNoticeEvent *aevtsrc = (AutoNoticeEvent*)evtsrc;
				aevt->SetToJid( aevtsrc->GetToJid() );		
				aevt->SetSubject( aevtsrc->GetSubject() );
				aevt->SetBody( aevtsrc->GetBody() );
			}

			SQLite::Statement query( *this->db, "UPDATE event_notice SET level=?,enable=?,do_interval=?,"\
				"to_jid=?,msg_subject=?,msg_body=? WHERE id=?" );

			int col = 1;
			query.bind(col++, (int)aevt->get_level_col_save());
			query.bind(col++, (int)aevt->GetEnable());
			query.bind(col++, (int)aevt->GetDoInterval());

			query.bind(col++, aevt->GetToJid());
			query.bind(col++, aevt->GetSubject());
			query.bind(col++, aevt->GetBody());

			query.bind(col++, (int)evt->GetID());
			query.exec();
		}
		break;

	case CONTROL_Event:
		{
			AutoControlEvent *aevt = (AutoControlEvent*)evt;

			if( evtsrc 
				&& evt->GetType() == evtsrc->GetType() )
			{
				const AutoControlEvent *aevtsrc = (AutoControlEvent*)evtsrc;

				aevt->SetControlDeviceType( aevtsrc->GetControlDeviceType() );
				aevt->SetControlDeviceId( aevtsrc->GetControlDeviceId() );
				aevt->SetAddress( aevtsrc->GetAddress() );
				aevt->SetValue( aevtsrc->GetValue() );
			}

			SQLite::Statement query( *this->db, "UPDATE event_autocontrol SET level=?,enable=?,do_interval=?,"\
				"ctrl_devtype=?,ctrl_devid=?,address=?,value=? WHERE id=?" );

			int col = 1;
			query.bind(col++, (int)aevt->get_level_col_save());
			query.bind(col++, (int)aevt->GetEnable());
			query.bind(col++, (int)aevt->GetDoInterval());

			query.bind(col++, (int)aevt->GetControlDeviceType());
			query.bind(col++, (int)aevt->GetControlDeviceId());
			query.bind(col++, (int)aevt->GetAddress());
			query.bind(col++, aevt->GetValue());

			query.bind(col++, (int)evt->GetID());
			query.exec();
		}
		break;
		
	case Eventthing_Event:
		{
			AutoEventthing *aevt = (AutoEventthing*)evt;

			if( evtsrc 
				&& evt->GetType() == evtsrc->GetType() )
			{
				const AutoEventthing *aevtsrc = (AutoEventthing*)evtsrc;

				aevt->SetControlDeviceType( aevtsrc->GetControlDeviceType() );
				aevt->SetControlDeviceId( aevtsrc->GetControlDeviceId() );
				aevt->SetRunState( aevtsrc->GetRunState() );
			}

			SQLite::Statement query( *this->db, "UPDATE event_eventthing SET level=?,enable=?,do_interval=?,"\
				"ctrl_devtype=?,ctrl_devid=?,runstate=? WHERE id=?" );

			int col = 1;
			query.bind(col++, (int)aevt->get_level_col_save());
			query.bind(col++, (int)aevt->GetEnable());
			query.bind(col++, (int)aevt->GetDoInterval());

			query.bind(col++, (int)aevt->GetControlDeviceType());
			query.bind(col++, (int)aevt->GetControlDeviceId());
			query.bind(col++, (int)aevt->GetRunState());

			query.bind(col++, (int)evt->GetID());
			query.exec();
		}
		break;

	case CALL_Event:
		{
			AutoCallEvent *aevt = (AutoCallEvent*)evt;

			if( evtsrc 
				&& evt->GetType() == evtsrc->GetType() )
			{
				const AutoCallEvent *aevtsrc = (AutoCallEvent*)evtsrc;

				aevt->Set_try_count( aevtsrc->Get_try_count() );
				aevt->Set_play_count( aevtsrc->Get_play_count() );
				aevt->SetFlag1( aevtsrc->GetFlag1() );
				aevt->SetFlag2( aevtsrc->GetFlag2() );
				aevt->SetPhone( aevtsrc->GetPhone() );
				aevt->SetContent( aevtsrc->GetContent() );
			}

			SQLite::Statement query( *this->db, "UPDATE event_call SET level=?,enable=?,do_interval=?,"\
				"try_count=?,play_count=?,flag1=?,flag2=?,phone=?,content=? WHERE id=?" );

			int col = 1;
			query.bind(col++, (int)aevt->get_level_col_save());
			query.bind(col++, (int)aevt->GetEnable());
			query.bind(col++, (int)aevt->GetDoInterval());

			query.bind(col++, (int)aevt->Get_try_count());
			query.bind(col++, (int)aevt->Get_play_count());
			query.bind(col++, (int)aevt->GetFlag1());
			query.bind(col++, (int)aevt->GetFlag2());
			query.bind(col++, aevt->GetPhone());
			query.bind(col++, aevt->GetContent());

			query.bind(col++, (int)evt->GetID());
			query.exec();
		}
		break;

	default:
		return false;
	}

	return true;
}

void GSIOTEvent::SaveToDb(ControlEvent *evt)
{
	switch(evt->GetType())
	{
	case SMS_Event:
		{
			AutoSendSMSEvent *aevt = (AutoSendSMSEvent *)evt;
			SQLite::Statement query(*this->db,"INSERT INTO event_sendsms VALUES(NULL,:device_type,:device_id,:level,:enable,:do_interval,"\
				":flag,:send_count,:phone,:sms)");

			int col = 1;
			query.bind(col++, (int)aevt->GetDeviceType());
			query.bind(col++, (int)aevt->GetDeviceID());
			query.bind(col++, (int)aevt->get_level_col_save());
			query.bind(col++, (int)aevt->GetEnable());
			query.bind(col++, (int)aevt->GetDoInterval());
			
			query.bind(col++, (int)aevt->GetFlag());
			query.bind(col++, (int)aevt->GetSendCount());
			query.bind(col++, aevt->GetPhone());
			query.bind(col++, aevt->GetSMS());
			query.exec();

			int id = db->execAndGet("SELECT id FROM event_sendsms ORDER BY id DESC LIMIT 1");
			evt->SetID(id);
			break;
		}
		break;

	case EMAIL_Event:
		break;

	case NOTICE_Event:
		{
			AutoNoticeEvent *aevt = (AutoNoticeEvent *)evt;
			SQLite::Statement query(*this->db,"INSERT INTO event_notice VALUES(NULL,:device_type,:device_id,:level,:enable,:do_interval,"\
				":to_jid,:msg_subject,:msg_body)");

			int col = 1;
			query.bind(col++, (int)aevt->GetDeviceType());
			query.bind(col++, (int)aevt->GetDeviceID());
			query.bind(col++, (int)aevt->get_level_col_save());
			query.bind(col++, (int)aevt->GetEnable());
			query.bind(col++, (int)aevt->GetDoInterval());

			query.bind(col++, aevt->GetToJid());
			query.bind(col++, aevt->GetSubject());
			query.bind(col++, aevt->GetBody());
			query.exec();

			int id = db->execAndGet("SELECT id FROM event_notice ORDER BY id DESC LIMIT 1");
			evt->SetID(id);
			break;
		}

	case CONTROL_Event:
		{
			AutoControlEvent *aevt = (AutoControlEvent *)evt;
			SQLite::Statement query(*this->db,"INSERT INTO event_autocontrol VALUES(NULL,:device_type,:device_id,:level,:enable,:do_interval,"\
				":ctrl_devtype,:ctrl_devid,:address,:value)");

			int col = 1;
			query.bind(col++, (int)aevt->GetDeviceType());
			query.bind(col++, (int)aevt->GetDeviceID());
			query.bind(col++, (int)aevt->get_level_col_save());
			query.bind(col++, (int)aevt->GetEnable());
			query.bind(col++, (int)aevt->GetDoInterval());

			query.bind(col++, (int)aevt->GetControlDeviceType());
			query.bind(col++, (int)aevt->GetControlDeviceId());
			query.bind(col++, (int)aevt->GetAddress());
			query.bind(col++, aevt->GetValue());
			query.exec();

			int id = db->execAndGet("SELECT id FROM event_autocontrol ORDER BY id DESC LIMIT 1");
			evt->SetID(id);
			break;
		}

	case Eventthing_Event:
		{
			AutoEventthing *aevt = (AutoEventthing *)evt;
			SQLite::Statement query(*this->db,"INSERT INTO event_eventthing VALUES(NULL,:device_type,:device_id,:level,:enable,:do_interval,"\
				":ctrl_devtype,:ctrl_devid,:runstate)");

			int col = 1;
			query.bind(col++, (int)aevt->GetDeviceType());
			query.bind(col++, (int)aevt->GetDeviceID());
			query.bind(col++, (int)aevt->get_level_col_save());
			query.bind(col++, (int)aevt->GetEnable());
			query.bind(col++, (int)aevt->GetDoInterval());

			query.bind(col++, (int)aevt->GetControlDeviceType());
			query.bind(col++, (int)aevt->GetControlDeviceId());
			query.bind(col++, (int)aevt->GetRunState());
			query.exec();

			int id = db->execAndGet("SELECT id FROM event_eventthing ORDER BY id DESC LIMIT 1");
			evt->SetID(id);
			break;
		}
		break;

	case CALL_Event:
		{
			AutoCallEvent *aevt = (AutoCallEvent*)evt;
			SQLite::Statement query(*this->db,"INSERT INTO event_call VALUES(NULL,:device_type,:device_id,:level,:enable,:do_interval,"\
				":try_count,:play_count,:flag1,:flag2,:phone,:content)");

			int col = 1;
			query.bind(col++, (int)aevt->GetDeviceType());
			query.bind(col++, (int)aevt->GetDeviceID());
			query.bind(col++, (int)aevt->get_level_col_save());
			query.bind(col++, (int)aevt->GetEnable());
			query.bind(col++, (int)aevt->GetDoInterval());

			query.bind(col++, (int)aevt->Get_try_count());
			query.bind(col++, (int)aevt->Get_play_count());
			query.bind(col++, (int)aevt->GetFlag1());
			query.bind(col++, (int)aevt->GetFlag2());
			query.bind(col++, aevt->GetPhone());
			query.bind(col++, aevt->GetContent());
			query.exec();

			int id = db->execAndGet("SELECT id FROM event_sendsms ORDER BY id DESC LIMIT 1");
			evt->SetID(id);
			break;
		}
		break;
	}
}

bool GSIOTEvent::LoadDB_event_sendsms()
{
	SQLite::Statement query(*this->db,"select * from event_sendsms order by id desc");
	while(query.executeStep()){
		AutoSendSMSEvent *evt = new AutoSendSMSEvent(NULL);

		int col = 0;
		evt->SetID(query.getColumn(col++).getInt());
		evt->SetDeviceType((IOTDeviceType)query.getColumn(col++).getInt());
		evt->SetDeviceID(query.getColumn(col++).getInt());
		evt->set_level_col_save(query.getColumn(col++).getInt());
		evt->SetEnable(query.getColumn(col++).getInt());
		evt->SetDoInterval(query.getColumn(col++).getInt());

		evt->SetFlag(query.getColumn(col++).getInt());
		evt->SetSendCount(query.getColumn(col++).getInt());
		//jyc20160823
		//evt->SetPhone( (std::string)query.getColumn(col++) );
		//evt->SetSMS( (std::string)query.getColumn(col++) );

		this->m_event.push_back(evt);
	}

	return true;
}

bool GSIOTEvent::LoadDB_event_notice()
{
	SQLite::Statement query(*this->db,"select * from event_notice order by id desc");
	while(query.executeStep()){
		AutoNoticeEvent *evt = new AutoNoticeEvent();

		int col = 0;
		evt->SetID(query.getColumn(col++).getInt());
		evt->SetDeviceType((IOTDeviceType)query.getColumn(col++).getInt());
		evt->SetDeviceID(query.getColumn(col++).getInt());
		evt->set_level_col_save(query.getColumn(col++).getInt());
		evt->SetEnable(query.getColumn(col++).getInt());
		evt->SetDoInterval(query.getColumn(col++).getInt());

		if( !query.isColumnNull(col) ) {
			//evt->SetToJid((std::string)query.getColumn(col));  //jyc20170223 modify
			evt->SetToJid(query.getColumn(col));
		}col++;
		std::string msg_subject = query.getColumn(col++);
		std::string msg_body = query.getColumn(col++); 

		evt->SetSubject(msg_subject);
		evt->SetBody(msg_body);

		this->m_event.push_back(evt);
	}

	return true;
}

bool GSIOTEvent::LoadDB_event_autocontrol()
{
	SQLite::Statement query(*this->db,"select * from event_autocontrol order by id desc");
	while(query.executeStep()){
		AutoControlEvent *evt = new AutoControlEvent();

		int col = 0;
		evt->SetID(query.getColumn(col++).getInt());
		evt->SetDeviceType((IOTDeviceType)query.getColumn(col++).getInt());
		evt->SetDeviceID(query.getColumn(col++).getInt());
		evt->set_level_col_save(query.getColumn(col++).getInt());
		evt->SetEnable(query.getColumn(col++).getInt());
		evt->SetDoInterval(query.getColumn(col++).getInt());

		evt->SetControlDeviceType((IOTDeviceType)query.getColumn(col++).getInt());
		evt->SetControlDeviceId(query.getColumn(col++).getInt());
		evt->SetAddress(query.getColumn(col++).getInt());
		evt->SetValue(query.getColumn(col++)); //jyc20170223 unnote

		this->m_event.push_back(evt);
	}

	return true;
}

bool GSIOTEvent::LoadDB_event_eventthing()
{
	SQLite::Statement query(*this->db,"select * from event_eventthing order by id desc");
	while(query.executeStep()){
		AutoEventthing *evt = new AutoEventthing();

		int col = 0;
		evt->SetID(query.getColumn(col++).getInt());
		evt->SetDeviceType((IOTDeviceType)query.getColumn(col++).getInt());
		evt->SetDeviceID(query.getColumn(col++).getInt());
		evt->set_level_col_save(query.getColumn(col++).getInt());
		evt->SetEnable(query.getColumn(col++).getInt());
		evt->SetDoInterval(query.getColumn(col++).getInt());

		evt->SetControlDeviceType((IOTDeviceType)query.getColumn(col++).getInt());
		evt->SetControlDeviceId(query.getColumn(col++).getInt());
		evt->SetRunState(query.getColumn(col++).getInt());

		this->m_event.push_back(evt);
	}

	return true;
}

bool GSIOTEvent::LoadDB_event_call()
{
	SQLite::Statement query( *this->db,"select * from event_call order by id desc" );
	while( query.executeStep() )
	{
		AutoCallEvent *evt = new AutoCallEvent(NULL);

		int col = 0;
		evt->SetID(query.getColumn(col++).getInt());
		evt->SetDeviceType((IOTDeviceType)query.getColumn(col++).getInt());
		evt->SetDeviceID(query.getColumn(col++).getInt());
		evt->set_level_col_save(query.getColumn(col++).getInt());
		evt->SetEnable(query.getColumn(col++).getInt());
		evt->SetDoInterval(query.getColumn(col++).getInt());

		evt->Set_try_count(query.getColumn(col++).getInt());
		evt->Set_play_count(query.getColumn(col++).getInt());
		evt->SetFlag1(query.getColumn(col++).getInt());
		evt->SetFlag2(query.getColumn(col++).getInt());
		//jyc20160823
		//evt->SetPhone( (std::string)query.getColumn(col++) );
		//evt->SetContent( (std::string)query.getColumn(col++) );

		this->m_event.push_back(evt);
	}

	return true;
}
