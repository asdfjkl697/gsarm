#ifndef CONTROLEVENT_H_
#define CONTROLEVENT_H_

#include "gloox/tag.h"
#include "ControlBase.h"
#include "EditAttrMgr.h"

using namespace gloox;

class GSIOTConfig;

class ControlEvent : public EditAttrMgr
{
protected:
	uint32_t m_id;
	IOTDeviceType m_device_type;
	uint32_t m_device_id;
	uint32_t m_level;
	bool m_isForce;
	uint32_t m_enable;
	bool m_istest;

	uint32_t m_DoInterval; // 执行间隔，单位毫秒
	uint32_t m_lastTimeTick_Do; // 最近一次的触发执行时间

	bool doEditAttrFromAttrMgr_EvtBase( const EditAttrMgr &attrMgr, defCfgOprt_ oprt );

public:
	//ControlEvent(void):m_id(0),m_device_id(0),m_level(0),m_enable(0) {};
	ControlEvent(const Tag* tag=0):m_id(0),m_device_type(IOT_DEVICE_Unknown),m_device_id(0),m_level(defEventLevel_Default),m_enable(1),m_istest(false),m_isForce(false),
		m_DoInterval(defDoInterval_UseSysDefault), m_lastTimeTick_Do(0) {};
	
	virtual bool IsCanDo( const GSIOTConfig *pcfg, uint32_t &outDoInterval ) const;
	void SetDo();

	bool isForce() const
	{
		return m_isForce;
	}
	void SetForce( bool Force )
	{
		m_isForce = Force;
	}

	void SetID(uint32_t id)
	{
		m_id = id;
	}
	void SetDeviceType(IOTDeviceType device_type)
	{
		m_device_type = device_type;
	}
	void SetDeviceID(uint32_t id)
	{
		m_device_id = id;
	}
	void SetLevel(uint32_t level)
	{
		m_level = level;

		if( m_level>90 )
		{
			m_level = 90;
		}
	}	
	void SetEnable(uint32_t enable)
	{
		m_enable = enable;
	}	
	
	void SetTest( bool istest = true )
	{
		m_istest = istest;;
	}

	void SetDoInterval(uint32_t DoInterval)
	{
		m_DoInterval = DoInterval;
	}	

	uint32_t GetDoInterval() const
	{
		return m_DoInterval;
	}

	uint32_t GetID() const
	{
		return m_id;
	}
	IOTDeviceType GetDeviceType() const
	{
		return this->m_device_type;
	}
	uint32_t GetDeviceID() const
	{
		return m_device_id;
	}
	uint32_t GetLevel() const
	{
		return m_level;
	}
	uint32_t GetEnable() const
	{
		return this->m_enable;
	}

	bool IsTest() const
	{
		return m_istest;
	}

	void set_level_col_save( int level_col_save )
	{
		m_level = level_col_save % 10000;
		m_isForce = (level_col_save / 10000) ? true:false;
	}

	int get_level_col_save() const
	{
		return ( (m_isForce?10000:0) + m_level );
	}

	virtual bool EventParamIsComplete() = 0;

	virtual EventType GetType() const =0;
	virtual Tag* tag(const struTagParam &TagParam) const = 0;
	void untagBase( const Tag *t );
	void tagBase( Tag *t, const struTagParam &TagParam ) const;
	virtual ControlEvent* clone() const = 0;
};

#endif