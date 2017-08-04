#ifndef DEVICEADDRESS_H_
#define DEVICEADDRESS_H_

#include "typedef.h"
#include "gloox/tag.h"
#include "ControlBase.h"
#include "gloox/mutexguard.h"
#include "EditAttrMgr.h"
#include "GSIOTObjBase.h"
#include <bitset>

#include <assert.h>
#define _ULonglong unsigned long long

using namespace gloox;

// DeviceAddress��������
class DeviceAddressAttr
{
public:
	DeviceAddressAttr( void )
		: m_Attribute( 1 )
	{};

	~DeviceAddressAttr( void ){};

	// ������Чλλ��
	// ע��˳�������ô洢�йأ��汾ȷ����ֻ��������ӣ�����������ԭ��˳��
	enum defAttr
	{
		defAttr_Min_ = 0,
		defAttr_Enable = 0,				// ���ñ�־
		defAttr_IsReSwitch,				// ��̬��ת�л�ʽ����
		defAttr_IsAutoBackSwitch,		// �Է����Իظ�����
		defAttr_Max_
	};

	// ���Բ���
	bool get_AdvAttr( const defAttr attr ) const
	{
		if( attr<0 || (size_t)attr>=m_Attribute.size() )
		{
			assert( false );
			return false;
		}

		return m_Attribute.test( attr );
	}

	bool set_AdvAttr( const defAttr attr, const bool val )
	{
		if( attr<0 || (size_t)attr>=m_Attribute.size() )
		{
			assert( false );
			return false;
		}

		m_Attribute.set( attr, val );
		return true;
	}

	uint32_t get_AdvAttr_uintfull() const
	{
		return m_Attribute.to_ulong();
	}

	bool set_AdvAttr_uintfull( const uint32_t fullval )
	{
		std::bitset<defAttr_Max_> attrfv( (_ULonglong)fullval );
		m_Attribute = attrfv;
		return true;
	}

	bool hasMoreCmd() const
	{
		return (get_AdvAttr_uintfull() ? true:false);
	}

private:
	std::bitset<defAttr_Max_> m_Attribute;
};

class DeviceAddress : public GSIOTObjBase, public EditAttrMgr//:public ControlBase
{
private:
	uint32_t m_address;
	std::string m_name;
	IOTDeviceType m_type;
	IOTDeviceReadType m_readType; 
	DataType m_dataType;

	std::string m_curValue; // ����ֱ�Ӵ��ڡ�С�ڡ����ڱȽ�
	std::string m_curMaxValue;
	std::string m_curMinValue;
	time_t m_timecurValue;
	time_t m_timecurMaxValue;
	time_t m_timecurMinValue;

	std::string m_defaultValue;
	std::string m_minValue;
	std::string m_maxValue;
	//uint32_t m_enable;

	uint8_t *m_dataObject;
	uint32_t m_dataObjLen; 
	
	gloox::util::Mutex *m_pmutex_addr;
	uint32_t m_lastSampTime;
	uint32_t m_lastUpdate;
	int m_data_abnormal_count;	// �쳣����
	int m_data_MultiReadCount;  // �����ȡ����

	// ���һ�δ洢ֵ
	time_t m_lastSaveTime;
	std::string m_lastSaveValue;

	// ���ڻ����
	uint32_t m_ValueWindow_StartTs;
	std::string m_ValueWindow_MaxValue;
	
public:
	DeviceAddress(uint32_t address, std::string name, IOTDeviceType type, DataType dataType, IOTDeviceReadType readType, 
		std::string curValue="0", std::string maxValue="0", std::string minValue="0", std::string defaultValue="0",
		uint8_t *dataObject=NULL, uint32_t dataObjLen=0 );
	DeviceAddress(uint32_t address);
	DeviceAddress(const Tag* tag=0);
	~DeviceAddress(void);

	bool doEditAttrFromAttrMgr( const EditAttrMgr &attrMgr, defCfgOprt_ oprt = defCfgOprt_Modify );

	void InitNewMutex();
	void ResetDataAnalyse();

	void SetName( const std::string& name )
	{
		this->m_name = name;
	}

	const std::string GetName() const
	{
		return this->m_name;
	}

	IOTDeviceType GetType() const
	{
		return m_type;
	}

	virtual int GetId() const
	{
		return GetAddress();
	}

	uint32_t GetAddress() const
	{
		return this->m_address;
	}

	std::string get_str_id() const
	{
		char buf[32] = {0};
		snprintf( buf, sizeof(buf), "(%d,%d)", m_type, m_address );

		return std::string(buf);
	}

	void SetAddress( uint32_t address )
	{
		this->m_address = address;
	}
	IOTDeviceReadType GetReadType() const
	{
		return this->m_readType;
	}
	DataType GetDataType()
	{
		return this->m_dataType;
	}

