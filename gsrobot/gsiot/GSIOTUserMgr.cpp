#include "GSIOTUserMgr.h"
#include "../common.h"
#include "../RunCode.h"
#include "gloox/util.h"

#ifdef _DEBUG
#define macDebugLog_GSIOTUserMgr //LOGMSG
#else
#define macDebugLog_GSIOTUserMgr
#endif


GSIOTUserMgr::GSIOTUserMgr(void)
	: m_GSIOTUser_Admin(NULL), m_NotifyHandler(NULL)
{
}


GSIOTUserMgr::~GSIOTUserMgr(void)
{
	if( m_GSIOTUser_Admin )
	{
		delete m_GSIOTUser_Admin;
		m_GSIOTUser_Admin = NULL;
	}

	ReleaseUserList();
	m_NotifyHandler = NULL;
}

defmapGSIOTUser::const_iterator GSIOTUserMgr::usermapFind( const defmapGSIOTUser &mapUser, const std::string jid )
{
	std::string jidFind = jid;
	g_toLowerCase( jidFind );

	return mapUser.find( jidFind );
}

void GSIOTUserMgr::usermapInsert( defmapGSIOTUser &mapUser, GSIOTUser *pUser )
{
	assert(pUser);

	std::string jidFind = pUser->GetKeyJid();
	
	mapUser.insert( std::make_pair( jidFind, pUser ) );
}

void GSIOTUserMgr::usermapRelease( defmapGSIOTUser &mapUser )
{
	defmapGSIOTUser::iterator it = mapUser.begin();
	defmapGSIOTUser::iterator it2;
	while( it != mapUser.end() )
	{
		it2 = it++;
		delete (*it2).second;
		mapUser.erase( it2 );
	}
}

void GSIOTUserMgr::usermapCopy( defmapGSIOTUser &mapUserDest, const defmapGSIOTUser &mapUserSrc )
{
	for( defmapGSIOTUser::const_iterator it=mapUserSrc.begin(); it!=mapUserSrc.end(); ++it )
	{
		GSIOTUser *pUser = it->second;
		usermapInsert( mapUserDest, pUser->clone() );
	}
}

void GSIOTUserMgr::Init( SQLite::Database *pdb )
{
	db = pdb;

	m_GSIOTUser_Admin = new GSIOTUser();
	m_GSIOTUser_Admin->SetAll_AuthDevice( defUserAuth_Null );
	m_GSIOTUser_Admin->SetAll_AuthModule( defUserAuth_RW );
	LoadDB_User();
}

GSIOTUser* GSIOTUserMgr::check_GetUser( const std::string &jid )
{
	GSIOTUser *pUser = this->GetUser( jid );

	if( pUser )
	{
		if( pUser->GetEnable() )
		{
			return pUser;
		}

		return NULL;
	}

	return this->GetUser( XMPP_GSIOTUser_Guest );
}

defGSReturn GSIOTUserMgr::check_User( const GSIOTUser *pUser )
{
	// �Ƿ�����Ȩ��
	if( IsRUNCODEEnable(defCodeIndex_Debug_DisableAuth) )
	{
		//LOGMSGEX( defLOGNAME, defLOG_WORN, "__TEST__: <Debug_DisableAuth> check_User" );
		return defGSReturn_Success;
	}

	if( !pUser )
	{
		return defGSReturn_NoExist;
	}

	if( !pUser->GetEnable() )
	{
		return defGSReturn_Err;
	}

	if( pUser->IsNullAuth() )
	{
		return defGSReturn_Err;
	}

	return defGSReturn_Success;
}

// ���еط����û�Ȩ����֤�����ô˺��������֤
// ����Ȩ�����ú͵���Ȩ������ ȡȨ�޸ߵ���ΪȨ����֤
defUserAuth GSIOTUserMgr::check_Auth( const GSIOTUser *pUser, const IOTDeviceType type, const int id )
{
	// �Ƿ�����Ȩ��
	if( IsRUNCODEEnable(defCodeIndex_Debug_DisableAuth) )
	{
		//LOGMSGEX( defLOGNAME, defLOG_WORN, "__TEST__: <Debug_DisableAuth> check_Auth" );
		return defUserAuth_RW;
	}

	if( !pUser )
	{
		return defUserAuth_Default;
	}

	if( !pUser->GetEnable() )
	{
		return defUserAuth_Default;
	}

	defUserAuth authAllVal = defUserAuth_Default;
	if( GSIOTUser::IsMoudle( type ) )
	{
		authAllVal = pUser->GetAll_AuthModule();
	}
	else
	{
		authAllVal = pUser->GetAll_AuthDevice();
	}

	//if( defUserAuth_RW == authAllVal )
	if( authAllVal > defUserAuth_Null ) // ֻҪ����������Ȩ�ޣ���������Ȩ��Ϊ����
	{
		return authAllVal;
	}

	// ��ȡ��������Ȩ��
	const struAuth *pAuth = pUser->Get_Auth( type, id );
	if( pAuth )
	{
		return pAuth->authority;
	}

	return authAllVal;
	//return defUserAuth_Default;
}

