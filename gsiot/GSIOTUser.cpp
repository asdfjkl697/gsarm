#include "GSIOTUser.h"
#include "RunCode.h"
//#include "logFileExFunc.h"
#include "GSIOTDevice.h"
#include "gloox/util.h"
#include "common.h"

#define defXmppUser_ResultSucceed	defGSReturnStr_Succeed
#define defXmppUser_ResultFail		defGSReturnStr_Fail

static char *s_defUserFlag_names[] =
{
	//"is added to notice group",
	//""
};

bool operator< ( const struAuth_key &key1, const struAuth_key &key2 )
{
	if( key1.device_type != key2.device_type )
		return (key1.device_type < key2.device_type);

	return (key1.device_id < key2.device_id);
}

/////////////////////

std::string struAuth::Print( const char *info, bool doPrint ) const
{
	char buf[256] = {0};

	snprintf( buf, sizeof(buf), "AuthDev(%s) devtype=%d, devid=%d, auth=%d", info?info:"", this->device_type, this->device_id, this->authority );

	if( doPrint )
	{
		//LOGMSG( buf );
	}

	return std::string(buf);
}

/////////////////////////////////////////////////

bool CUserFlag::isShow_UserFlag( defUserFlag flag )
{
	switch(flag)
	{
	case defUserFlag_Max_:
		return false;

	default:
		break;
	}

	return true;
}

const char* CUserFlag::get_UserFlag_Name( defUserFlag flag )
{
	if( flag <defUserFlag_Min_ || flag >=defUserFlag_Max_ )
	{
		return s_defUserFlag_names[defUserFlag_Max_];
	}

	return s_defUserFlag_names[flag];
}

bool CUserFlag::get_UserFlag( defUserFlag flag ) const
{
	if( flag<0 || (size_t)flag>=m_UserFlag.size() )
	{
		assert(false);
		return false;
	}

	return m_UserFlag.test( flag );
}

bool CUserFlag::set_UserFlag( defUserFlag flag, bool val )
{
	if( flag<0 || (size_t)flag>=m_UserFlag.size() )
	{
		assert(false);
		return false;
	}

	m_UserFlag.set( flag, val );
	return true;
}

uint32_t CUserFlag::get_UserFlag_uintfull() const
{
	return m_UserFlag.to_ulong();
}

bool CUserFlag::set_UserFlag_uintfull( uint32_t fullval )
{
	std::bitset<defUserFlag_Max_> flagfv((_ULonglong)fullval);
	m_UserFlag = flagfv;
	return true;
}

/////////////////////////////////////////////////

GSIOTUser::GSIOTUser()
{
	Reset();
}

GSIOTUser::GSIOTUser( const Tag* tag )
{
	Reset();
	if( !tag )
		return;

	SetValidAttribute( defAttr_all, false );

	if( tag->name() != "user" )
		return;

	if(tag->hasAttribute("jid"))
	{
		this->m_jid = tag->findAttribute("jid");
	}

	if(tag->hasAttribute("name"))
	{
		this->m_name = UTF8ToASCII(tag->findAttribute("name"));
		SetValidAttribute( defAttr_name );
	}

	if(tag->hasAttribute("enable"))
	{
		this->m_enable = atoi(tag->findAttribute("enable").c_str());
		SetValidAttribute( defAttr_enable );
	}

	if(tag->hasAttribute("level"))
	{
		this->SetUserLevel( atoi(tag->findAttribute("level").c_str()) );
		SetValidAttribute( defAttr_UserLevel );
	}

	if(tag->hasAttribute("noticegroup"))
	{
		this->set_UserFlag( defUserFlag_NoticeGroup, (bool)atoi(tag->findAttribute("noticegroup").c_str()) );
		SetValidAttribute( defAttr_NoticeGroup );
	}

	const TagList& l = tag->children();
	TagList::const_iterator it = l.begin();
	TagList::const_iterator itEnd = l.end();
	for( ; it != itEnd; ++it )
	{
		UntagChild( *it );
	}
}

GSIOTUser::~GSIOTUser(void)
{
	m_mapAuth.clear();
}

std::string GSIOTUser::GetKeyJid_spec( const std::string &jid )
{
	std::string KeyJid = jid;
	g_toLowerCase(KeyJid);
	return KeyJid;
}

std::string GSIOTUser::getstr_UserLevel( const int UserLevel )
{
	if( UserLevel >= defUserLevel_MaxCfgLevel )
	{
		return std::string("最高级别");
	}

	char buf[32] = {0};
	snprintf( buf, sizeof(buf), "%d%s", UserLevel, defUserLevel_Default==UserLevel?" (默认)":(defUserLevel_Guest==UserLevel?" (最低)":"") );

	return std::string(buf);
}

void GSIOTUser::FixJid_spec( std::string &jid )
{
	if( jid.empty() )
		return;

	if( std::string::npos == jid.find( "@" ) )
	{
		jid += XMPP_GSIOTUser_DefaultDomain;
	}
}

bool GSIOTUser::isSame( const std::string &jidA, const std::string &jidB )
{
	return ( !jidA.empty() && GetKeyJid_spec(jidA) == GetKeyJid_spec(jidB) );
}