	bool SetMultiReadCount( int MultiReadCount );
	int GetMultiReadCount();
	int PopMultiReadCount();

	void NowSampTick();

	std::string GetCurValue( bool *isOld=NULL, uint32_t *noUpdateTime=NULL, const uint32_t oldtimeSpan=1200, bool *isLowSampTime=NULL, const bool curisTimePoint=false,
		time_t *timecurValue=NULL, time_t *timecurMaxValue=NULL, time_t *timecurMinValue=NULL );

	std::string GetCurMaxValue( time_t *timecurMaxValue=NULL );
	std::string GetCurMinValue( time_t *timecurMinValue=NULL );

	const std::string GetDefualtValue()const
	{
		return this->m_defaultValue;
	}
	const std::string GetMinValue()const
	{
		return this->m_minValue;
	}
	const std::string GetMaxValue()const
	{
		return this->m_maxValue;
	}
	void SetDeviceType(IOTDeviceType type)
	{
		this->m_type = type;
	}
	void SetReadType(IOTDeviceReadType type)
	{
		this->m_readType = type;
	}
	void SetDataType(DataType dataType)
	{
		this->m_dataType = dataType;
	}

	void ResetUpdateState()
	{
		m_lastUpdate = 0;
	}

	bool SetCurValue( const std::string& newValue, const time_t newValTime=g_GetUTCTime(), const bool annlyse=false, std::string *strlog=NULL );

	bool SetCurValue(const uint16_t val, const time_t newValTime=g_GetUTCTime(), const bool annlyse=false, std::string *strlog=NULL )
	{
		char buffer[32] = {0};
		snprintf(buffer, sizeof(buffer), "%d", val); 

		return this->SetCurValue( std::string(buffer), newValTime, annlyse, strlog );
	}
	bool SetCurValue(const float val, const time_t newValTime=g_GetUTCTime(), const bool annlyse=false, std::string *strlog=NULL )
	{
		char buffer[32] = {0};
		snprintf(buffer, sizeof(buffer), "%.1f", val); 

		return this->SetCurValue( std::string(buffer), newValTime, annlyse, strlog );
	}
	//jyc20170422 add
	bool SetCurValue_3f(const float val, const time_t newValTime=g_GetUTCTime(), const bool annlyse=false, std::string *strlog=NULL )
	{
		char buffer[32] = {0};
		snprintf(buffer, sizeof(buffer), "%.3f", val);
		return this->SetCurValue( std::string(buffer), newValTime, annlyse, strlog );
	}

	void DataAnalyse( const std::string& newValue, const time_t newValTime, bool *doSave, time_t *SaveTime, std::string *SaveValue, defDataFlag_* dataflag, std::string *strlog );

	void SetDefualtValue(const std::string& val)
	{
		this->m_defaultValue = val;
	}
	void SetMinValue(const std::string& val)
	{
		this->m_minValue = val;
	}
	void SetMaxValue(const std::string& val)
	{
		this->m_maxValue = val;
	}

	// ���һ�δ洢ֵ
	void SetLastSave( const time_t lastSaveTime, const std::string lastSaveValue );
	time_t GetLastSaveTime() const;
	std::string GetLastSaveValue() const;

	void SetObjectData(uint8_t *data,uint32_t len);
	uint8_t *GetObjectData(uint8_t *data);
	uint8_t *GetCurToByte(uint8_t *data);
	uint8_t *GetObjectDataPtr()
	{
		return this->m_dataObject;
	}
	uint32_t GetObjectDataLen()
	{
		return this->m_dataObjLen;
	}

	Tag* tag(const struTagParam &TagParam);

	virtual DeviceAddress* clone() const;

	virtual const std::string GetObjName() const
	{
		return m_name;
	}

	virtual GSIOTObjType GetObjType() const
	{
		return GSIOTObjType_DeviceAddress;
	}

	virtual GSIOTObjBase* cloneObj() const
	{
		return clone();
	}
	
	uint32_t GetEnable() const 
	{
		return (uint32_t)m_Attr.get_AdvAttr( DeviceAddressAttr::defAttr_Enable );
		//return m_enable;
	}

	void SetEnable( uint32_t enable )
	{
		m_Attr.set_AdvAttr( DeviceAddressAttr::defAttr_Enable, enable?true:false );
		//m_enable = enable;
	}

	uint32_t get_save_col_val_enable() const
	{
		return m_Attr.get_AdvAttr_uintfull();
	}

	void set_save_col_val_enable( uint32_t col_val )
	{
		m_Attr.set_AdvAttr_uintfull( col_val );
	}

	DeviceAddressAttr& GetAttrObj()
	{
		return m_Attr;
	}

protected:
	DeviceAddressAttr m_Attr;

private:
	bool ValueWindow( const std::string& newValue, const time_t newValTime, std::string& WindowFinishValue, uint32_t WindowTime=60, uint32_t WindowTimeMin=30 );

};

#endif
