#pragma once

#include "../typedef.h"
#include "gloox/stanzaextension.h"
#include "gloox/tag.h"
#include <bitset>

enum defAuthOverview_Auth
{
	defAuthOverview_Auth_Null	= 0,	// ��Ȩ��
	defAuthOverview_Auth_RO		= 1,	// ����ֻ��
	defAuthOverview_Auth_WO		= 2,	// ����ֻд
	defAuthOverview_Auth_All	= 3,	// ӵ������Ȩ��
	defAuthOverview_Auth_PartRO	= 4,	// ����Ȩ�ޣ���дȨ��(ֻ��)
	defAuthOverview_Auth_PartW	= 5,	// ����Ȩ�ޣ��в�����дȨ�ޣ�����ֻд����д��
};

enum defAuthOverview_UserType
{
	defAuthOverview_UserType_Guest		= 0,	// ����=guest
	defAuthOverview_UserType_User		= 1,	// �û���һ�������û�
	defAuthOverview_UserType_AdminType	= 2,	// ����Ա������Ա����û�
	defAuthOverview_UserType_SysAdmin	= 3,	// ϵͳ����Ա=Admin
	defAuthOverview_UserType_Owner		= 4,	// �豸�����ߣ��豸��������Ա=�豸�������ʺ�
};

#define defGuestName "����"

// �û�����
#define defUserLevel_Default		5	// Ĭ�ϼ���
#define defUserLevel_Guest			1	// ����=guest
#define defUserLevel_MaxCfgLevel	99	// ���������õ�����û�����
#define defUserLevel_Owner			100	// �豸�����ߣ��豸��������Ա=�豸�������ʺ�
#define defUserLevel_Local			101	// ϵͳ����Ա=Admin
#define defUserLevel_SysAdmin		109	// ���ع������


using namespace gloox;

// Ȩ����Ϣ�ṹ
struct struAuth
{
	struAuth()
		:device_type(IOT_DEVICE_Unknown),device_id(0),authority(defUserAuth_Null)
	{
	}

	struAuth( IOTDeviceType type, int id, defUserAuth auth )
		:device_type(type),device_id(id),authority(auth)
	{
	}

	IOTDeviceType device_type;
	int device_id;
	defUserAuth authority;

	std::string Print( const char *info, bool doPrint ) const;
};

struct struAuth_key
{
	struAuth_key( IOTDeviceType type, int id )
		:device_type(type),device_id(id)
	{
	}

	struAuth_key( const struAuth &auth )
		:device_type(auth.device_type),device_id(auth.device_id)
	{
	}

	IOTDeviceType device_type;
	int device_id;
};

typedef std::map<struAuth_key,struAuth> defmapAuth;	// <Ȩ��key,Ȩ����Ϣ>

#define defUserAuth_Default	defUserAuth_Null // ��������ʱĬ��Ȩ��

// ������Чλλ��
// ��m_UserFlag�е�λ��
// ע��˳�������ô洢�йأ��汾ȷ����ֻ��������ӣ�����������ԭ��˳��
enum defUserFlag
{
	defUserFlag_Min_			= 0,
	defUserFlag_NoticeGroup		= 0,		// �Ƿ����Ĭ��֪ͨ��
	defUserFlag_Max_
};

class CUserFlag
{
public:
	CUserFlag(void)
		: m_UserFlag(0)
	{};

	~CUserFlag(void){};

	// �߼����Բ��� m_UserFlag
	static bool isShow_UserFlag( defUserFlag flag );
	static const char* get_UserFlag_Name( defUserFlag flag );

	bool get_UserFlag( defUserFlag flag ) const;
	bool set_UserFlag( defUserFlag flag, bool val );
	uint32_t get_UserFlag_uintfull() const;
	bool set_UserFlag_uintfull( uint32_t fullval );

private:
	std::bitset<defUserFlag_Max_> m_UserFlag;
};

/*
===========================
�û���Ϣ����Ȩ����Ϣ��װ��

===========================
*/
class GSIOTUser	: public CUserFlag
{
public:

	// ������Чλλ��
	// ��m_validAttribute�е�λ��
	// ע��˳�������ô洢�޹أ�˳��ɸ��ÿ���汾���е���
	enum defAttr
	{
		defAttr_all = -1,
		defAttr_name = 0,
		defAttr_enable,
		defAttr_NoticeGroup,
		defAttr_UserLevel,
		defAttr_Max
	};

public:
	GSIOTUser();
	GSIOTUser( const Tag* tag );
	~GSIOTUser(void);

	static std::string GetKeyJid_spec( const std::string &jid );
	static std::string getstr_UserLevel( const int UserLevel );
	static void FixJid_spec( std::string &jid );
	static bool isSame( const std::string &jidA, const std::string &jidB );
	static const struAuth* Get_Auth_spec( const defmapAuth &mapAuth, const IOTDeviceType type, const int id );
	static bool mapAuthCampareMyOnOther( const defmapAuth &mapAuth_my, const defmapAuth &mapAuth_other, const bool cmpDevice, const bool cmpModule );

	GSIOTUser* clone() const;
	void UntagChild( const Tag* tag );
	Tag* tag(const struTagParam &TagParam) const;
	Tag* tagChild( const struAuth &Auth ) const;

	bool IsValidAttribute( const GSIOTUser::defAttr attr ) const;
	void SetValidAttribute( const GSIOTUser::defAttr attr, bool valid=true );

public:
	// �����ж�
	static bool JudgeAuth( const defUserAuth curAuth, const defUserAuth needAuth );
	static bool IsMoudle( const IOTDeviceType type );
	static bool IsAllAuthConfig( const IOTDeviceType type, const int id );

	static std::string ConvertTypeToString( const IOTDeviceType type );
	static IOTDeviceType ConvertStringToType( const std::string &strType );
	static std::string ConvertTypeToShowName( const IOTDeviceType type );
	static std::string ConvertAuthToShowName( const defUserAuth auth );

	static int FixUserLevel( int UserLevel )
	{
		if( UserLevel<0 )
			UserLevel = 0;
		else if( UserLevel>defUserLevel_MaxCfgLevel )
			UserLevel = defUserLevel_MaxCfgLevel;

		return UserLevel;
	}
	
	uint32_t get_save_col_val_enable() const
	{
		return ( FixUserLevel( m_UserLevel )*10 + m_enable );
	}

	void set_save_col_val_enable( uint32_t col_val )
	{
		m_enable = col_val%2;

		this->SetUserLevel( (col_val-m_enable)/10 );
	}

	void SetID( int ID )
	{
		m_id = ID;
	}

	int GetID() const
	{
		return m_id;
	}

	void SetEnable( int enable )
	{
		m_enable = enable;
	}

	int GetEnable() const
	{
		return m_enable;
	}

	void SetUserLevel( int UserLevel )
	{
		m_UserLevel = UserLevel;

		m_UserLevel = FixUserLevel( m_UserLevel );
	}

	int GetUserLevel() const
	{
		return m_UserLevel;
	}

	int GetUserLevel_use() const
	{
		switch( m_AuthOverview_UserType )
		{
		case defAuthOverview_UserType_Guest:
			return defUserLevel_Guest;

		case defAuthOverview_UserType_SysAdmin:
			return defUserLevel_SysAdmin;

		case defAuthOverview_UserType_Owner:
			return defUserLevel_Owner;

		default:
			break;
		}

		if( 0==m_UserLevel ) // Ĭ���û�����
		{
			return defUserLevel_Default;
		}

		return m_UserLevel;
	}

	void SetJid( const std::string &jid )
	{
		m_jid = jid;
		this->FixJid();
	}

	std::string GetJid() const
	{
		return m_jid;
	}
	
	void FixJid();
	void FixName();
	std::string GetKeyJid() const;
	bool isMe( const std::string &in_jid ) const;

	void SetName( const std::string &name )
	{
		m_name = name;
		this->FixName();
	}

	std::string GetName() const
	{
		return m_name;
	}

	void SetResult( defGSReturn result )
	{
		m_result = result;
	}

	defGSReturn GetResult() const
	{
		return m_result;
	}

	const defmapAuth& GetList_Auth() const
	{
		return this->m_mapAuth;
	}

