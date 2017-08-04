#include "CommLinkManager.h"
//#include <Windows.h>

void g_del_struDelCommLink( struDelCommLink &delobj )
{
	if( delobj.runobj )
	{
		delete delobj.runobj;
	}

	if( delobj.cfgobj )
	{
		delete delobj.cfgobj;
	}
}

CCommLinkManager::CCommLinkManager(void)
	:SQLiteHelper()
{
	m_last_CheckDeleteList = timeGetTime();

}

CCommLinkManager::~CCommLinkManager(void)
{
}

// 初始化
bool CCommLinkManager::Init()
{
	LoadDB_comm_link();
	LoadDB_comm_link_relation();

	//LOGMSGEX( defLOGNAME, defLOG_SYS, "Create CommLinkRun count=%d", m_CfgList.size() );

	if( !m_CfgList.empty() )
	{
		for( defmapCommLinkCfg::iterator it = m_CfgList.begin(); it!= m_CfgList.end(); ++it )
		{
			switch(it->second->link_type)
			{
			case defCommLinkType_TCP:
				{
					m_RunList[it->second->id] = new CCommLinkRun_TCP(it->second);
				}
				break;

			default:
				break;
			}
		}
	}

	return true;
}

// 反初始化、进行释放
bool CCommLinkManager::UnInit()
{
	if( !m_RunList.empty() )
	{
		for( defmapCommLinkRun::iterator it = m_RunList.begin(); it!=m_RunList.end(); ++it )
		{
			delete it->second;
		}
		m_RunList.clear();
	}

	if( !m_DeleteList.empty() )
	{
		for( std::list<struDelCommLink>::iterator it = m_DeleteList.begin(); it!= m_DeleteList.end(); ++it )
		{
			g_del_struDelCommLink( *it );
		}
		m_DeleteList.clear();
	}

	if( !m_CfgList.empty() )
	{
		for( defmapCommLinkCfg::iterator it = m_CfgList.begin(); it!= m_CfgList.end(); ++it )
		{
			delete it->second;
		}
		m_CfgList.clear();
	}

	return true;
}

// 检测需最终删除的对象列表
void CCommLinkManager::CheckDeleteList()
{
	if( timeGetTime()-m_last_CheckDeleteList < 20*1000 )
	{
		return;
	}

	if( m_DeleteList.empty() )
	{
		return;
	}

	for( std::list<struDelCommLink>::iterator it = m_DeleteList.begin(); it!= m_DeleteList.end(); ++it )
	{
		if( it->runobj )
		{
			if( Run_IsUsingSpec( *(it->runobj) ) )
				continue;
		}

		g_del_struDelCommLink( *it );

		const defLinkID id = it->cfgobj?it->cfgobj->id:-1;
		m_DeleteList.erase(it);

		LOGMSG( "CommLinkMgr:DeleteList FinalDel LinkID=%d, aftercount(%d)", id, m_DeleteList.size() );

		return;
	}
}


//=====================================================
// db

