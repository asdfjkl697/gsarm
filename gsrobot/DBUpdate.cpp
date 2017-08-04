#include "SQLiteHelper.h"
#include "gloox/util.h"
#include "common.h"

int g_GetDBVer( SQLite::Database *db )
{
	try
	{
		return db->execAndGet("SELECT ver FROM db_ver LIMIT 1");
	}
	catch(...)
	{
	}

	return int(0);
}

static void doDBUpdate_DBVer( SQLite::Database *db, int newDBVer )
{
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "doDBUpdate_DBVer(%d)...\r\n", newDBVer );

	char sqlbuf[256] = {0};
	snprintf( sqlbuf, sizeof(sqlbuf), "UPDATE db_ver set ver=%d", newDBVer );

	db->exec( sqlbuf );
}

static void doDBCreate_event_call( SQLite::Database *db )
{
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "doDBCreate_event_call...\r\n" );

	db->exec( "CREATE TABLE [event_call] ( \r\n"\
		"[id] INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL, \r\n"\
		"[device_type] INTEGER  NULL, \r\n"\
		"[device_id] INTEGER  NULL, \r\n"\
		"[level] INTEGER  NULL, \r\n"\
		"[enable] INTEGER  NULL, \r\n"\
		"[do_interval] INTEGER  NULL, \r\n"\
		"[try_count] INTEGER  NULL, \r\n"\
		"[play_count] INTEGER  NULL, \r\n"\
		"[flag1] INTEGER  NULL, \r\n"\
		"[flag2] INTEGER  NULL, \r\n"\
		"[phone] VARCHAR(255)  NULL, \r\n"\
		"[content] VARCHAR(255)  NULL)" );
}