// 获取权限信息指针
const struAuth* GSIOTUser::Get_Auth_spec( const defmapAuth &mapAuth, const IOTDeviceType type, const int id )
{
	defmapAuth::const_iterator it = mapAuth.find( struAuth_key(type,id) );
	if( it!=mapAuth.end() )
	{
		return &(it->second);
	}

	return NULL;
}

// _my的权限在_other里是否都有并且相同，也就是_other包含_my的权限范围
bool GSIOTUser::mapAuthCampareMyOnOther( const defmapAuth &mapAuth_my, const defmapAuth &mapAuth_other, const bool cmpDevice, const bool cmpModule )
{
	for( defmapAuth::const_iterator it=mapAuth_my.begin(); it!=mapAuth_my.end(); ++it )
	{
		const struAuth &this_Auth = it->second;

		// 无权限的不用判断是否相同
		if( defUserAuth_Null!=this_Auth.authority )
		{
			const struAuth* other_auth = Get_Auth_spec( mapAuth_other, this_Auth.device_type, this_Auth.device_id );
			if( !other_auth )
				return false;

			if( IsMoudle(this_Auth.device_type) )
			{
				// 不是所有权限设置的需要进一步判断
				if( cmpModule )
				{
					if( this_Auth.authority != other_auth->authority )
						return false;
				}
			}
			else
			{
				// 不是所有权限设置的需要进一步判断
				if( cmpDevice )
				{
					if( this_Auth.authority != other_auth->authority )
						return false;
				}
			}
		}
	}

	return true;
}

GSIOTUser* GSIOTUser::clone() const
{
	return new GSIOTUser( *this );
}

void GSIOTUser::FixJid()
{
	FixJid_spec( this->m_jid );
}

void GSIOTUser::FixName()
{
	if( this->m_jid.empty() )
		return;

	if( !this->m_name.empty() )
		return;

	const std::string::size_type pos = this->m_jid.find( '@' );
	if( pos != std::string::npos )
	{
		this->SetName( this->m_jid.substr( 0, pos ) );
	}
	else
	{
		this->SetName( this->m_jid );
	}
}

std::string GSIOTUser::GetKeyJid() const
{
	return GetKeyJid_spec( this->GetJid() );
}

bool GSIOTUser::isMe( const std::string &in_jid ) const
{
	return this->isSame( this->GetJid(), in_jid );
}

void GSIOTUser::UntagChild( const Tag* tag )
{
	const std::string& name = tag->name();

	if( name == "device" )
	{
		struAuth auth;
		if(tag->hasAttribute("type"))
			auth.device_type = (IOTDeviceType)atoi(tag->findAttribute("type").c_str());
		else
			return;

		if(tag->hasAttribute("id"))
			auth.device_id = atoi(tag->findAttribute("id").c_str());
		else
			return;

		if(tag->hasAttribute("authority"))
			auth.authority = (defUserAuth)atoi(tag->findAttribute("authority").c_str());
		else
			return;

		this->m_mapAuth[struAuth_key(auth)] = auth;

	}
	else if( name == "module" )
	{
		struAuth auth;
		if(tag->hasAttribute("type"))
			auth.device_type = GSIOTUser::ConvertStringToType( tag->findAttribute("type") );
		else
			return;

		auth.device_id = defAuth_ModuleDefaultID;

		if(tag->hasAttribute("authority"))
			auth.authority = (defUserAuth)atoi(tag->findAttribute("authority").c_str());
		else
			return;

		this->m_mapAuth[struAuth_key(auth)] = auth;
	}
	//else if( name == "result" )
	//{
	//	const std::string strResult = tag->cdata();
	//	if( strResult == defXmppUser_ResultSucceed )
	//	{
	//		m_result = defGSReturn_Success;
	//	}
	//	else if( strResult == defXmppUser_ResultFail )
	//	{
	//		m_result = defGSReturn_Err;
	//	}
	//	else
	//	{
	//		m_result = defGSReturn_Null;
	//	}
	//}
}

