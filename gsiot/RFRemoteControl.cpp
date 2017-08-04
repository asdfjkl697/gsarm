#include "RFRemoteControl.h"
//#include "logFileExFunc.h"
#include "gloox/util.h"

// 通用参数
#define defParamNm_Remote_ExType	"ex"
#define defParamNm_Remote_pam		"pm"	// 参数字节流

// 各个类型不同参数
#define defParamNm_Remote_ip		"ip"
#define defParamNm_Remote_pwd		"pw"

RFRemoteControl::RFRemoteControl(const Tag* tag)
{
	if( !tag || tag->name() != defDeviceTypeTag_rfremote )
      return;

	if( tag->hasAttribute( "cmd" ) )
	{
		this->m_Cmd = (defCmd)atoi( tag->findAttribute( "cmd" ).c_str() );
	}

	UntagAllBtn( tag );
}

RFRemoteControl::~RFRemoteControl(void)
{
}

IOTDeviceType RFRemoteControl::Save_TransFromExType( const std::string& loadstr )
{
	std::map<std::string, std::string> mapval;
	split_getmapval( loadstr, mapval, defParamNm_Remote_ExType );

	if( mapval.empty() )
	{
		return IOT_DEVICE_Unknown;
	}

	const IOTDeviceType ExType = (IOTDeviceType)atoi( mapval.begin()->second.c_str() );

	return ExType;
}

Tag* RFRemoteControl::tag(const struTagParam &TagParam) const
{
	Tag* i = new Tag( defDeviceTypeTag_rfremote );
	
	tagAllBtn(i, TagParam);

	return i;
}

std::string RFRemoteControl::Print( const char *info, bool doPrint, RemoteButton *const pSpec ) const
{
	char buf[256] = {0};
	RemoteButton *pButton = NULL;
	
	if( pSpec )
		pButton = GetButton( pSpec->GetId() );
	else
		pButton = this->GetFristButton();

	snprintf( buf, sizeof(buf), "RFRemoteCtrl(%s) RemoteButton=%d", info?info:"", pButton?pButton->GetId():0 );

	if( doPrint )
	{
		LOGMSG( buf );
	}

	return std::string(buf);
}


/////////////////////////////////////////


GSRemoteCtl_AC::GSRemoteCtl_AC( const Tag* tag )
	:RFRemoteControl(tag)
{
}

GSRemoteCtl_AC::~GSRemoteCtl_AC()
{
}

Tag* GSRemoteCtl_AC::tag( const struTagParam &TagParam ) const
{
	Tag* i = RFRemoteControl::tag( TagParam );

	i->addAttribute( "extype", this->GetExType() );

	return i;
}

std::string GSRemoteCtl_AC::Save_RefreshToSave()
{
	char buf[64] = {0};

	std::vector<std::pair<std::string,std::string> > lstval;
	m_param.sParam.size = sizeof(m_param.sParam);
	//lstval.push_back( std::make_pair(defParamNm_Remote_ExType,itoa( this->GetExType(), buf, 10 )) );
	//jyc20160919 trouble
	//lstval.push_back( std::make_pair(defParamNm_Remote_ExType,snprintf(buf,10,"%d", this->GetExType())) );
	lstval.push_back( std::make_pair(defParamNm_Remote_ip,m_param.ip) );
	lstval.push_back( std::make_pair(defParamNm_Remote_pwd,m_param.pwd) );
	lstval.push_back( std::make_pair(defParamNm_Remote_pam,g_BufferToString( (unsigned char*)&m_param.sParam, sizeof(m_param.sParam), false )) );

	return g_lstval2str( lstval );
}

void GSRemoteCtl_AC::Save_RefreshFromLoad( const std::string& loadstr )
{
	std::map<std::string,std::string> mapval;
	split_getmapval( loadstr, mapval );

	for( std::map<std::string,std::string>::const_iterator it=mapval.begin(); it!=mapval.end(); ++it )
	{
		if( it->first == defParamNm_Remote_ip )
		{
			m_param.ip = it->second;
		}
		else if( it->first == defParamNm_Remote_pwd )
		{
			m_param.pwd = it->second;
		}
		else if( it->first == defParamNm_Remote_pam )
		{
			g_StringToBuffer( it->second, (uint8_t*)&m_param.sParam, sizeof(m_param.sParam), false );
		}
	}
}

std::string GSRemoteCtl_AC::GetCfgDesc() const
{
	std::string str;
	char buf[64] ={0};

	str = m_param.ip;
	str += ":";
	//str += itoa( m_param.sParam.port, buf, 10 );
	str += snprintf(buf, sizeof(buf), "%d", m_param.sParam.port);

	return str;
}

GSRemoteObj_AC_Door* GSRemoteCtl_AC::GetDoor( const long DoorNo ) const
{
	defButtonQueue::const_iterator it = m_ButtonQueue.begin();
	defButtonQueue::const_iterator itEnd = m_ButtonQueue.end();
	for( ; it!=itEnd; ++it )
	{
		if( IOTDevice_AC_Door == (*it)->GetExType() )
		{
			GSRemoteObj_AC_Door *prmobj = (GSRemoteObj_AC_Door*)(*it);
			if( DoorNo==prmobj->get_param().DoorNo )
				return prmobj;
		}
	}
	
	return NULL;
}

defGSReturn GSRemoteCtl_AC::UpdateDoorState( const long DoorNo, const long state )
{
	defGSReturn ret = defGSReturn_Err;
	defButtonQueue::const_iterator it = m_ButtonQueue.begin();
	defButtonQueue::const_iterator itEnd = m_ButtonQueue.end();
	for( ; it!=itEnd; ++it )
	{
		if( IOTDevice_AC_Door == (*it)->GetExType() )
		{
			GSRemoteObj_AC_Door *prmobj = (GSRemoteObj_AC_Door*)(*it);
			if( DoorNo==prmobj->get_param().DoorNo )
			{
				prmobj->get_state().DoorState = state;

				if( defGSReturn_Success!=ret )
				{
					ret = defGSReturn_Success;
				}
			}
		}
	}

	return ret;
}

long GSRemoteCtl_AC::GetOneNewDoorNo() const
{
	for( int i=1; i<32; ++i )
	{
		if( GetDoor(i) )
			continue;

		return i;
	}

	return 0;
}


/////////////////////////////////////////


GSRemoteCtl_Combo_Ctl::GSRemoteCtl_Combo_Ctl( const Tag* tag )
	:RFRemoteControl(tag)
{
}

GSRemoteCtl_Combo_Ctl::~GSRemoteCtl_Combo_Ctl()
{
}

Tag* GSRemoteCtl_Combo_Ctl::tag( const struTagParam &TagParam ) const
{
	Tag* i = RFRemoteControl::tag( TagParam );

	i->addAttribute( "extype", this->GetExType() );

	return i;
}

std::string GSRemoteCtl_Combo_Ctl::Save_RefreshToSave()
{
	char buf[64] = {0};

	std::vector<std::pair<std::string,std::string> > lstval;
	//lstval.push_back( std::make_pair(defParamNm_Remote_ExType,itoa( this->GetExType(), buf, 10 )) );
	//lstval.push_back( std::make_pair(defParamNm_Remote_ExType,snprintf(buf,sizeof(buf),"%d",this->GetExType())) );
	return g_lstval2str( lstval );
}

void GSRemoteCtl_Combo_Ctl::Save_RefreshFromLoad( const std::string& loadstr )
{
}