	bool IsNullAuth() const;
	bool IsSameAuth( const GSIOTUser &other ) const;
	bool IsNearAuth( const GSIOTUser &other ) const;

	bool IsAll_Auth( defUserAuth needauth=defUserAuth_RW ) const;
	void SetAll_Auth( defUserAuth auth=defUserAuth_RW, bool blLog=true );
	bool IsAllAuthCfg() const;

	bool IsAll_AuthDevice( defUserAuth needauth=defUserAuth_RW ) const;
	bool IsAll_AuthModule( defUserAuth needauth=defUserAuth_RW ) const;
	defUserAuth GetAll_AuthDevice() const;
	defUserAuth GetAll_AuthModule() const;
	void SetAll_AuthDevice( defUserAuth auth, bool blLog=true );
	void SetAll_AuthModule( defUserAuth auth, bool blLog=true );

	const struAuth* Get_Auth( const IOTDeviceType type, const int id ) const;
	bool CfgChange_Auth( const struAuth &auth, const defCfgOprt_ oprt, const bool FailedIsErr=true, bool blLog=true );
	bool Delete_Auth( const IOTDeviceType type, const int id, const bool FailedIsErr=true );

	void Reset();
	void ResetOnlyAuth( bool allauth=true, bool reset_overview=true );
	void RemoveUnused( bool onlyRemoveNull=false );
	void AddReplace( const GSIOTUser *pUser, bool NoAdd=false );
	void AddReplaceList( const defmapAuth &mapAuth, bool NoAdd=false );
	void InitCfgContent( defUserAuth auth );

	void SetDefaultAuth( int level=0 );
	void SetDefaultAuth_AuthDevice( int level=0 );
	void SetDefaultAuth_AuthModule( int level=0 );

	// Ȩ�޸���
	void Refresh_AuthOverview_Auth();
	static std::string Get_AuthOverview_Auth_str( const defAuthOverview_Auth type );
	static std::string Get_AuthOverview_UserType_str( const defAuthOverview_UserType type );
	std::string Get_AuthOverview_Str() const;
	void Set_AuthOverview_Device( const defAuthOverview_Auth ov_auth )
	{
		m_AuthOverview_Device = ov_auth;
	}
	void Set_AuthOverview_Module( const defAuthOverview_Auth ov_auth )
	{
		m_AuthOverview_Module = ov_auth;
	}
	void Set_AuthOverview_UserType( const defAuthOverview_UserType ov_usertype )
	{
		m_AuthOverview_UserType = ov_usertype;

		switch( m_AuthOverview_UserType )
		{
		case defAuthOverview_UserType_Guest:
		case defAuthOverview_UserType_SysAdmin:
		case defAuthOverview_UserType_Owner:
			{
				this->SetUserLevel( defUserLevel_Default );
			}
			break;

		default:
			break;
		}
	}

	defAuthOverview_Auth Get_AuthOverview_Device() const
	{
		return m_AuthOverview_Device;
	}
	defAuthOverview_Auth Get_AuthOverview_Module() const
	{
		return m_AuthOverview_Module;
	}
	defAuthOverview_UserType Get_AuthOverview_UserType() const
	{
		return m_AuthOverview_UserType;
	}

private:
	defAuthOverview_Auth m_AuthOverview_Device;
	defAuthOverview_Auth m_AuthOverview_Module;
	defAuthOverview_UserType m_AuthOverview_UserType;

private:
	int m_id;			// int Ψһ����ݿ��Զ����
	int m_enable;		// �Ƿ�����
	int m_UserLevel;	// �û�����
	std::string m_jid;	// ��jidΨһ
	std::string m_name;	// �û������ʾ��
	defmapAuth m_mapAuth; // Ȩ���б�

	std::bitset<defAttr_Max> m_validAttribute; // ��Ч���ԣ�����������������Ϣ�������ֶο���û�У�Ҳ�Ͳ���������Ч
	defGSReturn m_result;		// Զ�̲������
};

bool operator< ( const struAuth_key &key1, const struAuth_key &key2 );

typedef std::map<std::string,GSIOTUser*> defmapGSIOTUser; // <�û�jid,�û���Ϣ>