Tag* GSIOTUser::tag(const struTagParam &TagParam) const
{
	Tag* i = new Tag( "user" );
	
	i->addAttribute("jid",m_jid);

	if( IsValidAttribute(defAttr_name) )
		i->addAttribute("name",ASCIIToUTF8(this->m_name));

	if( IsValidAttribute(defAttr_enable) )
		i->addAttribute("enable",util::int2string(m_enable));

	if( IsValidAttribute(defAttr_UserLevel) )
		i->addAttribute("level",util::int2string(m_UserLevel));

	if( IsValidAttribute(defAttr_NoticeGroup) )
	{
		const int NoticeGroup = this->get_UserFlag(defUserFlag_NoticeGroup);
		if( NoticeGroup ) i->addAttribute("noticegroup",util::int2string(NoticeGroup));
	}

#if 0
	Tag* iOverview = new Tag( i, "overview" );
	iOverview->addAttribute("usertype",m_AuthOverview_UserType);
	iOverview->addAttribute("devauth",m_AuthOverview_Device);
	iOverview->addAttribute("modauth",m_AuthOverview_Module);
#else
	Tag* iOverview = new Tag( i, "ov" );
	iOverview->addAttribute("u",Get_AuthOverview_UserType_str(m_AuthOverview_UserType));
	iOverview->addAttribute("d",Get_AuthOverview_Auth_str(m_AuthOverview_Device));
	iOverview->addAttribute("m",Get_AuthOverview_Auth_str(m_AuthOverview_Module));
#endif

	if( TagParam.isValid && TagParam.isResult )
	{
		if( !macGSIsReturnNull(m_result) )
		{
			if( macGSSucceeded(m_result) )
			{
				new Tag( i, "result", std::string(defXmppUser_ResultSucceed) );
			}
			else
			{
				Tag* iRet = new Tag( i, "result", std::string(defXmppUser_ResultFail) );
				iRet->addAttribute("code",m_result);
			}
		}
	}
	else
	{
		for( defmapAuth::const_iterator it=m_mapAuth.begin(); it!=m_mapAuth.end(); ++it )
		{
			const struAuth &Auth = it->second;

			i->addChild( this->tagChild( Auth ) );
		}
	}

	return i;
}

Tag* GSIOTUser::tagChild( const struAuth &Auth ) const
{
	Tag* i = NULL;

	if( IsMoudle( Auth.device_type ) )
	{
		i = new Tag( "module" );
		i->addAttribute("type",ConvertTypeToString(Auth.device_type));
		i->addAttribute("authority",(int)Auth.authority);
	}
	else
	{
		i = new Tag( "device" );
		i->addAttribute("type",(int)Auth.device_type);
		i->addAttribute("id",(int)Auth.device_id);
		i->addAttribute("authority",(int)Auth.authority);
	}

	return i;
}

// 属性成员是否有效
bool GSIOTUser::IsValidAttribute( const GSIOTUser::defAttr attr ) const
{
	if( defAttr_all == attr )
	{
		return m_validAttribute.all();
	}

	if( attr<0 || (size_t)attr>=m_validAttribute.size() )
	{
		assert(false);
		return false;
	}

	return m_validAttribute.test( attr );
}

// 设置属性成员有效性
void GSIOTUser::SetValidAttribute( const GSIOTUser::defAttr attr, bool valid )
{
	if( defAttr_all == attr )
	{
		if( valid )
		{
			m_validAttribute.set();
		}
		else
		{
			m_validAttribute.reset();
		}

		return ;
	}

	if( attr<0 || (size_t)attr>=m_validAttribute.size() )
	{
		assert(false);
		return ;
	}

	m_validAttribute.set( attr, valid );
}

// curAuth:		目前拥有的权限
// needAuth:	操作需要的权限
bool GSIOTUser::JudgeAuth( const defUserAuth curAuth, const defUserAuth needAuth )
{
	//return ( curAuth >= needAuth );

	switch(needAuth)
	{
	case defUserAuth_Null:
		return true;

	case defUserAuth_RO:
		{
			if( defUserAuth_RO==curAuth
				|| defUserAuth_RW==curAuth )
			{
				return true;
			}
		}
		break;

	case defUserAuth_WO:
		{
			if( defUserAuth_WO==curAuth
				|| defUserAuth_RW==curAuth )
			{
				return true;
			}
		}
		break;

	case defUserAuth_RW:
		{
			if( defUserAuth_RW==curAuth )
			{
				return true;
			}
		}
		break;
	}

	return false;
}

// 是否为功能模块
bool GSIOTUser::IsMoudle( const IOTDeviceType type )
{
	return ( type>=IOT_Module_Unknown && type<=IOT_Module_all );
}

// 是否为所有权限类的配置信息
bool GSIOTUser::IsAllAuthConfig( const IOTDeviceType type, const int id )
{
	if( defAllAuth_DeviceType == type
		&& defAllAuth_DeviceID == id )
	{
		return true;
	}

	if( defAllAuth_ModuleType == type
		&& defAllAuth_ModuleID == id )
	{
		return true;
	}

	return false;
}

// 枚举值转换为字符串形式
std::string GSIOTUser::ConvertTypeToString( const IOTDeviceType type )
{
	switch(type)
	{
	case IOT_Module_authority:
		return std::string("authority");

	case IOT_Module_record:
		return std::string("record");

	case IOT_Module_system:
		return std::string("system");

	case IOT_Module_manager:
		return std::string("manager");

	case IOT_Module_event:
		return std::string("event");

	case IOT_Module_talk:
		return std::string("talk");
		
	case IOT_Module_reboot:
		return std::string("reboot");

	case IOT_Module_acctl:
		return std::string( "acctl" );

	case IOT_Module_all:
		return std::string("all");

	default:
		return std::string("Unknown");
	}
}

