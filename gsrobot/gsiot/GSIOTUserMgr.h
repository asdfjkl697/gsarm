#pragma once

#include "../SQLiteHelper.h"
#include "GSIOTUser.h"

class INotifyHandler_UserMgr
{
public:
	virtual void OnNotify_UserMgr( defNotify_ notify, GSIOTUser *pUser ){};
};

class GSIOTUserMgr
{
public:
	GSIOTUserMgr(void);
	~GSIOTUserMgr(void);

	static defmapGSIOTUser::const_iterator usermapFind( const defmapGSIOTUser &mapUser, const std::string jid );
	static void usermapInsert( defmapGSIOTUser &mapUser, GSIOTUser *pUser );
	static void usermapRelease( defmapGSIOTUser &mapUser );
	static void usermapCopy( defmapGSIOTUser &mapUserDest, const defmapGSIOTUser &mapUserSrc );

	void Init( SQLite::Database *pdb );

	SQLite::Database* get_db() const
	{
		return db;
	}

	// Ȩ���ж���
	GSIOTUser* check_GetUser( const std::string &jid );
	defGSReturn check_User( const GSIOTUser *pUser );
	defUserAuth check_Auth( const GSIOTUser *pUser, const IOTDeviceType type, const int id );

	static bool IsAdmin( const GSIOTUser *pUser );
	static bool IsGuest( const GSIOTUser *pUser );

	// Ȩ���������
	defmapGSIOTUser& GetList_User()
	{
		return this->m_mapUser;
	}

	// ֻ��������
	GSIOTUser* GetUser( const std::string &jid );
	defGSReturn CfgChange_User( GSIOTUser *const pUser, const defCfgOprt_ oprt, const bool FailedIsErr=true, bool doTRANSACTION=true );
	defGSReturn Delete_User( const std::string &jid, const bool FailedIsErr=true );
	defGSReturn ClearAuth_User( const std::string &jid, const bool FailedIsErr=true );
	defGSReturn ClearAuth_AllUser();
	bool Update_user_base( GSIOTUser *pCurUser );

	void ReleaseUserList();

	void SetNotifyHandler( INotifyHandler_UserMgr *handler )
	{
		m_NotifyHandler = handler;
	}
	void DelNotifyHandler()
	{
		m_NotifyHandler = NULL;
	}
	void OnNotify( defNotify_ notify, GSIOTUser *pUser );

protected:
	SQLite::Database *db;
	INotifyHandler_UserMgr *m_NotifyHandler;

private:
	bool LoadDB_User();
	int db_Insert_user_base( GSIOTUser *const pNewUser );
	bool db_Update_user_base( GSIOTUser *const pCurUser );
	int db_Insert_user_auth( const int UserID, const struAuth &newauth );
	bool db_Update_user_auth( const int UserID, const struAuth &curauth );
	bool db_Delete_user( const int UserID );
	bool db_Delete_user_auth( const int UserID );
	bool db_Delete_alluser_auth();

private:
	defmapGSIOTUser m_mapUser;
	GSIOTUser *m_GSIOTUser_Admin;
};

