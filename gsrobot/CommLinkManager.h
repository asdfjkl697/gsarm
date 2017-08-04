#pragma once
#include "SQLiteHelper.h"
#include "CommLink.h"
#include "gsiot/GSIOTDevice.h"

typedef std::map<defLinkID,CommLinkCfg*> defmapCommLinkCfg;
typedef std::map<GSIOTDeviceKey,CommLinkRelation> defmapCommLinkRelation;
typedef std::map<defLinkID,CCommLinkRun*> defmapCommLinkRun;

struct struDelCommLink
{
	uint32_t delts;			// ɾ��ʱ��ʱ���
	CommLinkCfg *cfgobj;	// ��ɾ������
	CCommLinkRun *runobj;	// ��ɾ�����ж���

	struDelCommLink()
	{
		delts = 0;
		cfgobj = NULL;
		runobj = NULL;
	}
};

void g_del_struDelCommLink( struDelCommLink &delobj );

class CCommLinkManager : public SQLiteHelper
{
public:
	CCommLinkManager(void);
	~CCommLinkManager(void);

	bool Init();
	bool UnInit();

	void CheckDeleteList();

	// ���ò���
	CommLinkCfg* Cfg_Get( defLinkID LinkID ) const;
	defGSReturn Cfg_CheckExist( const CommLinkCfg &cfgobj, std::string *strerr );
	CommLinkCfg* Cfg_Add( CommLinkCfg &cfgobj, defGSReturn &ret, std::string *strerr );
	defGSReturn Cfg_Delete( const defLinkID LinkID );
	defGSReturn Cfg_Modify( CommLinkCfg &cfgobj, std::string *strerr );
	defmapCommLinkCfg& Cfg_GetList() { return m_CfgList; }
	std::string Cfg_GetShowNameForUI( const IOTDeviceType device_type, const defLinkID LinkID ) const;

	// ��·����
	bool Relation_Get( const GSIOTDeviceKey &key, CommLinkRelation &Relation ) const;
	defGSReturn Relation_AddModify( CommLinkRelation &Relation );
	defGSReturn Relation_DeleteForDev( const GSIOTDeviceKey &key );
	defGSReturn Relation_DeleteForLink( const defLinkID LinkID );
	defmapCommLinkRelation& Relation_GetList() { return m_RelationList; }

	// ���в���
	CCommLinkRun* Run_Proc_Get( const bool allowConnect );
	void Run_Proc_Release( CCommLinkRun *runobj );
	CCommLinkRun* Run_Info_Get( defLinkID LinkID );
	void Run_Info_Release( CCommLinkRun *runobj );
	bool Run_IsUsing( defLinkID LinkID );
	defConnectState Run_GetConnectState( defLinkID LinkID );
	void Run_GetAllStateInfo( int &enableCount, bool &allConnectState );

	bool Run_Reconnect( defLinkID LinkID );
	bool Run_ReconnectAll();
	void Run_Print();

private:
	bool Run_IsUsingSpec( const CCommLinkRun &runobj );
	defGSReturn Run_Add( CommLinkCfg *cfgobj, std::string *strerr );
	CCommLinkRun* Run_Remove( const defLinkID LinkID );
	CCommLinkRun* Run_GetSpec_nolock( defLinkID LinkID );

	bool LoadDB_comm_link();
	bool LoadDB_comm_link_relation();
	int db_Insert_comm_link( CommLinkCfg *const cfgobj );
	bool db_Update_comm_link( CommLinkCfg *const cfgobj );
	bool db_Delete_comm_link( const defLinkID LinkID );
	int db_Insert_comm_link_relation( CommLinkRelation &Relation );
	bool db_Update_comm_link_relation( CommLinkRelation &Relation );
	bool db_Delete_comm_link_relation( const GSIOTDeviceKey &key );
	bool db_Delete_comm_link_relation_all( const defLinkID LinkID );
	bool db_IsBeing_comm_link_relation( IOTDeviceType device_type, int device_id );

private:
	gloox::util::Mutex m_mutex_CommLinkMgr;

	defmapCommLinkCfg m_CfgList; // <ID,*>
	defmapCommLinkRelation m_RelationList; // <DevKey,rel>
	defmapCommLinkRun m_RunList; // <ID,*>

	uint32_t m_last_CheckDeleteList;
	std::list<struDelCommLink> m_DeleteList; // ��ɾ���б�
};

// �Զ���ȡ���ͷŷ�װ��
class CCommLinkAuto_Run_Proc_Get
{
public:

	CCommLinkAuto_Run_Proc_Get( CCommLinkManager &Mgr, const bool allowConnect ) : m_Mgr(Mgr)
	{
		m_pCommLink = m_Mgr.Run_Proc_Get( allowConnect );
	}

	~CCommLinkAuto_Run_Proc_Get()
	{
		if( m_pCommLink )
		{
			m_Mgr.Run_Proc_Release(m_pCommLink);
		}
	}

	CCommLinkRun* p()
	{
		return m_pCommLink;
	}

private:
	CCommLinkAuto_Run_Proc_Get& operator=( const CCommLinkAuto_Run_Proc_Get& );
	CCommLinkRun *m_pCommLink;
	CCommLinkManager &m_Mgr;
};

// �Զ���ȡ���ͷŷ�װ��
class CCommLinkAuto_Run_Info_Get
{
public:

	CCommLinkAuto_Run_Info_Get( CCommLinkManager &Mgr, defLinkID LinkID ) : m_Mgr(Mgr)
	{
		m_pCommLink = m_Mgr.Run_Info_Get(LinkID);
	}

	~CCommLinkAuto_Run_Info_Get()
	{
		if( m_pCommLink )
		{
			m_Mgr.Run_Info_Release(m_pCommLink);
		}
	}

	CCommLinkRun* p()
	{
		return m_pCommLink;
	}

private:
	CCommLinkAuto_Run_Info_Get& operator=( const CCommLinkAuto_Run_Info_Get& );
	CCommLinkRun *m_pCommLink;
	CCommLinkManager &m_Mgr;
};
