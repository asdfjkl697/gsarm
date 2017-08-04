#ifndef GSIOTDEVICE_H_
#define GSIOTDEVICE_H_

#include "../typedef.h"
#include "gloox/tag.h"
#include "../ControlBase.h"
#include "../DeviceAddress.h"
#include "../RemoteButtonClass.h"
#include "../EditAttrMgr.h"
#include "../GSObjLocker.h"

using namespace gloox;

struct GSIOTDeviceKey 
{
	IOTDeviceType m_type;
	int m_id;

	GSIOTDeviceKey( IOTDeviceType type, int id )
		: m_type(type), m_id(id)
	{
	}

	GSIOTDeviceKey()
		: m_type(IOT_DEVICE_Unknown), m_id(0)
	{
	}
};
bool operator< ( const GSIOTDeviceKey &key1, const GSIOTDeviceKey &key2 );
typedef std::vector<GSIOTDeviceKey> defvecDevKey;

class GSIOTDevice : public EditAttrMgr
{
public:
	GSObjLocker m_ObjLocker;

private:
	int m_id;
	std::string m_name;
	IOTDeviceType m_type;
	std::string m_ver;
	ControlBase *m_control;
	
	uint32_t m_enable;

public:
	GSIOTDevice(int id,const std::string& name,
	IOTDeviceType type,
	const std::string& ver,
	const std::string& serialno,
	const std::string& factoryno,
	ControlBase *control);
	GSIOTDevice( const Tag* tag = 0 );
	~GSIOTDevice(void);
	
	static bool IsCanRelation( IOTDeviceType type );
	static bool IsHasSignalDevice( IOTDeviceType type );
	static bool IsSupportAlarm( const GSIOTDevice *device );

	bool doEditAttrFromAttrMgr( const EditAttrMgr &attrMgr, defCfgOprt_ oprt = defCfgOprt_Modify );
	bool doEditAttrFromAttrMgr_All();

	static IOTDeviceReadType getStaticReadType( const IOTDeviceType type );
	IOTDeviceReadType getReadType() const;

	std::string GetStrAlmBody( const bool isAlarm, const struGSTime &dt, const std::string alarmstr=c_NullStr, const std::string suffix=c_NullStr ) const;
	std::string GetStrAlmSubject( const bool isAlarm ) const;

	void SetLinkID( const defLinkID LinkID, bool log=true )
	{
		if( m_control )
		{
			if( log ) { printf( "SetLinkID dev(type=%d,id=%d), LinkID(old=%d,new=%d)\r\n", m_type, m_id, m_control->LinkID, LinkID ); }
			m_control->LinkID = LinkID;
		}
	}

	defLinkID GetLinkID() const
	{
		if( m_control ) { return m_control->GetLinkID(); }

		return defLinkID_Local;
	}

	int getId() const {
	    return m_id;
	}
	
	std::string get_str_id() const
	{
		char buf[32] = {0};
		snprintf( buf, sizeof(buf), "(%d,%d)", m_type, m_id );

		return std::string(buf);
	}

	void setId( int id ){
	    m_id = id;
	}

	const std::string& getName()const {
		return this->m_name;
	}

	IOTDeviceType getType() const {
		return this->m_type;
	}

	IOTDeviceType getExType() const
	{
		return ( m_control ? m_control->GetExType():getType() );
	}

	void setType( IOTDeviceType type ){
		m_type = type;
	}
	
	void setVer( const std::string &ver )
	{
		m_ver = ver;
	}

	const std::string&  getVer() const {
		return this->m_ver;
	}
	
	ControlBase *getControl() const{
		return this->m_control;
	}
	void setControl(ControlBase *ctl){
		this->m_control = ctl;
	}
	void setName(const std::string& name)
	{
		this->m_name = name;
		if( this->m_control )
			this->m_control->SetName( name );
	}

	uint32_t GetEnable() const 
	{
		return m_enable;
	}

	void SetEnable( uint32_t enable )
	{
		m_enable = enable;
	}
	
	// 
	defUseable get_all_useable_state() const;
	
	// 
	defAlarmState GetCurAlarmState() const;
	
	std::string GetPrePicChangeCode() const;

    Tag* tag(const struTagParam &TagParam) const;

	virtual GSIOTDevice* clone( bool CreateLock=true ) const;

	bool ResetUpdateState( const uint32_t address );
	bool SetCurValue( DeviceAddress *addr );

	bool AddAddressObj( const uint32_t address, const std::string &address_name, const std::string &value );

	bool AddAddress( DeviceAddress *addr );
	bool AddButton( RemoteButton *btn );
	
	// �ж��Ƿ���������Ҫ����
	bool hasChild() const;

	bool isDevSelfAndIncludeAddr( const IOTDeviceType deviceType, const uint32_t deviceId, const uint32_t address ) const;
};

typedef std::map<GSIOTDeviceKey,GSIOTDevice*> defmapDev; //<key,p>

#endif