bool CCommLinkManager::LoadDB_comm_link()
{
	//LOGMSGEX( defLOGNAME, defLOG_INFO, "LoadDB_comm_link" );

	SQLite::Statement query(*this->db,"select * from comm_link");
	while( query.executeStep() )
	{
		CommLinkCfg *cfg = new CommLinkCfg();

		int col = 0;
		cfg->id = query.getColumn(col++).getInt();
		//jyc20160826 modify
		string tmp1 = query.getColumn(col++);
		cfg->name = tmp1;
		cfg->enable = query.getColumn(col++).getInt();
		cfg->link_type = (defCommLinkType)query.getColumn(col++).getInt();
		cfg->trans_mod = (defTransMod)query.getColumn(col++).getInt();
		cfg->heartbeat_type = (defCommLinkHeartbeatType)query.getColumn(col++).getInt();
		string tmp2 = query.getColumn(col++);
		cfg->heartbeat_param.setstrfmt( tmp2 ); //jyc20160826 modify

		cfg->param[0] = query.getColumn(col++).getInt();
		cfg->param[1] = query.getColumn(col++).getInt();
		cfg->param[2] = query.getColumn(col++).getInt();
		cfg->param[3] = query.getColumn(col++).getInt();
		cfg->param[4] = query.getColumn(col++).getInt();
		cfg->param[5] = query.getColumn(col++).getInt();

		//cfg->param_str[0] = query.getColumn(col++);
		//cfg->param_str[1] = query.getColumn(col++);
		string tmp3 = query.getColumn(col++);
		cfg->param_str[0] = tmp3;
		string tmp4 = query.getColumn(col++);
		cfg->param_str[1] = tmp4;

		if( defCommLinkType_TCP != cfg->link_type )
		{
			LOGMSG( "LoadDB_comm_link(LinkID=%d) link_type=%d nosupport", cfg->id, cfg->link_type );
			delete cfg;
			continue;
		}

		m_CfgList[cfg->id] = cfg;
	}

	return true;
}

bool CCommLinkManager::LoadDB_comm_link_relation()
{
	//LOGMSGEX( defLOGNAME, defLOG_INFO, "LoadDB_comm_link_relation" );

	SQLite::Statement query(*this->db,"select * from comm_link_relation");
	while( query.executeStep() )
	{
		CommLinkRelation Relation;

		int col = 0;
		int id = query.getColumn(col++).getInt();
		Relation.device_type = (IOTDeviceType)query.getColumn(col++).getInt();
		Relation.device_id = query.getColumn(col++).getInt();
		Relation.link_id = query.getColumn(col++).getInt();
		Relation.link_id_bak = query.getColumn(col++).getInt();

		m_RelationList[GSIOTDeviceKey(Relation.device_type,Relation.device_id)] = Relation;
	}

	return true;
}

int CCommLinkManager::db_Insert_comm_link( CommLinkCfg *const cfgobj )
{
	SQLite::Statement  query(*db,"INSERT INTO comm_link VALUES(NULL,:name,:enable,:link_type,:trans_mod,:heartbeat_type,:heartbeat_param,"\
		":param1,:param2,:param3,:param4,:param5,:param6,"\
		":param_str1,:param_str2)");

	int col = 1;
	query.bind( col++, cfgobj->name );
	query.bind( col++, cfgobj->enable );
	query.bind( col++, cfgobj->link_type );
	query.bind( col++, cfgobj->trans_mod );
	query.bind( col++, cfgobj->heartbeat_type );
	query.bind( col++, cfgobj->heartbeat_param.getstrfmt(cfgobj->heartbeat_type) );
	query.bind( col++, cfgobj->param[0] );
	query.bind( col++, cfgobj->param[1] );
	query.bind( col++, cfgobj->param[2] );
	query.bind( col++, cfgobj->param[3] );
	query.bind( col++, cfgobj->param[4] );
	query.bind( col++, cfgobj->param[5] );
	query.bind( col++, cfgobj->param_str[0] );
	query.bind( col++, cfgobj->param_str[1] );
	query.exec();

	int id = db->execAndGet("SELECT id FROM comm_link ORDER BY id DESC LIMIT 1");
	
	g_Changed( defCfgOprt_Add, IOT_Obj_CommLink, id );

	return id;
}

bool CCommLinkManager::db_Update_comm_link( CommLinkCfg *const cfgobj )
{
	char sqlbuf[4096] = {0};

	snprintf( sqlbuf, sizeof(sqlbuf), "update comm_link set name='%s', enable=%d, link_type=%d, trans_mod=%d, heartbeat_type=%d, heartbeat_param='%s',"\
		"param1=%d, param2=%d, param3=%d, param4=%d, param5=%d, param6=%d, param_str1='%s', param_str2='%s' where id=%d", 
		cfgobj->name.c_str(), cfgobj->enable, cfgobj->link_type, cfgobj->trans_mod, cfgobj->heartbeat_type, cfgobj->heartbeat_param.getstrfmt(cfgobj->heartbeat_type).c_str(),
		cfgobj->param[0], cfgobj->param[1], cfgobj->param[2], cfgobj->param[3], cfgobj->param[4], cfgobj->param[5], 
		cfgobj->param_str[0].c_str(), cfgobj->param_str[1].c_str(), cfgobj->id );

	db->exec( sqlbuf );

	g_Changed( defCfgOprt_Modify, IOT_Obj_CommLink, cfgobj->id );
	return true;
}

