#ifndef GSIOTCONFIG_H_
#define GSIOTCONFIG_H_

#include "common.h"
#include "SQLiteHelper.h" //20160603
#include "GSIOTUserMgr.h"
#include "gloox/mutexguard.h"

using namespace gloox;

// ���� ���豸��Ϣ
struct struRelationMainDev
{
	IOTDeviceType device_type;
	int device_id;

	struRelationMainDev( IOTDeviceType in_device_type, int in_device_id )
		:device_type(in_device_type), device_id(in_device_id)
	{

	}
};

// ���� ���豸��Ϣ
struct struRelationChild
{
	IOTDeviceType child_dev_type;	// �����豸����
	int child_dev_id;				// �����豸ID
	int child_dev_subid;			// �����Ӷ���ID  0 ��ʾ�����Ӷ���

	struRelationChild( IOTDeviceType in_child_dev_type, int in_child_dev_id, int in_child_dev_subid=0 )
		:child_dev_type(in_child_dev_type), child_dev_id(in_child_dev_id), child_dev_subid(in_child_dev_subid)
	{

	}
};
bool operator< ( const struRelationMainDev &key1, const struRelationMainDev &key2 );
typedef std::list<struRelationChild> deflstRelationChild;
typedef std::map<struRelationMainDev,deflstRelationChild> defmapRelation;


class GSIOTConfig: public SQLiteHelper, public INotifyHandler_UserMgr
{
public:
	GSIOTConfig(void);
	~GSIOTConfig(void);
	
	bool doDBUpdate();
	void LoadDB_cfg();
	bool PreInit( const std::string &RunParam );
	
	GSIOTUserMgr m_UserMgr;

	std::string GetOwnerKeyJid() const
	{
		return GSIOTUser::GetKeyJid_spec(this->m_noticejid);
	}
	bool isOwner( const std::string &UserJID ); // �豸�����ʺţ�ӵ����̨�豸���ʺ�
	bool isOwnerForKeyJid( const std::string &OwnerKeyJid, const std::string &UserKeyJid ) // �豸�����ʺţ�ӵ����̨�豸���ʺ�
	{
		return ( !OwnerKeyJid.empty() && OwnerKeyJid == UserKeyJid );
	}
	void FixOwnerAuth( GSIOTUser *pUser );
	void RefreshAuthOverview( GSIOTUser *pUser );

	virtual void OnNotify_UserMgr( defNotify_ notify, GSIOTUser *pUser );
	void AddNotifyHandler( INotifyHandler_UserMgr *handler );
	void RemoveNotifyHandler( INotifyHandler_UserMgr *handler );

	const std::string& getSerialNumber() const{
		return this->m_serialnumber;
	}

	void setSerialNumber(const std::string& serialNumber)
	{
		this->m_serialnumber = serialNumber;
	}
	int getSerialPort()
	{
		return this->m_serialport;
	}
	void setSerialPort(int serialPort)
	{
		this->m_serialport = serialPort;
	}

	const std::string& getJid() const{
		return this->m_jid;
	}
	
	void setJid(const std::string& jid)
	{
		this->m_jid = jid;
	}
	const std::string& getPassword() const{
		return this->m_password;
	}

	void setPassword(const std::string& password)
	{
		this->m_password = password;
	}
	const std::string& getSmtpServer() const{
		return this->m_smtpserver;
	}

	void setSmtpServer(const std::string& smtpserver)
	{
		this->m_smtpserver = smtpserver;
	}
	
	const std::string& getSmtpUser() const{
		return this->m_smtpuser;
	}

	void setSmtpUser(const std::string& smtpuser)
	{
		this->m_smtpuser = smtpuser;
	}
	const std::string& getSmtpPassword() const{
		return this->m_smtppassword;
	}
	void setSmtpPassword(const std::string& password)
	{
		this->m_smtppassword = password;
	}

	void SetNoticeJid( const std::string& noticejid )
	{
		if( noticejid != this->m_noticejid )
		{
			this->m_noticejid = noticejid;
		}
	}

	const std::string GetNoticeJid()
	{
		return this->m_noticejid;
	}

	void SetPhone( const std::string& phone )
	{
		this->m_phone = phone;
	}

	const std::string GetPhone()
	{
		return this->m_phone;
	}

	void SetDoInterval_ForSMS(uint32_t DoInterval)
	{
		m_DoInterval_ForSMS = DoInterval;
	}	

	uint32_t GetDoInterval_ForSMS() const
	{
		return m_DoInterval_ForSMS;
	}

	void SaveToFile();

	bool SetRelation( IOTDeviceType device_type, int device_id, const deflstRelationChild &ChildList );
	bool GetRelation( IOTDeviceType device_type, int device_id, deflstRelationChild &ChildList );
	bool ClearAllRelation_SpecDev( IOTDeviceType device_type, int device_id );
	bool ClearAllRelation_SpecType( IOTDeviceType device_type );
	bool ClearAllRelation_All();
private:
	bool LoadRelation();
	bool SaveRelation( IOTDeviceType device_type, int device_id, const deflstRelationChild &ChildList );
	bool IsInDB_Relation( IOTDeviceType device_type, int device_id, IOTDeviceType child_dev_type, int child_dev_id, int child_dev_subid );

public:
	bool VObj_Get( int id, stru_vobj_config &cfgobj ) const;
	defGSReturn VObj_CheckExist( const stru_vobj_config &cfgobj, std::string *strerr );
	defGSReturn VObj_Add( stru_vobj_config &cfgobj, std::string *strerr );
	defGSReturn VObj_Delete( const IOTDeviceType vobj_type, const int id );
	defGSReturn VObj_Modify( const stru_vobj_config &cfgobj, std::string *strerr );
	const defmapVObjConfig& VObj_GetList() const { return m_mapVObj; }

private:
	bool LoadDB_vobj_config();
	int db_Insert_vobj_config( const stru_vobj_config &cfg );
	bool db_Update_vobj_config( const stru_vobj_config &cfg );
	bool db_Delete_vobj_config( const IOTDeviceType vobj_type, const int id );

private:
	std::string m_serialnumber;
	std::string m_jid;
	std::string m_password;
	int m_serialport;
	std::string m_smtpserver;
	std::string m_smtpuser;
	std::string m_smtppassword;
	std::string m_noticejid;
	std::string m_phone;
	uint32_t m_DoInterval_ForSMS; // ִ�м������λ��

private:
	defmapRelation m_mapRelation; // �豸����
	defmapVObjConfig m_mapVObj;		// �������

	std::list<INotifyHandler_UserMgr*> m_NotifyHandlerList;

private:

};

#endif