static void doDBUpdate_devices_ipcamera( SQLite::Database *db )
{
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "doDBUpdate_devices_ipcamera Begin..." );

	//
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "CREATE devices_ipcamera_bak" );
	db->exec( "CREATE TEMPORARY TABLE devices_ipcamera_bak ( \r\n"\
		"[id] INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL, \r\n"\
		"[device_id] VARCHAR(255)  NULL, \r\n"\
		"[module_type] INTEGER  NULL, \r\n"\
		"[name] VARCHAR(255)  NULL, \r\n"\
		"[ver] VARCHAR(255)  NULL, \r\n"\
		"[ipaddress] VARCHAR(255)  NULL, \r\n"\
		"[port] INTEGER  NULL, \r\n"\
		"[username] VARCHAR(255)  NULL, \r\n"\
		"[password] VARCHAR(255)  NULL, \r\n"\
		"[channel] INTEGER  NULL, \r\n"\
		"[streamfmt] INTEGER  NULL, \r\n"\
		"[ptz] INTEGER  NULL, \r\n"\
		"[focal] INTEGER  NULL, \r\n"\
		"[focal_min] INTEGER  NULL, \r\n"\
		"[focal_max] INTEGER  NULL, \r\n"\
		"[focal_zoom] INTEGER  NULL, \r\n"\
		"[enable] INTEGER  NULL, \r\n"\
		"[buffertime] INTEGER DEFAULT '-1' NULL, \r\n"\
		"[rec_mod] INTEGER  NULL, \r\n"\
		"[rec_svrtype] INTEGER  NULL, \r\n"\
		"[rec_ipaddress] VARCHAR(255)  NULL, \r\n"\
		"[rec_port] INTEGER  NULL, \r\n"\
		"[rec_username] VARCHAR(255)  NULL, \r\n"\
		"[rec_password] VARCHAR(255)  NULL, \r\n"\
		"[rec_channel] INTEGER  NULL, \r\n"\
		"[FixTSMod] INTEGER  NULL, \r\n"\
		"[span_Min] INTEGER  NULL, \r\n"\
		"[span_Max] INTEGER  NULL, \r\n"\
		"[fix_Min] INTEGER  NULL, \r\n"\
		"[fix_Max] INTEGER  NULL, \r\n"\
		"[spanWarn] INTEGER  NULL, \r\n"\
		"[mfactor_span_Min] INTEGER  NULL, \r\n"\
		"[mfactor_span_Max] INTEGER  NULL, \r\n"\
		"[mfactor_fix_Min] INTEGER  NULL, \r\n"\
		"[mfactor_fix_Max] INTEGER  NULL, \r\n"\
		"[mfactor_spanWarn] INTEGER  NULL, \r\n"\
		"[Cam_Info1] INTEGER  NULL, \r\n"\
		"[Cam_Info2] INTEGER  NULL, \r\n"\
		"[Audio_Source] INTEGER  NULL, \r\n"\
		"[Audio_FmtType] INTEGER  NULL, \r\n"\
		"[Audio_ParamDef] INTEGER  NULL, \r\n"\
		"[Audio_Channels] INTEGER  NULL, \r\n"\
		"[Audio_bitSize] INTEGER  NULL, \r\n"\
		"[Audio_SampleRate] INTEGER  NULL, \r\n"\
		"[Audio_ByteRate] INTEGER  NULL, \r\n"\
		"[Audio_cap_src] VARCHAR(255)  NULL, \r\n"\
		"[Audio_cap_param] VARCHAR(255)  NULL);" );

	//
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "INSERT INTO devices_ipcamera_bak" );
	db->exec( "INSERT INTO devices_ipcamera_bak("\
		"[id],[device_id],[module_type],[name],[ver],[ipaddress],[port],[username],[password],[channel],[streamfmt],"\
		"[ptz],[focal],[focal_min],[focal_max],[focal_zoom],[enable],[buffertime],"\
		"[rec_mod],[rec_svrtype],[rec_ipaddress],[rec_port],[rec_username],[rec_password],[rec_channel],"\
		"[FixTSMod],[span_Min],[span_Max],[fix_Min],[fix_Max],[spanWarn],[mfactor_span_Min],[mfactor_span_Max],[mfactor_fix_Min],[mfactor_fix_Max],[mfactor_spanWarn] "\
		")"\
		"SELECT "\
		"[id],[device_id],[module_type],[name],[ver],[ipaddress],[port],[username],[password],[channel],[streamfmt],"\
		"[ptz],[focal],[focal_min],[focal_max],[focal_zoom],[enable],[buffertime],"\
		"[rec_mod],[rec_svrtype],[rec_ipaddress],[rec_port],[rec_username],[rec_password],[rec_channel],"\
		"[FixTSMod],[span_Min],[span_Max],[fix_Min],[fix_Max],[spanWarn],[mfactor_span_Min],[mfactor_span_Max],[mfactor_fix_Min],[mfactor_fix_Max],[mfactor_spanWarn] "\
		" FROM devices_ipcamera;" );

	//
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "DROP devices_ipcamera" );
	db->exec( "DROP TABLE devices_ipcamera;" );

	//
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "CREATE TABLE devices_ipcamera" );
	db->exec( "CREATE TABLE devices_ipcamera ( \r\n"\
		"[id] INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL, \r\n"\
		"[device_id] VARCHAR(255)  NULL, \r\n"\
		"[module_type] INTEGER  NULL, \r\n"\
		"[name] VARCHAR(255)  NULL, \r\n"\
		"[ver] VARCHAR(255)  NULL, \r\n"\
		"[ipaddress] VARCHAR(255)  NULL, \r\n"\
		"[port] INTEGER  NULL, \r\n"\
		"[username] VARCHAR(255)  NULL, \r\n"\
		"[password] VARCHAR(255)  NULL, \r\n"\
		"[channel] INTEGER  NULL, \r\n"\
		"[streamfmt] INTEGER  NULL, \r\n"\
		"[ptz] INTEGER  NULL, \r\n"\
		"[focal] INTEGER  NULL, \r\n"\
		"[focal_min] INTEGER  NULL, \r\n"\
		"[focal_max] INTEGER  NULL, \r\n"\
		"[focal_zoom] INTEGER  NULL, \r\n"\
		"[enable] INTEGER  NULL, \r\n"\
		"[buffertime] INTEGER DEFAULT '-1' NULL, \r\n"\
		"[rec_mod] INTEGER  NULL, \r\n"\
		"[rec_svrtype] INTEGER  NULL, \r\n"\
		"[rec_ipaddress] VARCHAR(255)  NULL, \r\n"\
		"[rec_port] INTEGER  NULL, \r\n"\
		"[rec_username] VARCHAR(255)  NULL, \r\n"\
		"[rec_password] VARCHAR(255)  NULL, \r\n"\
		"[rec_channel] INTEGER  NULL, \r\n"\
		"[FixTSMod] INTEGER  NULL, \r\n"\
		"[span_Min] INTEGER  NULL, \r\n"\
		"[span_Max] INTEGER  NULL, \r\n"\
		"[fix_Min] INTEGER  NULL, \r\n"\
		"[fix_Max] INTEGER  NULL, \r\n"\
		"[spanWarn] INTEGER  NULL, \r\n"\
		"[mfactor_span_Min] INTEGER  NULL, \r\n"\
		"[mfactor_span_Max] INTEGER  NULL, \r\n"\
		"[mfactor_fix_Min] INTEGER  NULL, \r\n"\
		"[mfactor_fix_Max] INTEGER  NULL, \r\n"\
		"[mfactor_spanWarn] INTEGER  NULL, \r\n"\
		"[Cam_Info1] INTEGER  NULL, \r\n"\
		"[Cam_Info2] INTEGER  NULL, \r\n"\
		"[Audio_Source] INTEGER  NULL, \r\n"\
		"[Audio_FmtType] INTEGER  NULL, \r\n"\
		"[Audio_ParamDef] INTEGER  NULL, \r\n"\
		"[Audio_Channels] INTEGER  NULL, \r\n"\
		"[Audio_bitSize] INTEGER  NULL, \r\n"\
		"[Audio_SampleRate] INTEGER  NULL, \r\n"\
		"[Audio_ByteRate] INTEGER  NULL, \r\n"\
		"[Audio_cap_src] VARCHAR(255)  NULL, \r\n"\
		"[Audio_cap_param] VARCHAR(255)  NULL);" );

	//
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "INSERT INTO devices_ipcamera" );
	db->exec( "INSERT INTO devices_ipcamera("\
		"[id],[device_id],[module_type],[name],[ver],[ipaddress],[port],[username],[password],[channel],[streamfmt],"\
		"[ptz],[focal],[focal_min],[focal_max],[focal_zoom],[enable],[buffertime],"\
		"[rec_mod],[rec_svrtype],[rec_ipaddress],[rec_port],[rec_username],[rec_password],[rec_channel],"\
		"[FixTSMod],[span_Min],[span_Max],[fix_Min],[fix_Max],[spanWarn],[mfactor_span_Min],[mfactor_span_Max],[mfactor_fix_Min],[mfactor_fix_Max],[mfactor_spanWarn] "\
		")"\
		"SELECT "\
		"[id],[device_id],[module_type],[name],[ver],[ipaddress],[port],[username],[password],[channel],[streamfmt],"\
		"[ptz],[focal],[focal_min],[focal_max],[focal_zoom],[enable],[buffertime],"\
		"[rec_mod],[rec_svrtype],[rec_ipaddress],[rec_port],[rec_username],[rec_password],[rec_channel],"\
		"[FixTSMod],[span_Min],[span_Max],[fix_Min],[fix_Max],[spanWarn],[mfactor_span_Min],[mfactor_span_Max],[mfactor_fix_Min],[mfactor_fix_Max],[mfactor_spanWarn] "\
		" FROM devices_ipcamera_bak;" );

	//
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "DROP TABLE devices_ipcamera_bak" );
	db->exec( "DROP TABLE devices_ipcamera_bak;" );

	//LOGMSGEX( defLOGNAME, defLOG_SYS, "doDBUpdate_devices_ipcamera End\r\n" );
}