bool CCommLinkManager::db_Delete_comm_link( const defLinkID LinkID )
{
	char sqlbuf[1024] = {0};

	snprintf( sqlbuf, sizeof(sqlbuf), "DELETE FROM comm_link WHERE id=%d", LinkID );
	db->exec( sqlbuf );

	g_Changed( defCfgOprt_Delete, IOT_Obj_CommLink, LinkID );
	return true;
}

int CCommLinkManager::db_Insert_comm_link_relation( CommLinkRelation &Relation )
{
	SQLite::Statement  query(*db,"INSERT INTO comm_link_relation VALUES(NULL,:device_type,:device_id,:link_id,:link_id_bak,"\
		":param1,:param2,:param3,:param4)");

	int col = 1;
	query.bind( col++, Relation.device_type );
	query.bind( col++, Relation.device_id );
	query.bind( col++, Relation.link_id );
	query.bind( col++, Relation.link_id_bak );
	query.bind( col++, int(0) );
	query.bind( col++, int(0) );
	query.bind( col++, int(0) );
	query.bind( col++, int(0) );
	query.exec();

	g_Changed( defCfgOprt_Modify, IOT_Obj_CommLink, Relation.link_id, defIOTDeviceAddr_Other );
	return 1;
}

bool CCommLinkManager::db_Update_comm_link_relation( CommLinkRelation &Relation )
{
	char sqlbuf[1024] = {0};

	snprintf( sqlbuf, sizeof(sqlbuf), "update comm_link_relation set link_id=%d, link_id_bak=%d where device_type=%d and device_id=%d", 
		Relation.link_id, Relation.link_id_bak, Relation.device_type, Relation.device_id );

	db->exec( sqlbuf );

	g_Changed( defCfgOprt_Modify, IOT_Obj_CommLink, Relation.link_id, defIOTDeviceAddr_Other );
	return true;
}

bool CCommLinkManager::db_Delete_comm_link_relation( const GSIOTDeviceKey &key )
{
	char sqlbuf[1024] = {0};

	snprintf( sqlbuf, sizeof(sqlbuf), "DELETE FROM comm_link_relation where device_type=%d and device_id=%d", key.m_type, key.m_id );
	db->exec( sqlbuf );

	g_Changed( defCfgOprt_Modify, IOT_Obj_CommLink, 0, defIOTDeviceAddr_Other );
	return true;
}

bool CCommLinkManager::db_Delete_comm_link_relation_all( const defLinkID LinkID )
{
	char sqlbuf[1024] = {0};

	snprintf( sqlbuf, sizeof(sqlbuf), "DELETE FROM comm_link_relation where link_id=%d", LinkID );
	db->exec( sqlbuf );

	g_Changed( defCfgOprt_Modify, IOT_Obj_CommLink, LinkID, defIOTDeviceAddr_Other );
	return true;
}

bool CCommLinkManager::db_IsBeing_comm_link_relation( IOTDeviceType device_type, int device_id )
{
	char sqlbuf[1024] = {0};
	snprintf( sqlbuf, sizeof(sqlbuf), "SELECT id FROM comm_link_relation where device_type=%d and device_id=%d", 
		device_type, device_id );

	SQLite::Statement query( *db, sqlbuf );
	if( query.executeStep() )
	{
		return true;
	}

	return false;
}

//=====================================================
// Cfg