bool GSIOTUserMgr::IsAdmin( const GSIOTUser *pUser )
{
	std::string jid = pUser->GetKeyJid();

	if( jid == XMPP_GSIOTUser_Admin )
		return true;

	return false;
}

bool GSIOTUserMgr::IsGuest( const GSIOTUser *pUser )
{
	if( !pUser )
		return false;

	std::string jid = pUser->GetKeyJid();

	if( jid == XMPP_GSIOTUser_Guest )
		return true;

	return false;
}

// ��ȡ�û���Ϣָ��
GSIOTUser* GSIOTUserMgr::GetUser( const std::string &jid )
{
	if( jid==XMPP_GSIOTUser_Admin )
	{
		return m_GSIOTUser_Admin;
	}

	defmapGSIOTUser::const_iterator it = GSIOTUserMgr::usermapFind( m_mapUser, jid );
	if( it!=m_mapUser.end() )
	{
		return (*it).second;
	}

	return NULL;
}

// �û����ñ��
defGSReturn GSIOTUserMgr::CfgChange_User( GSIOTUser *const pUser, const defCfgOprt_ oprt, const bool FailedIsErr, bool doTRANSACTION )
{
	if( !pUser )
	{
		return defGSReturn_Err;
	}

	pUser->FixJid();
	pUser->FixName();

	std::string jid = pUser->GetKeyJid();

	if( IsAdmin(pUser) )
	{
		return defGSReturn_Err;
	}

	switch( oprt )
	{
	case defCfgOprt_Add:
	case defCfgOprt_Modify:
	case defCfgOprt_AddModify:
		break;

	case defCfgOprt_Delete:
		{
			return Delete_User( jid, FailedIsErr );
		}

	default:
		assert(false);
		return defGSReturn_Err;
	}

	GSIOTUser *pCurUser = GetUser( jid );
	if( pCurUser )
	{
		// modify or addmodify
		LOGMSG( "CfgChange_User(oprt=%d) isbeing, %s\r\n", oprt, jid.c_str() );

		// �޸�ʱ���������ú������ö��Ѿ�������Ȩ�ޱ�־�����ñ������Ȩ�޼�¼
		bool Pass_mapAuthAdd = false;
		//if( pUser->IsAllAuthCfg() && pCurUser->IsAllAuthCfg() )
		if( pUser->IsAllAuthCfg() )
		{
			Pass_mapAuthAdd = true;
		}

		// update
		if( defCfgOprt_Modify == oprt
			|| defCfgOprt_AddModify == oprt )
		{
			bool doUpdate = false;

			if( pUser->IsValidAttribute(GSIOTUser::defAttr_enable)
				&& pUser->GetEnable() != pCurUser->GetEnable() )
			{
				doUpdate = true;
				pCurUser->SetEnable( pUser->GetEnable() );
			}

			if( pUser->IsValidAttribute(GSIOTUser::defAttr_UserLevel)
				&& pUser->GetUserLevel() != pCurUser->GetUserLevel() )
			{
				doUpdate = true;
				pCurUser->SetUserLevel( pUser->GetUserLevel() );
			}

			if( pUser->IsValidAttribute(GSIOTUser::defAttr_NoticeGroup)
				&& pUser->get_UserFlag(defUserFlag_NoticeGroup) != pCurUser->get_UserFlag(defUserFlag_NoticeGroup) )
			{
				doUpdate = true;
				pCurUser->set_UserFlag( defUserFlag_NoticeGroup, pUser->get_UserFlag(defUserFlag_NoticeGroup) );
			}

			if( pUser->IsValidAttribute(GSIOTUser::defAttr_name)
				&& pUser->GetName() != pCurUser->GetName() )
			{
				doUpdate = true;
				pCurUser->SetName( pUser->GetName() );
			}

			if( doUpdate )
			{
				LOGMSG( "CfgChange_User(oprt=%d) do modify newenable=%d, newname=%s, newNoticeGroup=%d, jid=%s\r\n", oprt, pCurUser->GetEnable(), pCurUser->GetName().c_str(), pCurUser->get_UserFlag(defUserFlag_NoticeGroup), jid.c_str() );

				db_Update_user_base( pCurUser );
			}
		}

		// �Ƚϱ仯
		UseDbTransAction dbta( doTRANSACTION?db:NULL );
		const defmapAuth &mapAuth = pUser->GetList_Auth();
		for( defmapAuth::const_iterator it=mapAuth.begin(); it!=mapAuth.end(); ++it )
		{
			const struAuth *pAuth = &(it->second);
			struAuth *pCurAuth = (struAuth*)pCurUser->Get_Auth( pAuth->device_type, pAuth->device_id );
			if( pCurAuth )
			{
				if( pAuth->authority != pCurAuth->authority )
				{
					//update
					//if( defCfgOprt_Modify == oprt
					//	|| defCfgOprt_AddModify == oprt )
					{
						macDebugLog_GSIOTUserMgr( "CfgChange_User(oprt=%d) do modify, %s\r\n", oprt, pAuth->Print( jid.c_str(), false ).c_str() );

						pCurAuth->authority = pAuth->authority;
						db_Update_user_auth( pCurUser->GetID(), *pCurAuth );
					}
				}
				else
				{
					macDebugLog_GSIOTUserMgr( "CfgChange_User(oprt=%d) authority is same, %s\r\n", oprt, pAuth->Print( jid.c_str(), false ).c_str() );
				}
			}
			else if( pAuth->authority > defUserAuth_Null ) // �µ���Ȩ�����ò����ӣ��������
			{
				if( Pass_mapAuthAdd )
				{
					if( !GSIOTUser::IsAllAuthConfig( pAuth->device_type, pAuth->device_id ) )
						continue;
				}

				{
					macDebugLog_GSIOTUserMgr( "CfgChange_User(oprt=%d) do add, %s\r\n", oprt, pAuth->Print( jid.c_str(), false ).c_str() );

					pCurUser->CfgChange_Auth( *pAuth, oprt, FailedIsErr );
					db_Insert_user_auth( pCurUser->GetID(), *pAuth );
				}
			}
			else
			{
				macDebugLog_GSIOTUserMgr( "CfgChange_User(oprt=%d) auth is null, no add, %s\r\n", oprt, pAuth->Print( jid.c_str(), false ).c_str() );
			}
		}
		dbta.Commit();
		
		this->OnNotify( defNotify_Update, pCurUser );
	}
	else
	{
		if( defCfgOprt_Modify==oprt )
		{
			//LOGMSGEX( defLOGNAME, FailedIsErr?defLOG_ERROR:defLOG_INFO, "CfgChange_User(oprt=%d) err, not found! %s\r\n", oprt, jid.c_str() );
			return defGSReturn_NoExist;
		}

		if( jid.empty() )
		{
			return defGSReturn_Err;
		}

		if( !g_CheckEMail( std::string(jid) ) )  
		{
			//return defGSReturn_Err;  //jyc20170301 remove   no <regex> lib
		}
		
		// add or addmodify
		LOGMSG( "CfgChange_User(oprt=%d) do add, %s\r\n", oprt, jid.c_str() );

		GSIOTUser *pNewUser = pUser->clone();

		// ���ʱ������Ѿ�������Ȩ�ޱ�־�����ñ������Ȩ�޼�¼
		if( pNewUser->IsAllAuthCfg() )
		{
			LOGMSG( "CfgChange_User(oprt=%d) do add IsAll_Auth, %s\r\n", oprt, jid.c_str() );
			pNewUser->ResetOnlyAuth(false,false);
		}

		pNewUser->SetID( db_Insert_user_base( pNewUser ) );
		GSIOTUserMgr::usermapInsert( m_mapUser, pNewUser );

		UseDbTransAction dbta( doTRANSACTION?db:NULL );
		const defmapAuth mapAuth = pNewUser->GetList_Auth();
		for( defmapAuth::const_iterator it=mapAuth.begin(); it!=mapAuth.end(); ++it )
		{
			db_Insert_user_auth( pNewUser->GetID(), it->second );
		}
		dbta.Commit();

		this->OnNotify( defNotify_Add, pNewUser );
	}

	return defGSReturn_Success;
}