// 字符串形式转换为枚举值
IOTDeviceType GSIOTUser::ConvertStringToType( const std::string &strType )
{
	if( strType == "authority" )
	{
		return IOT_Module_authority;
	}
	else if( strType == "record" )
	{
		return IOT_Module_record;
	}
	else if( strType == "system" )
	{
		return IOT_Module_system;
	}
	else if( strType == "manager" )
	{
		return IOT_Module_manager;
	}
	else if( strType == "event" )
	{
		return IOT_Module_event;
	}
	else if( strType == "talk" )
	{
		return IOT_Module_talk;
	}
	else if( strType == "reboot" )
	{
		return IOT_Module_reboot;
	}
	else if( strType == "acctl" )
	{
		return IOT_Module_acctl;
	}
	else if( strType == "all" )
	{
		return IOT_Module_all;
	}

	return IOT_Module_Unknown;
}

// 转换为界面显示名称
std::string GSIOTUser::ConvertTypeToShowName( const IOTDeviceType type )
{
	switch(type)
	{
	case IOT_Module_authority:
		return std::string("权限管理");

	case IOT_Module_record:
		return std::string("录像回放");

	case IOT_Module_system:
		return std::string("系统管理");

	case IOT_Module_manager:
		return std::string("设备管理");

	case IOT_Module_event:
		return std::string("事件配置");

	case IOT_Module_talk:
		return std::string("远程通话");

	case IOT_Module_reboot:
		return std::string("远程重启");

	case IOT_Module_acctl:
		return std::string( "门禁控制" );

	case IOT_Module_all:
		return std::string("所有功能模块");

	default:
		return std::string("Unknown");
	}
}

// 转换为界面显示名称
std::string GSIOTUser::ConvertAuthToShowName( const defUserAuth auth )
{
	switch(auth)
	{
	case defUserAuth_Null:
		return std::string("  无");//无权限

	case defUserAuth_RO:
		return std::string("只读");

	case defUserAuth_WO:
		return std::string("只写");

	case defUserAuth_RW:
		return std::string("读写");

	default:
		return std::string("Unknown");
	}
}

// 是否没有权限
bool GSIOTUser::IsNullAuth() const
{
	if( !m_mapAuth.empty() )
	{
		for( defmapAuth::const_iterator it=m_mapAuth.begin(); it!=m_mapAuth.end(); ++it )
		{
			const struAuth &Auth = it->second;

			if( Auth.authority>defUserAuth_Null )
			{
				return false;
			}
		}
	}

	return true;
}

// 权限是否相同
bool GSIOTUser::IsSameAuth( const GSIOTUser &other ) const
{
	if( this == &other )
		return true;

	const defAuthOverview_Auth this_All_AuthDevice = this->Get_AuthOverview_Device();
	const defAuthOverview_Auth other_All_AuthDevice = other.Get_AuthOverview_Device();

	if( this_All_AuthDevice != other_All_AuthDevice )
		return false;

	const defAuthOverview_Auth this_All_AuthModule = this->Get_AuthOverview_Module();
	const defAuthOverview_Auth other_All_AuthModule = other.Get_AuthOverview_Module();

	if( this_All_AuthModule != other_All_AuthModule )
		return false;

	// 都是所有权限设置，则判断结束返回
	bool isAllFlag_AuthDevice = false;
	if( defAuthOverview_Auth_RO == this_All_AuthDevice
		|| defAuthOverview_Auth_WO == this_All_AuthDevice
		|| defAuthOverview_Auth_All == this_All_AuthDevice
		)
	{
		isAllFlag_AuthDevice = true;
	}

	bool isAllFlag_AuthModule = false;
	if( defAuthOverview_Auth_RO == this_All_AuthModule
		|| defAuthOverview_Auth_WO == this_All_AuthModule
		|| defAuthOverview_Auth_All == this_All_AuthModule
		)
	{
		isAllFlag_AuthModule = true;
	}

	if( isAllFlag_AuthDevice && isAllFlag_AuthModule )
		return true;

	if( mapAuthCampareMyOnOther( this->m_mapAuth, other.m_mapAuth, !isAllFlag_AuthDevice, !isAllFlag_AuthModule )
		&& mapAuthCampareMyOnOther( other.m_mapAuth, this->m_mapAuth, !isAllFlag_AuthDevice, !isAllFlag_AuthModule )
		)
	{
		return true;
	}

	return false;
}

// 权限是否相近
bool GSIOTUser::IsNearAuth( const GSIOTUser &other ) const
{
	if( this == &other )
		return true;

	if( this->Get_AuthOverview_Device() != other.Get_AuthOverview_Device() )
		return false;

	if( this->Get_AuthOverview_Module() != other.Get_AuthOverview_Module() )
		return false;

	return true;
}

// 是否拥有所有权限
bool GSIOTUser::IsAll_Auth( defUserAuth needauth ) const
{
	return ( IsAll_AuthDevice( needauth ) && IsAll_AuthModule( needauth ) );
}

// 设置所有权限
void GSIOTUser::SetAll_Auth( defUserAuth auth, bool blLog )
{
	SetAll_AuthDevice( auth, blLog );
	SetAll_AuthModule( auth, blLog );
}

// 当前的配置是否已经以所有权限为准
bool GSIOTUser::IsAllAuthCfg() const
{
	if( GetAll_AuthDevice()>defUserAuth_Null 
		&& GetAll_AuthModule()>defUserAuth_Null )
	{
		return true;
	}

	return false;
}