// 获取链路配置信息
CommLinkCfg* CCommLinkManager::Cfg_Get( defLinkID LinkID ) const
{
	defmapCommLinkCfg::const_iterator it = this->m_CfgList.find(LinkID);
	if( it != this->m_CfgList.end() )
	{
		return it->second;
	}

	return NULL;
}

// 链路是否已存在等信息判断
defGSReturn CCommLinkManager::Cfg_CheckExist( const CommLinkCfg &cfgobj, std::string *strerr )
{
	for( defmapCommLinkCfg::const_iterator it = m_CfgList.begin(); it!= m_CfgList.end(); ++it )
	{
		if( cfgobj.id == it->second->id )
			continue;
		
		if( cfgobj.name == it->second->name )
		{
			if( strerr ) *strerr = "名称已存在";
			return defGSReturn_SameName;
		}

		if( cfgobj == *(it->second) )
		{
			if( strerr ) *strerr = "配置已存在";
			return defGSReturn_IsExist;
		}
	}

	return defGSReturn_Success;
}

// 添加链路配置信息
// (添加完成后在运行线程管理里添加线程)
CommLinkCfg* CCommLinkManager::Cfg_Add( CommLinkCfg &cfgobj, defGSReturn &ret, std::string *strerr )
{
	ret = Cfg_CheckExist( cfgobj, strerr );
	if( macGSFailed(ret) )
	{
		return NULL;
	}

	cfgobj.id = db_Insert_comm_link( &cfgobj );
	if( cfgobj.id < 0 )
	{
		if( strerr ) *strerr = "保存失败";
		ret = defGSReturn_SaveFailed;
		return NULL;
	}

	CommLinkCfg *newcfgobj = new CommLinkCfg(cfgobj);

	ret = Run_Add( newcfgobj, strerr );
	if( macGSFailed(ret) )
	{
		delete newcfgobj;
		return NULL;
	}

	m_CfgList[newcfgobj->id] = newcfgobj;

	LOGMSG( "CommLinkMgr:CfgList Add LinkID=%d, aftercount(%d)", newcfgobj->id, m_CfgList.size() );

	return newcfgobj;
}

// 删除链路配置信息
defGSReturn CCommLinkManager::Cfg_Delete( const defLinkID LinkID )
{
	db_Delete_comm_link( LinkID );

	Relation_DeleteForLink( LinkID );

	struDelCommLink delobj;
	delobj.delts = timeGetTime();

	//defmapCommLinkCfg::const_iterator it = this->m_CfgList.find(LinkID);
	defmapCommLinkCfg::iterator it = this->m_CfgList.find(LinkID);
	if( it != this->m_CfgList.end() )
	{
		it->second->enable = 0;
		delobj.cfgobj = it->second;
		m_CfgList.erase(it);

		LOGMSG( "CommLinkMgr:CfgList Del LinkID=%d, aftercount(%d)", LinkID, m_CfgList.size() );
	}

	delobj.runobj = Run_Remove( LinkID );

	{
		gloox::util::MutexGuard mutexguard( m_mutex_CommLinkMgr );

		m_DeleteList.push_back( delobj );
	}

	LOGMSG( "CommLinkMgr:DeleteList insert LinkID=%d, aftercount(%d)", LinkID, m_DeleteList.size() );

	return defGSReturn_Success;
}

// 修改链路配置信息
defGSReturn CCommLinkManager::Cfg_Modify( CommLinkCfg &cfgobj, std::string *strerr )
{
	CommLinkCfg *cfgcur = Cfg_Get( cfgobj.id );
	if( !cfgcur )
	{
		if( strerr ) *strerr = "配置不存在";
		return defGSReturn_NoExist;
	}

	defGSReturn ret = Cfg_CheckExist( cfgobj, strerr );
	if( macGSFailed(ret) )
	{
		return ret;
	}

	const uint32_t lastCfgTs = cfgcur->lastCfgTs;
	if( cfgcur != &cfgobj )
	{
		*cfgcur = cfgobj;
	}
	cfgcur->lastCfgTs = lastCfgTs;

	cfgcur->refreshCfgTs();

	db_Update_comm_link( cfgcur );

	return defGSReturn_Success;
}