static bool doDBUpdate_to14032500( SQLite::Database *db, const int cur_db_ver )
{
	const int this_DBVER = 14032500; // 此函数升级完成后的数据库版本

	// 旧版到此版本的变化
	//--
	// 1: add table event_call
	// 2: devices_ipcamera 增加音频字段
	//--

	if( cur_db_ver >= this_DBVER )
	{
		return true;
	}

	try
	{
		doDBCreate_event_call( db );
		doDBUpdate_devices_ipcamera( db );

		doDBUpdate_DBVer( db, this_DBVER );
	}
	catch(...)
	{
		return false;
	}

	return true;
}

static void doDBCreate_dev_relation( SQLite::Database *db )
{
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "doDBCreate_dev_relation...\r\n" );

	db->exec( "CREATE TABLE [dev_relation] ( \r\n"\
		"[id] INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL, \r\n"\
		"[enable] INTEGER  NULL, \r\n"\
		"[device_type] INTEGER  NULL, \r\n"\
		"[device_id] INTEGER  NULL, \r\n"\
		"[sort_no] INTEGER  NULL, \r\n"\
		"[child_dev_type] INTEGER  NULL, \r\n"\
		"[child_dev_id] INTEGER  NULL, \r\n"\
		"[child_dev_subid] INTEGER  NULL, \r\n"\
		"[flag] INTEGER  NULL)" );
}