// 是否拥有所有设备访问权限
bool GSIOTUser::IsAll_AuthDevice( defUserAuth needauth ) const
{
	return GSIOTUser::JudgeAuth( GetAll_AuthDevice(), needauth );
}

// 是否拥有所有功能模块权限
bool GSIOTUser::IsAll_AuthModule( defUserAuth needauth ) const
{
	return GSIOTUser::JudgeAuth( GetAll_AuthModule(), needauth );
}

// 获取 所有设备访问权限 的权限值
defUserAuth GSIOTUser::GetAll_AuthDevice() const
{
	const struAuth *pAuth = Get_Auth( defAllAuth_DeviceType, defAllAuth_DeviceID );
	if( pAuth )
	{
		return pAuth->authority;
	}

	return defUserAuth_Null;
}

// 获取 所有功能模块权限 的权限值
defUserAuth GSIOTUser::GetAll_AuthModule() const
{
	const struAuth *pAuth = Get_Auth( defAllAuth_ModuleType, defAllAuth_ModuleID );
	if( pAuth )
	{
		return pAuth->authority;
	}

	return defUserAuth_Null;
}

// 设置所有设备访问权限
void GSIOTUser::SetAll_AuthDevice( defUserAuth auth, bool blLog )
{
	if( defUserAuth_Null!=auth )
	{
		CfgChange_Auth( struAuth( defAllAuth_DeviceType, defAllAuth_DeviceID, auth ), defCfgOprt_AddModify, true, blLog );
	}
	else
	{
		CfgChange_Auth( struAuth( defAllAuth_DeviceType, defAllAuth_DeviceID, defAllAuth_DeviceAuth_Null ), defCfgOprt_Modify, false, blLog );
	}
}

// 设置所有功能模块权限
void GSIOTUser::SetAll_AuthModule( defUserAuth auth, bool blLog )
{
	if( defUserAuth_Null!=auth )
	{
		CfgChange_Auth( struAuth( defAllAuth_ModuleType, defAllAuth_ModuleID, auth ), defCfgOprt_AddModify, true, blLog );
	}
	else
	{
		CfgChange_Auth( struAuth( defAllAuth_ModuleType, defAllAuth_ModuleID, defAllAuth_ModuleAuth_Null ), defCfgOprt_Modify, false, blLog );
	}
}

// 获取权限信息指针
const struAuth* GSIOTUser::Get_Auth( const IOTDeviceType type, const int id ) const
{
	return Get_Auth_spec( m_mapAuth, type, id );
	//defmapAuth::const_iterator it = m_mapAuth.find( struAuth_key(type,id) );
	//if( it!=m_mapAuth.end() )
	//{
	//	return &(it->second);
	//}

	//return NULL;
}

// 配置变更
bool GSIOTUser::CfgChange_Auth( const struAuth &auth, const defCfgOprt_ oprt, const bool FailedIsErr, bool blLog )
{
	switch( oprt )
	{
	case defCfgOprt_Add:
	case defCfgOprt_Modify:
	case defCfgOprt_AddModify:
		break;

	case defCfgOprt_Delete:
		{
			return Delete_Auth( auth.device_type, auth.device_id, FailedIsErr );
		}

	default:
		assert(false);
		return false;
	}

	struAuth_key k(auth);
	defmapAuth::iterator it = m_mapAuth.find( k );
	if( it!=m_mapAuth.end() )
	{
		if( defCfgOprt_Add==oprt )
		{
			//if( blLog ) LOGMSGEX( defLOGNAME, FailedIsErr?defLOG_ERROR:defLOG_INFO, "CfgChange_Auth(oprt=%d) err, isbeing! %s\r\n", oprt, auth.Print( this->m_jid.c_str(), false ).c_str() );
			return false;
		}

		// modify or addmodify
		if( blLog ) LOGMSG( "CfgChange_Auth(oprt=%d) isbeing, do modify, %s\r\n", oprt, auth.Print( this->m_jid.c_str(), false ).c_str() );

		it->second = auth;
	}
	else
	{
		if( defCfgOprt_Modify==oprt )
		{
			//if( blLog && defAllAuth_DeviceAuth_Null != auth.authority ) LOGMSGEX( defLOGNAME, FailedIsErr?defLOG_ERROR:defLOG_INFO, "CfgChange_Auth(oprt=%d) err, not found! %s\r\n", oprt, auth.Print( this->m_jid.c_str(), false ).c_str() );
			return false;
		}

		// add or addmodify
		if( blLog ) LOGMSG( "CfgChange_Auth(oprt=%d) do add, %s\r\n", oprt, auth.Print( this->m_jid.c_str(), false ).c_str() );

		m_mapAuth[k] = auth;
	}

	return true;
}

bool GSIOTUser::Delete_Auth( const IOTDeviceType type, const int id, const bool FailedIsErr )
{
	defmapAuth::iterator it = m_mapAuth.find( struAuth_key(type,id) );
	if( it!=m_mapAuth.end() )
	{
		LOGMSG( "Delete_Auth type=%d, id=%d\r\n", type, id );

		m_mapAuth.erase(it);
		return true;
	}

	//LOGMSGEX( defLOGNAME, FailedIsErr?defLOG_ERROR:defLOG_INFO, "Delete_Auth err, not found! type=%d, id=%d\r\n", type, id );
	return false;
}