std::string CCommLinkManager::Cfg_GetShowNameForUI( const IOTDeviceType device_type, const defLinkID LinkID ) const
{
	if( IOT_DEVICE_Camera == device_type )
		return std::string("");

	if( LinkID <= 0 )
		return std::string("");

	const CommLinkCfg *cfg = Cfg_Get( LinkID );
	if( cfg )
	{
		if( !cfg->enable )
		{
			return std::string("(禁用)") + cfg->name;
		}

		return cfg->name;
	}

	char sqlbuf[256] = {0};
	snprintf( sqlbuf, sizeof(sqlbuf), "(链路%d不存在)", LinkID );

	return std::string( sqlbuf );
}


//=====================================================
// Relation

// 获取设备所关联的链路
bool CCommLinkManager::Relation_Get( const GSIOTDeviceKey &key, CommLinkRelation &Relation ) const
{
	defmapCommLinkRelation::const_iterator it = this->m_RelationList.find(key);
	if( it != this->m_RelationList.end() )
	{
		Relation = it->second;
		return true;
	}

	return false;
}

// 添加或修改链路关联
defGSReturn CCommLinkManager::Relation_AddModify( CommLinkRelation &Relation )
{
	if( db_IsBeing_comm_link_relation( Relation.device_type, Relation.device_id ) )
	{
		db_Update_comm_link_relation( Relation );
	}
	else
	{
		db_Insert_comm_link_relation( Relation );
	}

	m_RelationList[GSIOTDeviceKey(Relation.device_type,Relation.device_id)] = Relation;

	return defGSReturn_Success;
}

// 根据设备删除链路关联
defGSReturn CCommLinkManager::Relation_DeleteForDev( const GSIOTDeviceKey &key )
{
	db_Delete_comm_link_relation( key );

	//defmapCommLinkRelation::const_iterator it = this->m_RelationList.find(key);
	defmapCommLinkRelation::iterator it = this->m_RelationList.find(key);
	if( it != this->m_RelationList.end() )
	{
		m_RelationList.erase(it);
	}

	return defGSReturn_Success;
}

// 根据链路ID删除所有相关的链路关联
defGSReturn CCommLinkManager::Relation_DeleteForLink( const defLinkID LinkID )
{
	db_Delete_comm_link_relation_all( LinkID );

	//for( defmapCommLinkRelation::const_iterator it=m_RelationList.begin(); it!=m_RelationList.end(); )
	for( defmapCommLinkRelation::iterator it=m_RelationList.begin(); it!=m_RelationList.end(); )
	{
		if( LinkID == it->second.link_id )
		{
			m_RelationList.erase(it);
			it=m_RelationList.begin();
			continue;
		}

		++it;
	}

	return defGSReturn_Success;
}


//=====================================================
// Run