defGSReturn GSIOTUserMgr::Delete_User( const std::string &jid, const bool FailedIsErr )
{
	if( jid==XMPP_GSIOTUser_Admin )
	{
		return defGSReturn_Err;
	}

	defmapGSIOTUser::const_iterator it = GSIOTUserMgr::usermapFind( m_mapUser, jid );
	
	if( it !=m_mapUser.end() )
	{
		LOGMSG( "Delete_User jid=%s\r\n", jid.c_str() );

		db_Delete_user( it->second->GetID() );

		this->OnNotify( defNotify_Delete, it->second );

		delete (it->second);
		m_mapUser.erase(it->first);  //jyc20170301 notice m_mapUser.erase(it);
		return defGSReturn_Success;
	}

	//LOGMSGEX( defLOGNAME, FailedIsErr?defLOG_ERROR:defLOG_INFO, "Delete_User err, not found! jid=%s\r\n", jid.c_str() );
	return defGSReturn_NoExist;
}

// ����û�����Ȩ��
defGSReturn GSIOTUserMgr::ClearAuth_User( const std::string &jid, const bool FailedIsErr )
{
	if( jid==XMPP_GSIOTUser_Admin )
	{
		return defGSReturn_Err;
	}

	defmapGSIOTUser::const_iterator it = GSIOTUserMgr::usermapFind( m_mapUser, jid );
	if( it!=m_mapUser.end() )
	{
		LOGMSG( "ClearAuth_User jid=%s\r\n", jid.c_str() );

		it->second->ResetOnlyAuth();
		db_Delete_user_auth( it->second->GetID() );

		this->OnNotify( defNotify_Update, it->second );
		return defGSReturn_Success;
	}

	//LOGMSGEX( defLOGNAME, FailedIsErr?defLOG_ERROR:defLOG_INFO, "ClearAuth_User err, not found! jid=%s\r\n", jid.c_str() );
	return defGSReturn_NoExist;
}