void GSIOTUser::Reset()
{
	m_id = 0;
	m_enable = defDeviceEnable;
	m_UserLevel = 0;	// 0为默认用户级别
	m_jid = "";
	m_name = "";
	this->set_UserFlag_uintfull( 0 );
	m_result = defGSReturn_Null;

	// 默认所有属性有效
	SetValidAttribute( defAttr_all );

	m_AuthOverview_UserType = defAuthOverview_UserType_User;

	ResetOnlyAuth();
}

// allauth=true：清除权限设置，包括所有权限
// allauth=false：清除单个权限设置，保留所有设备访问权限、所有功能模块权限等
void GSIOTUser::ResetOnlyAuth( bool allauth, bool reset_overview )
{
	if( allauth )
	{
		m_mapAuth.clear();
	}
	else
	{
		const defUserAuth authAll_AuthDevice = GetAll_AuthDevice();
		const defUserAuth authAll_AuthModule = GetAll_AuthModule();

		m_mapAuth.clear();

		{
			struAuth auth(defAllAuth_DeviceType,defAllAuth_DeviceID,authAll_AuthDevice);
			this->m_mapAuth[struAuth_key(auth)] = auth;
		}
		{
			struAuth auth(defAllAuth_ModuleType,defAllAuth_ModuleID,authAll_AuthModule);
			this->m_mapAuth[struAuth_key(auth)] = auth;
		}
	}

	if( reset_overview )
	{
		m_AuthOverview_Device = defAuthOverview_Auth_Null;
		m_AuthOverview_Module = defAuthOverview_Auth_Null;
	}
}

// 移除冗余的、判断逻辑中不需要用到的信息
void GSIOTUser::RemoveUnused( bool onlyRemoveNull )
{
	const defUserAuth authAll_AuthDevice = GetAll_AuthDevice();
	const defUserAuth authAll_AuthModule = GetAll_AuthModule();

	if( !onlyRemoveNull && authAll_AuthDevice!=defUserAuth_Null && authAll_AuthModule!=defUserAuth_Null )
	{
		this->ResetOnlyAuth(false,false);
	}
	else
	{
		defmapAuth tempmapAuth;
		tempmapAuth.swap( m_mapAuth );
		m_mapAuth.clear();

		{
			struAuth auth(defAllAuth_DeviceType,defAllAuth_DeviceID,authAll_AuthDevice);
			this->m_mapAuth[struAuth_key(auth)] = auth;
		}
		{
			struAuth auth(defAllAuth_ModuleType,defAllAuth_ModuleID,authAll_AuthModule);
			this->m_mapAuth[struAuth_key(auth)] = auth;
		}

		for( defmapAuth::const_iterator it=tempmapAuth.begin(); it!=tempmapAuth.end(); ++it )
		{
			const struAuth &Auth = it->second;
			
			if( defUserAuth_Null!=Auth.authority )
			{
				if( IsMoudle(Auth.device_type) )
				{
					if( onlyRemoveNull || defUserAuth_Null==authAll_AuthModule )
					{
						this->m_mapAuth[struAuth_key(Auth)] = Auth;
					}
				}
				else
				{
					if( onlyRemoveNull || defUserAuth_Null==authAll_AuthDevice )
					{
						this->m_mapAuth[struAuth_key(Auth)] = Auth;
					}
				}
			}
		}

		tempmapAuth.clear();
	}
}

// 覆盖添加
void GSIOTUser::AddReplace( const GSIOTUser *pUserSrc, bool NoAdd )
{
	this->m_id = pUserSrc->m_id;
	this->m_enable = pUserSrc->m_enable;
	this->m_UserLevel = pUserSrc->m_UserLevel;
	this->m_jid = pUserSrc->m_jid;
	this->m_name = pUserSrc->m_name;
	this->set_UserFlag_uintfull( pUserSrc->get_UserFlag_uintfull() );

	AddReplaceList( pUserSrc->m_mapAuth, NoAdd );
}

void GSIOTUser::AddReplaceList( const defmapAuth &mapAuth, bool NoAdd )
{
	for( defmapAuth::const_iterator it=mapAuth.begin(); it!=mapAuth.end(); ++it )
	{
		const struAuth &AuthSrc = it->second;

		struAuth_key k(AuthSrc);
		defmapAuth::iterator itCur = m_mapAuth.find( k );
		if( itCur!=m_mapAuth.end() )
		{
			itCur->second.authority = it->second.authority;
		}
		else
		{
			if( !NoAdd )
			{
				this->m_mapAuth[k] = AuthSrc;
			}
		}
	}
}

#define macAddAllModuleOne(ModuleType,authority) \
{ \
	struAuth auth(ModuleType,defAuth_ModuleDefaultID,authority); \
	this->m_mapAuth[struAuth_key(auth)] = auth; \
}