// 获取一个轮到处理的运行对象
// allowConnect:当前获取的线程是否运行进行连接操作
CCommLinkRun* CCommLinkManager::Run_Proc_Get( const bool allowConnect )
{
	gloox::util::MutexGuard mutexguard( m_mutex_CommLinkMgr );

	CheckDeleteList();
	
	if( !m_RunList.empty() )
	{
		CCommLinkRun *runobj = NULL;

		//printf("runlist size=%d\n",m_RunList.size()); //jyc20160914
		unsigned int testii = 0;
		for( defmapCommLinkRun::iterator it = m_RunList.begin(); it!=m_RunList.end(); ++it )
		{
			++testii;
			if( it->second->m_lockProc ){
				//printf("lockproc %d...\n",testii);
				continue;
			}

			it->second->check_cfg();

			if( !it->second->get_cfg().enable )
			{
				if( !it->second->IsOpen() )
				{
					//printf("no open %d...\n",testii);
					continue;
				}
			}

			//printf("runlist size=%d listnum=%d\n",m_RunList.size(),testii);

			if( !allowConnect )
			{
				// 未连接需要进行连接
				if( !it->second->IsOpen() )
				{
					continue;
				}

				// 需要重连
				if( it->second->get_doReconnect() )
				{
					continue;
				}

				// 不启用需要关闭连接
				if( !it->second->get_cfg().enable && it->second->IsOpen() )
				{
					continue;
				}
			}

			if( runobj )
			{
				if( it->second->m_lastWork < runobj->m_lastWork )
				{
					runobj = it->second;
				}
			}
			else
			{
				runobj = it->second;
			}

			if( 0==runobj->m_lastWork ){
				break;
			}
		}

		if( runobj )
		{
			static int s_lastWork_tick = 0;

			// reset
			if( s_lastWork_tick > 0x7FFFFFFF )
			{
				s_lastWork_tick = 0;
				for( defmapCommLinkRun::iterator it = m_RunList.begin(); it!=m_RunList.end(); ++it )
				{
					it->second->m_lastWork = 0;
				}
			}

			runobj->m_lockProc = true;
			runobj->m_lastWork = ++s_lastWork_tick;
			return runobj;
		}
	}
	
	return NULL;
}

// 释放获取的处理对象
void CCommLinkManager::Run_Proc_Release( CCommLinkRun *runobj )
{
	gloox::util::MutexGuard mutexguard( m_mutex_CommLinkMgr );

	if( runobj )
	{
		assert(runobj->m_lockProc);

		runobj->m_lockProc = false;
	}
}

// 获取一个指定对象，不处理，只获取信息
CCommLinkRun* CCommLinkManager::Run_Info_Get( defLinkID LinkID )
{
	gloox::util::MutexGuard mutexguard( m_mutex_CommLinkMgr );

	CCommLinkRun *runobj = Run_GetSpec_nolock( LinkID );
	if( runobj )
	{
		runobj->m_lockInfoUse = true;
	}

	return runobj;
}

// 释放获取的信息对象
void CCommLinkManager::Run_Info_Release( CCommLinkRun *runobj )
{
	gloox::util::MutexGuard mutexguard( m_mutex_CommLinkMgr );

	if( runobj )
	{
		assert(runobj->m_lockInfoUse);

		runobj->m_lockInfoUse = false;
	}
}

CCommLinkRun* CCommLinkManager::Run_GetSpec_nolock( defLinkID LinkID )
{
	if( !m_RunList.empty() )
	{
		defmapCommLinkRun::iterator it = m_RunList.find( LinkID );
		if( it != m_RunList.end() )
		{
			return it->second;
		}
	}

	return NULL;
}

// 判断一个链路是否在使用，包括是否在处理或者在信息查看
bool CCommLinkManager::Run_IsUsing( defLinkID LinkID )
{
	gloox::util::MutexGuard mutexguard( m_mutex_CommLinkMgr );

	CCommLinkRun *runobj = Run_GetSpec_nolock( LinkID );
	if( runobj )
	{
		return Run_IsUsingSpec( *runobj );
	}

	return false;
}

// 判断一个链路是否在使用，包括是否在处理或者在信息查看
bool CCommLinkManager::Run_IsUsingSpec( const CCommLinkRun &runobj )
{
	return ( runobj.m_lockProc || runobj.m_lockInfoUse );
}

// 获取连接状态
defConnectState CCommLinkManager::Run_GetConnectState( defLinkID LinkID )
{
	gloox::util::MutexGuard mutexguard( m_mutex_CommLinkMgr );

	CCommLinkRun *runobj = Run_GetSpec_nolock( LinkID );
	if( runobj )
	{
		return runobj->get_ConnectState();
	}

	return defConnectState_Null;
}