// ��������û�������Ȩ��
defGSReturn GSIOTUserMgr::ClearAuth_AllUser()
{
	LOGMSG( "ClearAuth_AllUser" );

	db_Delete_alluser_auth();

	for( defmapGSIOTUser::const_iterator it=m_mapUser.begin(); it!=m_mapUser.end(); ++it )
	{
		it->second->ResetOnlyAuth();
		db_Delete_user_auth( it->second->GetID() );

		this->OnNotify( defNotify_Update, it->second );
	}

	return defGSReturn_Success;
}

void GSIOTUserMgr::OnNotify( defNotify_ notify, GSIOTUser *pUser )
{
	if( m_NotifyHandler )
	{
		m_NotifyHandler->OnNotify_UserMgr( notify, pUser );
	}
}

void GSIOTUserMgr::ReleaseUserList()
{
	usermapRelease( m_mapUser );
}

bool GSIOTUserMgr::LoadDB_User()
{
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "LoadDB_User" );

	SQLite::Statement query(*this->db,"select * from user_base order by name");
	while( query.executeStep() )
	{
		int col = 0;
		int id = query.getColumn(col++).getInt();
		int enable = query.getColumn(col++).getInt();
		std::string jid = query.getColumn(col++);
		std::string name = query.getColumn(col++);
		int authflag = query.getColumn(col++).getInt();

		GSIOTUser *pNewUser = new GSIOTUser();
		pNewUser->SetID(id);
		pNewUser->set_save_col_val_enable(enable);
		pNewUser->SetJid(jid);
		pNewUser->SetName(name);
		pNewUser->set_UserFlag_uintfull(authflag);

		std::string sql = "select * from user_auth where user_id="+util::int2string(pNewUser->GetID());
		SQLite::Statement auth_query(*this->db,sql.c_str());
		while(auth_query.executeStep())
		{
			int col = 2;
			IOTDeviceType device_type = (IOTDeviceType)auth_query.getColumn(col++).getInt();
			int device_id = auth_query.getColumn(col++).getInt();
			defUserAuth authority = (defUserAuth)auth_query.getColumn(col++).getInt();

			pNewUser->CfgChange_Auth( struAuth( device_type, device_id, authority ), defCfgOprt_Add, false, false );
		}

		GSIOTUserMgr::usermapInsert( m_mapUser, pNewUser );
	}

	return true;
}