void GSIOTUser::InitCfgContent( defUserAuth authority )
{
	macAddAllModuleOne( IOT_Module_authority, authority );
	macAddAllModuleOne( IOT_Module_record, authority );
	macAddAllModuleOne( IOT_Module_system, authority );
	macAddAllModuleOne( IOT_Module_manager, authority );
	macAddAllModuleOne( IOT_Module_event, authority );
	macAddAllModuleOne( IOT_Module_talk, authority );
	macAddAllModuleOne( IOT_Module_reboot, authority );
	macAddAllModuleOne( IOT_Module_acctl, authority );

	{
		struAuth auth(defAllAuth_ModuleType,defAllAuth_ModuleID,authority);
		this->m_mapAuth[struAuth_key(auth)] = auth;
	}

	{
		struAuth auth(defAllAuth_DeviceType,defAllAuth_DeviceID,authority);
		this->m_mapAuth[struAuth_key(auth)] = auth;
	}
}

void GSIOTUser::SetDefaultAuth( int level )
{
	SetDefaultAuth_AuthDevice( level );
	SetDefaultAuth_AuthModule( level );
}

void GSIOTUser::SetDefaultAuth_AuthDevice( int level )
{
	struAuth auth(defAllAuth_DeviceType,defAllAuth_DeviceID,level?defUserAuth_RW:defUserAuth_RO);
	this->m_mapAuth[struAuth_key(auth)] = auth;
}

void GSIOTUser::SetDefaultAuth_AuthModule( int level )
{
	//macAddAllModuleOne( IOT_Module_authority, level?defUserAuth_RW:defUserAuth_Null );
	//macAddAllModuleOne( IOT_Module_record, level?defUserAuth_RW:defUserAuth_RO );
	//macAddAllModuleOne( IOT_Module_system, level?defUserAuth_RW:defUserAuth_RO );
	//macAddAllModuleOne( IOT_Module_manager, level?defUserAuth_RW:defUserAuth_RO );
	//macAddAllModuleOne( IOT_Module_event, level?defUserAuth_RW:defUserAuth_RO );
	//macAddAllModuleOne( IOT_Module_talk, level?defUserAuth_RW:defUserAuth_RO );
	//macAddAllModuleOne( IOT_Module_reboot, level?defUserAuth_RW:defUserAuth_Null );
	//macAddAllModuleOne( IOT_Module_acctl, level?defUserAuth_RW:defUserAuth_Null );

	{
		struAuth auth(defAllAuth_ModuleType,defAllAuth_ModuleID,level?defUserAuth_RW:defUserAuth_RO);
		this->m_mapAuth[struAuth_key(auth)] = auth;
	}
}

// 刷新用户权限信息概览
void GSIOTUser::Refresh_AuthOverview_Auth()
{
	// 已禁用的用户，权限概览是无权限
	if( !this->GetEnable() )
	{
		m_AuthOverview_Device = defAuthOverview_Auth_Null;
		m_AuthOverview_Module = defAuthOverview_Auth_Null;
		return;
	}

	// 所有权限标志
	const defUserAuth authAll_AuthDevice = GetAll_AuthDevice();
	const defUserAuth authAll_AuthModule = GetAll_AuthModule();

	// 如果有一种是自定义权限方式，则遍历计算是否存在部分权限
	defUserAuth part_AuthDevice = defUserAuth_Null;
	defUserAuth part_AuthModule = defUserAuth_Null;
	if( defUserAuth_Null==authAll_AuthDevice || defUserAuth_Null==authAll_AuthModule )
	{
		for( defmapAuth::const_iterator it=m_mapAuth.begin(); it!=m_mapAuth.end(); ++it )
		{
			const struAuth &Auth = it->second;

			if( defUserAuth_Null!=Auth.authority )
			{
				if( IsMoudle(Auth.device_type) )
				{
					if( Auth.authority > part_AuthModule ) part_AuthModule = Auth.authority;
				}
				else
				{
					if( Auth.authority > part_AuthDevice ) part_AuthDevice = Auth.authority;
				}

				if( part_AuthModule>=defUserAuth_WO && part_AuthDevice>=defUserAuth_WO )
				{
					break;
				}
			}
		}
	}

	// 设备类权限概览
	switch( authAll_AuthDevice )
	{
	case defUserAuth_Null:
		{
			if( defUserAuth_Null==part_AuthDevice )
			{
				m_AuthOverview_Device = defAuthOverview_Auth_Null;
			}
			else if( defUserAuth_WO==part_AuthDevice || defUserAuth_RW==part_AuthDevice )
			{
				m_AuthOverview_Device = defAuthOverview_Auth_PartW;
			}
			else
			{
				m_AuthOverview_Device = defAuthOverview_Auth_PartRO;
			}
		}
		break;

	case defUserAuth_RO:
		m_AuthOverview_Device = defAuthOverview_Auth_RO;
		break;

	case defUserAuth_WO:
		m_AuthOverview_Device = defAuthOverview_Auth_WO;
		break;

	case defUserAuth_RW:
		m_AuthOverview_Device = defAuthOverview_Auth_All;
		break;

	default:
		m_AuthOverview_Device = defAuthOverview_Auth_Null;
	}

	// 模块类权限概览
	switch( authAll_AuthModule )
	{
	case defUserAuth_Null:
		{
			if( defUserAuth_Null==part_AuthModule )
			{
				m_AuthOverview_Module = defAuthOverview_Auth_Null;
			}
			else if( defUserAuth_WO==part_AuthModule || defUserAuth_RW==part_AuthModule )
			{
				m_AuthOverview_Module = defAuthOverview_Auth_PartW;
			}
			else
			{
				m_AuthOverview_Module = defAuthOverview_Auth_PartRO;
			}
		}
		break;

	case defUserAuth_RO:
		m_AuthOverview_Module = defAuthOverview_Auth_RO;
		break;

	case defUserAuth_WO:
		m_AuthOverview_Module = defAuthOverview_Auth_WO;
		break;

	case defUserAuth_RW:
		m_AuthOverview_Module = defAuthOverview_Auth_All;
		break;

	default:
		m_AuthOverview_Module = defAuthOverview_Auth_Null;
	}
}

