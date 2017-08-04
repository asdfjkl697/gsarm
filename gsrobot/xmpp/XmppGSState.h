#pragma once
#include "../typedef.h"
#include "gloox/stanzaextension.h"
#include "gloox/tag.h"
#include "../ControlEvent.h"
#include "../gsiot/GSIOTDevice.h"

class XmppGSState :
	public StanzaExtension
{
public:
	enum defStateCmd
	{
		defStateCmd_Unknown	= 0,
		defStateCmd_events	= 1,
		defStateCmd_alarmguard = 2,
		defStateCmd_exitlearnmod	= 3,
		defStateCmd_reboot	= 901,
	};

	struct struAGTime
	{
		int allday; // ��־
		std::vector<int> vecagTime;

		struAGTime()
			: allday(true)
		{
		}
	};

public:
	XmppGSState( const struTagParam &TagParam, uint32_t state_board, uint32_t state_mobile, uint32_t state_events, GSAGCurState_ AGCurState, uint32_t state_starttime, const std::list<GSIOTDevice*>& deviceList );
	XmppGSState( const Tag* tag );
	~XmppGSState(void);

	// reimplemented from StanzaExtension
	virtual const std::string& filterString() const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* newInstance( const Tag* tag ) const
	{
		return new XmppGSState( tag );
	}

	void PrintTag( const Tag* tag, const Stanza *stanza ) const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* clone() const
	{
		XmppGSState *pnew = new XmppGSState(NULL);
		*pnew = *this;
		return pnew;
	}

	// reimplemented from StanzaExt
	virtual Tag* tag() const;

	uint32_t get_state_events() const
	{
		return m_state_events;
	}

	defStateCmd get_cmd() const
	{
		return m_cmd;
	}

	const std::map<int,struAGTime>& get_mapagTime() const
	{
		return m_mapAGTime;
	}

private:
	struTagParam m_TagParam;
	defStateCmd m_cmd;
	
	uint32_t m_state_board;		//���ذ�״̬
	uint32_t m_state_mobile;	//�ƶ��绰ģ��
	uint32_t m_state_events;	//�豸�¼�����Ӧ����������
	GSAGCurState_ m_AGCurState;	//�ۺϷ�����Ĳ���ʵ��״̬
	uint32_t m_state_starttime;	//��ʼ����ʱ��

	std::map<int,struAGTime> m_mapAGTime;

	std::list<GSIOTDevice*> m_deviceList;
};