static void doDBCreate_dev_preset( SQLite::Database *db )
{
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "doDBCreate_dev_preset...\r\n" );

	db->exec( "CREATE TABLE [dev_preset] ( \r\n"\
		"[id] INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL, \r\n"\
		"[device_type] INTEGER  NULL, \r\n"\
		"[device_id] INTEGER  NULL, \r\n"\
		"[sort_no] INTEGER  NULL, \r\n"\
		"[preset_index] INTEGER  NULL, \r\n"\
		"[enable] INTEGER  NULL, \r\n"\
		"[param1] INTEGER  NULL, \r\n"\
		"[param2] INTEGER  NULL, \r\n"\
		"[name] VARCHAR(255)  NULL)" );
}

static void doDBCreate_comm_link( SQLite::Database *db )
{
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "doDBCreate_comm_link...\r\n" );

	db->exec( "CREATE TABLE [comm_link] ( \r\n"\
		"[id] INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL, \r\n"\
		"[name] VARCHAR(255)  NULL, \r\n"\
		"[enable] INTEGER  NULL, \r\n"\
		"[link_type] INTEGER  NULL, \r\n"\
		"[trans_mod] INTEGER  NULL, \r\n"\
		"[heartbeat_type] INTEGER  NULL, \r\n"\
		"[heartbeat_param] VARCHAR(255)  NULL, \r\n"\
		"[param1] INTEGER  NULL, \r\n"\
		"[param2] INTEGER  NULL, \r\n"\
		"[param3] INTEGER  NULL, \r\n"\
		"[param4] INTEGER  NULL, \r\n"\
		"[param5] INTEGER  NULL, \r\n"\
		"[param6] INTEGER  NULL, \r\n"\
		"[param_str1] VARCHAR(255)  NULL, \r\n"\
		"[param_str2] VARCHAR(255)  NULL)" );
}

static void doDBCreate_comm_link_relation( SQLite::Database *db )
{
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "doDBCreate_comm_link_relation...\r\n" );

	db->exec( "CREATE TABLE [comm_link_relation] ( \r\n"\
		"[id] INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL, \r\n"\
		"[device_type] INTEGER  NULL, \r\n"\
		"[device_id] INTEGER  NULL, \r\n"\
		"[link_id] INTEGER  NULL, \r\n"\
		"[link_id_bak] INTEGER  NULL, \r\n"\
		"[param1] INTEGER  NULL, \r\n"\
		"[param2] INTEGER  NULL, \r\n"\
		"[param3] INTEGER  NULL, \r\n"\
		"[param4] INTEGER  NULL)" );
}

static void doDBCreate_vobj_config( SQLite::Database *db )
{
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "doDBCreate_vobj_config...\r\n" );

	db->exec( "CREATE TABLE [vobj_config] ( \r\n"\
		"[id] INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL, \r\n"\
		"[enable] INTEGER  NULL, \r\n"\
		"[vobj_type] INTEGER  NULL, \r\n"\
		"[vobj_flag] INTEGER  NULL, \r\n"\
		"[name] VARCHAR(255)  NULL, \r\n"\
		"[ver] VARCHAR(64)  NULL, \r\n"\
		"[param1] INTEGER  NULL, \r\n"\
		"[param2] INTEGER  NULL, \r\n"\
		"[param3] INTEGER  NULL, \r\n"\
		"[param4] INTEGER  NULL, \r\n"\
		"[param5] INTEGER  NULL, \r\n"\
		"[param6] INTEGER  NULL, \r\n"\
		"[param_str1] VARCHAR(255)  NULL, \r\n"\
		"[param_str2] VARCHAR(255)  NULL)" );
}

