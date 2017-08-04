#ifndef GSIOTEVENT_H_
#define GSIOTEVENT_H_

#include "../SQLiteHelper.h"
#include "../AutoControlEvent.h"
#include "../AutoNoticeEvent.h"
#include "../AutoSendSMSEvent.h"
#include "../AutoEventthing.h"
#include "../AutoCallEvent.h"

class GSIOTEvent: public SQLiteHelper
{
public:
	GSIOTEvent(void);
	~GSIOTEvent(void);
	
	static std::string GetEventTypeToString(EventType type);
	static void SortEvents_spec( std::list<ControlEvent*> &events );
	
	void SortEvents();

	void AddEvent(ControlEvent *evt);
	void DeleteEvent(ControlEvent *evt);
	bool ModifyEvent(ControlEvent *evt, const ControlEvent *evtsrc);
	bool DeleteDeviceEvent( IOTDeviceType devtype, int id );

	std::list<ControlEvent*>& GetEvents()
	{
		return m_event;
	}

private:
	void SaveToDb(ControlEvent *evt);

	bool LoadDB_event_sendsms();
	bool LoadDB_event_notice();
	bool LoadDB_event_autocontrol();
	bool LoadDB_event_eventthing();
	bool LoadDB_event_call();

private:
	std::list<ControlEvent *> m_event;
};

#endif