int GSIOTUserMgr::db_Insert_user_base( GSIOTUser *const pNewUser )
{
	if( GSIOTUserMgr::IsGuest(pNewUser) )
	{
		pNewUser->SetName( defGuestName );
	}

	SQLite::Statement  query(*db,"INSERT INTO user_base VALUES(NULL,:enable,:jid,:name,:authflag)");

	int col = 1;
	query.bind(col++, (int)pNewUser->get_save_col_val_enable());
	query.bind(col++, pNewUser->GetJid());
	query.bind(col++, pNewUser->GetName());
	query.bind(col++, (int)pNewUser->get_UserFlag_uintfull());
	query.exec();

	int id = db->execAndGet("SELECT id FROM user_base ORDER BY id DESC LIMIT 1");

	g_Changed( defCfgOprt_Add, IOT_Obj_User, id );

	return id;
}

bool GSIOTUserMgr::db_Update_user_base( GSIOTUser *const pCurUser )
{
	if( GSIOTUserMgr::IsGuest(pCurUser) )
	{
		pCurUser->SetName( defGuestName );
	}

	char sqlbuf[1024] = {0};

	snprintf( sqlbuf, sizeof(sqlbuf), "update user_base set enable=%d, name='%s', authflag=%d where id=%d", 
		pCurUser->get_save_col_val_enable(), pCurUser->GetName().c_str(), pCurUser->get_UserFlag_uintfull(), pCurUser->GetID() );

	db->exec( sqlbuf );
	
	g_Changed( defCfgOprt_Modify, IOT_Obj_User, pCurUser->GetID() );
	return true;
}

int GSIOTUserMgr::db_Insert_user_auth( const int UserID, const struAuth &newauth )
{
	SQLite::Statement  query(*db,"INSERT INTO user_auth VALUES(NULL,:user_id,:device_type,:device_id,:authority)");

	int col = 1;
	query.bind(col++, UserID);
	query.bind(col++, newauth.device_type);
	query.bind(col++, newauth.device_id);
	query.bind(col++, newauth.authority);
	query.exec();

	int id = db->execAndGet("SELECT id FROM user_auth ORDER BY id DESC LIMIT 1");

	g_Changed( defCfgOprt_Modify, IOT_Obj_User, UserID );
	return id;
}

bool GSIOTUserMgr::db_Update_user_auth( const int UserID, const struAuth &curauth )
{
	char sqlbuf[1024] = {0};

	snprintf( sqlbuf, sizeof(sqlbuf), "update user_auth set authority=%d where user_id=%d and device_type=%d and device_id=%d", 
		curauth.authority, UserID, curauth.device_type, curauth.device_id );

	db->exec( sqlbuf );

	g_Changed( defCfgOprt_Modify, IOT_Obj_User, UserID );
	return true;
}

bool GSIOTUserMgr::db_Delete_user( const int UserID )
{
	char sqlbuf[1024] = {0};

	db_Delete_user_auth( UserID );

	snprintf( sqlbuf, sizeof(sqlbuf), "DELETE FROM user_base WHERE id=%d", UserID );
	db->exec( sqlbuf );

	g_Changed( defCfgOprt_Delete, IOT_Obj_User, UserID );
	return true;
}

bool GSIOTUserMgr::db_Delete_user_auth( const int UserID )
{
	char sqlbuf[1024] = {0};

	snprintf( sqlbuf, sizeof(sqlbuf), "DELETE FROM user_auth WHERE user_id=%d", UserID );
	db->exec( sqlbuf );

	g_Changed( defCfgOprt_Modify, IOT_Obj_User, UserID );
	return true;
}

bool GSIOTUserMgr::db_Delete_alluser_auth()
{
	db->exec( "DELETE FROM user_auth" );
	
	g_Changed( defCfgOprt_Modify, IOT_Obj_User, 0 );
	return true;
}

bool GSIOTUserMgr::Update_user_base( GSIOTUser *pCurUser )
{
	db_Update_user_base( pCurUser );
	this->OnNotify( defNotify_Update, pCurUser );
	return true;
}