static bool doDBUpdate_to14061700( SQLite::Database *db, const int cur_db_ver )
{
	const int this_DBVER = 14061700; // 此函数升级完成后的数据库版本

	// 旧版到此版本的变化
	//--
	// 1: add table dev_relation
	//--

	if( cur_db_ver >= this_DBVER )
	{
		return true;
	}

	try
	{
		doDBCreate_dev_relation( db );

		doDBUpdate_DBVer( db, this_DBVER );
	}
	catch(...)
	{
		return false;
	}

	return true;
}

static bool doDBUpdate_to15030900( SQLite::Database *db, const int cur_db_ver )
{
	const int this_DBVER = 15030900; // 此函数升级完成后的数据库版本

	// 旧版到此版本的变化
	//--
	// 1: add table dev_preset
	//--

	if( cur_db_ver >= this_DBVER )
	{
		return true;
	}

	try
	{
		doDBCreate_dev_preset( db );

		doDBUpdate_DBVer( db, this_DBVER );
	}
	catch(...)
	{
		return false;
	}

	return true;
}

static void doDBUpdate_code_str_val( SQLite::Database *db )
{
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "doDBCreate_event_call...\r\n" );

	db->exec( "ALTER TABLE `code` ADD [str_val] VARCHAR(128)  NULL;" );
}

static bool doDBUpdate_to15060800( SQLite::Database *db, const int cur_db_ver )
{
	const int this_DBVER = 15060800; // 此函数升级完成后的数据库版本

	// 旧版到此版本的变化
	//--
	// 1: add table comm_link
	// 2: add table comm_link_relation
	// 3: add table vobj_config
	// 4: Update table code
	//--

	if( cur_db_ver >= this_DBVER )
	{
		return true;
	}

	try
	{
		doDBCreate_comm_link( db );
		doDBCreate_comm_link_relation( db );
		doDBCreate_vobj_config( db );
		doDBUpdate_code_str_val( db );

		doDBUpdate_DBVer( db, this_DBVER );
	}
	catch(...)
	{
		return false;
	}

	return true;
}

// 数据库升级
bool g_doDBUpdate( SQLite::Database *db )
{
	const int cur_db_ver = g_GetDBVer(db);

	//LOGMSGEX( defLOGNAME, defLOG_SYS, "doDBUpdate run..." );

	// 太旧的不能升级
	const int db_ver_min = 13112100;
	if( cur_db_ver < db_ver_min )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "cur db_ver=%d, db can't update to %d! need >= db_ver_min.", cur_db_ver, GSIOT_DBVER );

		return false;
	}

	if( cur_db_ver >= GSIOT_DBVER )
	{
		//LOGMSGEX( defLOGNAME, defLOG_SYS, "db is new, need not update." );

		return true;
	}

	// 开始升级
	//LOGMSGEX( defLOGWatch, defLOG_SYS, "数据库升级\r\n" );
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "doDBUpdate Begin. cur db_ver=%d, update to %d...", cur_db_ver, GSIOT_DBVER );
	UseDbTransAction dbta( db );

	if( !doDBUpdate_to14032500( db, cur_db_ver ) )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "doDBUpdate_to14032500 failed!" );
		return false;
	}

	if( !doDBUpdate_to14061700( db, cur_db_ver ) )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "doDBUpdate_to14061700 failed!" );
		return false;
	}

	if( !doDBUpdate_to15030900( db, cur_db_ver ) )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "doDBUpdate_to15030900 failed!" );
		return false;
	}

	if( !doDBUpdate_to15060800( db, cur_db_ver ) )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "doDBUpdate_to15060800 failed!" );
		return false;
	}

	// 升级完成
	dbta.Commit();
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "doDBUpdate End.\r\n" );

	return true;
}