// 获取综合状态信息
void CCommLinkManager::Run_GetAllStateInfo( int &enableCount, bool &allConnectState )
{
	enableCount = 0;
	allConnectState = true;

	gloox::util::MutexGuard mutexguard( m_mutex_CommLinkMgr );

	if( m_RunList.empty() )
	{
		return;
	}

	for( defmapCommLinkRun::iterator it = m_RunList.begin(); it!=m_RunList.end(); ++it )
	{
		if( !it->second->get_cfg().enable )
		{
			continue;
		}

		enableCount++;

		if( !it->second->IsOpen() )
		{
			allConnectState = false;
			return;
		}
	}
}

// 添加一个运行对象
defGSReturn CCommLinkManager::Run_Add( CommLinkCfg *cfgobj, std::string *strerr )
{
	if( !cfgobj )
	{
		assert(cfgobj);
		return defGSReturn_ErrParam;
	}

	gloox::util::MutexGuard mutexguard( m_mutex_CommLinkMgr );

	CCommLinkRun *runobj = Run_GetSpec_nolock( cfgobj->id );
	if( runobj )
	{
		if( strerr ) *strerr = "运行对象已存在";
		return defGSReturn_IsExist;
	}

	switch(cfgobj->link_type)
	{
	case defCommLinkType_TCP:
		{
			m_RunList[cfgobj->id] = new CCommLinkRun_TCP(cfgobj);

			LOGMSG( "CommLinkMgr:RunList Add LinkID=%d, aftercount(%d)", cfgobj->id, m_RunList.size() );

			return defGSReturn_Success;
		}
		break;

	case defCommLinkType_COM:
	case defCommLinkType_UDP:
	default:
		break;
	}

	if( strerr ) *strerr = "链路类型不支持";
	return defGSReturn_UnSupport;
}

// 移除一个运行对象，在deletelist里真正删除
CCommLinkRun* CCommLinkManager::Run_Remove( const defLinkID LinkID )
{
	gloox::util::MutexGuard mutexguard( m_mutex_CommLinkMgr );

	CCommLinkRun *runobj = NULL;

	defmapCommLinkRun::iterator it = m_RunList.find( LinkID );
	if( it != m_RunList.end() )
	{
		runobj = it->second;
		m_RunList.erase(it);

		LOGMSG( "CommLinkMgr:RunList Del LinkID=%d, aftercount(%d)", LinkID, m_RunList.size() );
	}

	return runobj;
}

// 链路重连
bool CCommLinkManager::Run_Reconnect( defLinkID LinkID )
{
	gloox::util::MutexGuard mutexguard( m_mutex_CommLinkMgr );

	CCommLinkRun *runobj = Run_GetSpec_nolock( LinkID );
	if( runobj )
	{
		runobj->set_doReconnect( 1, "UI" );
		return true;
	}

	return false;
}

// 所有链路重连
bool CCommLinkManager::Run_ReconnectAll()
{
	gloox::util::MutexGuard mutexguard( m_mutex_CommLinkMgr );

	if( !m_RunList.empty() )
	{
		for( defmapCommLinkRun::iterator it = m_RunList.begin(); it!=m_RunList.end(); ++it )
		{
			it->second->set_doReconnect( 1, "UI" );
		}
	}

	return true;
}

// 打印的运行对象信息
void CCommLinkManager::Run_Print()
{
	gloox::util::MutexGuard mutexguard( m_mutex_CommLinkMgr );

	const size_t runCount = m_RunList.size();

	//LOGMSGEX( defLOGNAME, defLOG_INFO, "CommLinkMgr:Run_Print runCount=%d", runCount );
	
	if( !m_RunList.empty() )
	{
		int i = 1;
		for( defmapCommLinkRun::iterator it = m_RunList.begin(); it!=m_RunList.end(); ++it, ++i )
		{
			/*
			LOGMSGEX( defLOGNAME, defLOG_INFO, "(%d/%d)(%s) lockProc=%d, enable=%d, IsOpen=%d, lastWork=%u",
				i, runCount,
				it->second->get_cfg().name.c_str(),
				(int)it->second->m_lockProc,
				it->second->get_cfg().enable,
				(int)it->second->IsOpen(),
				it->second->m_lastWork
				);*/
		}
	}
}
