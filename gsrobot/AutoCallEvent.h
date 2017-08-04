#pragma once
//#include "controlevent.h"
#include "ControlEvent.h"


// 自动拨打电话通知事件
class AutoCallEvent :
	public ControlEvent
{
public:
	AutoCallEvent( std::string &Phone, std::string &content, uint32_t try_count=1, uint32_t play_count=1, uint32_t flag1=0, uint32_t flag2=0 );
	AutoCallEvent(const Tag* tag);
	~AutoCallEvent(void);

	bool doEditAttrFromAttrMgr( const EditAttrMgr &attrMgr, defCfgOprt_ oprt = defCfgOprt_Modify );

	EventType GetType() const
	{
		return CALL_Event;
	}

	uint32_t Set_try_count( uint32_t try_count )
	{
		m_try_count = ( try_count>0 && try_count<=9 ) ? try_count : 1;

		return m_try_count;
	}

	uint32_t Set_play_count( uint32_t play_count )
	{
		m_play_count = ( play_count>0 && play_count<=9 ) ? play_count : 1;

		return m_play_count;
	}

	void SetFlag1( uint32_t Flag1 )
	{
		m_flag1 = Flag1;
	}

	void SetFlag2( uint32_t Flag2 )
	{
		m_flag2 = Flag2;
	}
	
	void SetPhone( const std::string &Phone )
	{
		this->m_Phone = Phone;
	}

	void SetContent( const std::string &content  )
	{
		this->m_content = m_content;
	}

	uint32_t Get_try_count() const
	{
		if( m_try_count>0 && m_try_count<=9 ) { return m_try_count; }

		return uint32_t(1);
	}

	uint32_t Get_play_count() const
	{
		if( m_play_count>0 && m_play_count<=9 ) { return m_play_count; }

		return uint32_t(1);
	}

	uint32_t GetFlag1() const
	{
		return m_flag1;
	}

	uint32_t GetFlag2() const
	{
		return m_flag2;
	}

	const std::string& GetPhone() const
	{
		return m_Phone;
	}

	const std::string& GetContent() const
	{
		return m_content;
	}

	bool EventParamIsComplete();

	Tag* tag(const struTagParam &TagParam) const;
	virtual ControlEvent* clone() const{
		return new AutoCallEvent(*this);
	}

private:
	uint32_t m_try_count;	// 试拨打电话次数
	uint32_t m_play_count;	// 语音播放重复次数
	uint32_t m_flag1; // 预留
	uint32_t m_flag2; // 预留
	std::string m_Phone;
	std::string m_content;
};