std::string GSIOTUser::Get_AuthOverview_Auth_str( const defAuthOverview_Auth type )
{
	switch(type)
	{
	case defAuthOverview_Auth_Null:
		return std::string("nu");

	case defAuthOverview_Auth_RO:
		return std::string("ro");

	case defAuthOverview_Auth_WO:
		return std::string("wo");

	case defAuthOverview_Auth_All:
		return std::string("al");

	case defAuthOverview_Auth_PartRO:
		return std::string("pr");

	case defAuthOverview_Auth_PartW:
		return std::string("pw");

	default:
		break;
	}

	return std::string("un");
}

std::string GSIOTUser::Get_AuthOverview_UserType_str( const defAuthOverview_UserType type )
{
	switch(type)
	{
		case defAuthOverview_UserType_Guest:
			return std::string("gst");

		case defAuthOverview_UserType_User:
			return std::string("usr");

		case defAuthOverview_UserType_AdminType:
			return std::string("adm");

		case defAuthOverview_UserType_SysAdmin:
			return std::string("sadm");

		case defAuthOverview_UserType_Owner:
			return std::string("own");

		default:
			break;
	}

	return std::string("un");
}

std::string GSIOTUser::Get_AuthOverview_Str() const
{
	std::string strAuth;

	if( defAuthOverview_UserType_Owner==this->Get_AuthOverview_UserType() )
	{
		strAuth = "* 设备所有者(超级管理员)，固定拥有所有权限";
	}
	else
	{
		switch( this->Get_AuthOverview_UserType() )
		{
		case defAuthOverview_UserType_AdminType:
			strAuth = "管理员，";
			break;

		case defAuthOverview_UserType_SysAdmin:
			strAuth = "* 系统管理员，";
			break;

		case defAuthOverview_UserType_Guest:
			strAuth = "* 来宾，";
			break;

		default:
			break;
		}

		if( defAuthOverview_Auth_All==this->Get_AuthOverview_Device() && defAuthOverview_Auth_All==this->Get_AuthOverview_Module() )
		{
			strAuth += "拥有所有权限";
		}
		else if( defAuthOverview_Auth_Null==this->Get_AuthOverview_Device() && defAuthOverview_Auth_Null==this->Get_AuthOverview_Module() )
		{
			strAuth += "无权限";
		}
		else
		{
			bool needsep = false;

			//if( defAuthOverview_Auth_PartRO==this->Get_AuthOverview_Device()
			//	&& defAuthOverview_Auth_PartRO==this->Get_AuthOverview_Module()
			//	)
			//{
			//	strAuth += "部分读权限";
			//}
			//else  if( defAuthOverview_Auth_PartW==this->Get_AuthOverview_Device()
			//	&& defAuthOverview_Auth_PartW==this->Get_AuthOverview_Module()
			//	)
			//{
			//	strAuth += "部分写权限";
			//}
			//else
			{
				if( needsep ) strAuth += "，";

				switch( this->Get_AuthOverview_Device() )
				{
				case defAuthOverview_Auth_PartRO:
					strAuth += "部分设备只读";
					break;
				case defAuthOverview_Auth_PartW:
					strAuth += "部分设备可写";
					break;

				case defAuthOverview_Auth_All:
					strAuth += "所有设备访问可读写";
					break;

				case defAuthOverview_Auth_WO:
					strAuth += "所有设备访问只写";
					break;

				case defAuthOverview_Auth_RO:
					strAuth += "所有设备访问只读";
					break;

				default:
					strAuth += "设备访问无权限";
					break;
				}

				switch( this->Get_AuthOverview_Module() )
				{
				case defAuthOverview_Auth_PartRO:
					strAuth += "，部分功能只读";
					break;
				case defAuthOverview_Auth_PartW:
					strAuth += "，部分功能可写";
					break;

				case defAuthOverview_Auth_All:
					strAuth += "，所有功能模块可读写";
					break;

				case defAuthOverview_Auth_WO:
					strAuth += "，所有功能模块只写";
					break;

				case defAuthOverview_Auth_RO:
					strAuth += "，所有功能模块只读";
					break;

				default:
					strAuth += "，功能模块无权限";
					break;
				}
			}
		}
	}

	return strAuth;
}
