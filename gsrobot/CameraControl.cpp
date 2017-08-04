#include "CameraControl.h"
#include "IPCameraBase.h"
#include "gloox/util.h"
#include "gsiot/GSIOTUser.h"

CameraControl::CameraControl(IPCameraBase* pCamBase, const std::string& protocol,const std::string& ver,
	const std::string& url,const std::string& state, GSPtzFlag PTZFlag, GSFocalFlag FocalFlag)
		:m_pCamBase(pCamBase), m_protocol(protocol), m_ver(ver), m_FromUrl(url), m_state(state), m_PTZFlag(PTZFlag), m_FocalFlag(FocalFlag)
{
	m_reboot = 0;
	m_HasCmdCtl = false;
	m_buffertime = 2000;
	m_trackCtl = GSPTZ_Null;
	m_trackX = 0;
	m_trackY = 0;
	m_trackEndX = 0;
	m_trackEndY = 0;
}

CameraControl::CameraControl( const Tag* tag)
	:m_pCamBase(NULL), ControlBase(tag), m_PTZFlag(GSPtzFlag_Null), m_FocalFlag(GSFocalFlag_Null)
{
	m_reboot = 0;
	m_HasCmdCtl = false;
	m_buffertime = 2000;
	m_trackCtl = GSPTZ_Null;
	m_trackX = 0;
	m_trackY = 0;
	m_trackEndX = 0;
	m_trackEndY = 0;

	if( !tag || tag->name() != defDeviceTypeTag_camera)
      return;

	this->m_ver = tag->findAttribute("ver");

	if(tag->hasAttribute("buffertime"))
		this->m_buffertime = atoi(tag->findAttribute("buffertime").c_str());

	if(tag->hasChild("protocol"))
	    this->m_protocol = tag->findChild("protocol")->cdata();
	if(tag->hasChild("url"))
	    this->m_FromUrl = tag->findChild("url")->cdata();

	//#ifdef _DEBUG//
#if 0
	std::string useUrl_lastback;
	if( IsRUNCODEEnable( defCodeIndex_TEST_Debug_TempCode ) )
	{
		if( !g_IsRTMFP_url( m_FromUrl ) ) useUrl_lastback = m_FromUrl;

		//m_FromUrl = "rtmfp://192.168.0.76:1985/p2p";
		m_FromUrl = "rtmfp://p2p.gsss.cn:1985/p2p";
		//m_FromUrl = "rtmfp://unknownp2p.gsss.cn:1985/p2p";

		unsigned char thisid[32] ={0};
		uint16_t thisid_num = 0;

		GUID guid;
		::CoCreateGuid( &guid );
		m_FromUrl += "/";
		m_FromUrl += g_BufferToString( (unsigned char*)&guid, sizeof( guid ), false, false );
	}
#endif

	if( IsRUNCODEEnable( defCodeIndex_RTMFP_DelFromUrlStreamID ) && g_IsRTMFP_url( m_FromUrl ) )
	{
		std::vector<std::string> useUrl_unit_vec;
		split( m_FromUrl, useUrl_unit_vec, "/" );

		const int unitsize = useUrl_unit_vec.size();
		// ż�����ʾ���һ��Ϊstreamid��ȥ��
		if( 0 == (unitsize%2) )
		{
			g_replace_all_distinct( m_FromUrl, std::string( "/" )+useUrl_unit_vec[unitsize-1], "" );
		}
	}

	Tag *tchildlist_url_backup = tag->findChild("url_backup");
	if( tchildlist_url_backup )
	{
		const TagList& l = tchildlist_url_backup->children();
		TagList::const_iterator it = l.begin();
		for( ; it != l.end(); ++it )
		{
			Tag *tChild = (*it);
			if( tChild->name() == "url" )
			{
				m_url_backup.push_back( tChild->cdata() );
			}
		}
	}
	//if( !useUrl_lastback.empty() ) m_url_backup.push_back( useUrl_lastback );

	if(tag->hasChild("state"))
	    this->m_state = tag->findChild("state")->cdata();
	if(tag->hasChild("ptz")){
		m_PTZ.untag( tag->findChild("ptz") );
		m_PTZFlag = GSPtzFlag_forward_direction;
	}
	if(tag->hasChild("focal")){
	    m_Focal.untag( tag->findChild("focal") );
		m_FocalFlag = GSFocalFlag_Enable;
	}

	Tag *tTrack = tag->findChild("track");
	if( tTrack )
	{
		m_HasCmdCtl = true;
		if( tTrack->hasAttribute("method") )
		{
			std::string method = tTrack->findAttribute("method");
			g_toLowerCase( method );
			if( method == "motion" )
			{
				Tag *tEnable = tTrack->findChild("enable");
				if( tEnable )
				{
					if( tEnable->cdata() == "true" )
						m_trackCtl = GSPTZ_MOTION_TRACK_Enable;
					else
						m_trackCtl = GSPTZ_MOTION_TRACK_Disable;
				}
			}
			else if( method == "manual" )
			{
				Tag *tX = tTrack->findChild("x");
				Tag *tY = tTrack->findChild("y");
				if( tX && tY )
				{
					m_trackCtl = GSPTZ_MANUALTRACE;
					m_trackX = atoi( tX->cdata().c_str() );
					m_trackY = atoi( tY->cdata().c_str() );
				}
			}
		}
	}
	
	Tag *tPtzsel = tag->findChild("ptzsel");
	if( tPtzsel )
	{
		m_HasCmdCtl = true;
		Tag *tX = tPtzsel->findChild("x");
		Tag *tY = tPtzsel->findChild("y");
		if( tX && tY )
		{
			m_trackCtl = GSPTZ_MANUALPTZSel;
			m_trackX = atoi( tX->cdata().c_str() );
			m_trackY = atoi( tY->cdata().c_str() );
		}
	}
	
	const Tag *tZoomrng = tag->findChild("zoomrng");
	if( tZoomrng )
	{
		m_HasCmdCtl = true;
		const Tag *tX = tZoomrng->findChild("x");
		const Tag *tY = tZoomrng->findChild("y");
		const Tag *tEndX = tZoomrng->findChild("endx");
		const Tag *tEndY = tZoomrng->findChild("endy");
		if( tX && tY && tEndX && tEndY )
		{
			m_trackCtl = GSPTZ_MANUALZoomRng;
			m_trackX = atoi( tX->cdata().c_str() );
			m_trackY = atoi( tY->cdata().c_str() );
			m_trackEndX = atoi( tEndX->cdata().c_str() );
			m_trackEndY = atoi( tEndY->cdata().c_str() );
		}
	}

	Tag *tParkAction = tag->findChild("parkaction");
	if( tParkAction )
	{
		m_HasCmdCtl = true;

		if( tParkAction->hasAttribute( "enable" ) )
		{
			if( atoi( tParkAction->findAttribute( "enable" ).c_str() ) )
				m_trackCtl = GSPTZ_PTZ_ParkAction_Enable;
			else
				m_trackCtl = GSPTZ_PTZ_ParkAction_Disable;
		}
	}
	
	Tag *tPrepic = tag->findChild("prepic");
	if( tPrepic )
	{
		m_HasCmdCtl = true;

		if( tPrepic->cdata() == "do" )
		{
			m_trackCtl = GSPTZ_DoPrePic;
		}
	}

	if( tag->hasChild("reboot") )
	{
		m_reboot = GSPTZ_CameraReboot;
	}

	this->UntagEditAttr( tag );
}

CameraControl::~CameraControl(void)
{
}

Tag* CameraControl::tag(const struTagParam &TagParam) const
{
	Tag* i = new Tag( defDeviceTypeTag_camera );
	
	if( TagParam.isValid && TagParam.isResult )
	{
		this->tagEditAttr( i, TagParam );
		return i;
	}

	const bool auth_wo = GSIOTUser::JudgeAuth( TagParam.Auth, defUserAuth_WO );

	i->addAttribute("ver",this->m_ver);

	new Tag(i,"protocol", g_IsRTMFP_url(m_pCamBase->getUrl())?"rtmfp":"rtmp");
	new Tag(i,"url",m_pCamBase->getUrl());
	new Tag(i,"peerid",m_pCamBase->GetStreamObj()->GetRTMPSendObj()->getPeerID());
	new Tag(i,"streamid",m_pCamBase->GetStreamObj()->GetRTMPSendObj()->getStreamID());
	new Tag(i,"state",this->m_state);
	new Tag(i,"buffertime",util::int2string(this->m_buffertime));

	if( m_pCamBase->IsSupportAGRunState() )
	{
		new Tag(i,"agstate",util::int2string(m_pCamBase->GetAGRunStateFinalST()));
	}

	if(m_PTZFlag && auth_wo){
		Tag *pPTZ = m_PTZ.tag();

		if( m_pCamBase->GetAdvAttr().get_AdvAttr( defCamAdvAttr_PTZ_Ang ) )
		{
			pPTZ->addAttribute("ang",1); // Ĭ���ǲ�֧��б�Ƿ������
		}

		i->addChild(pPTZ);
	}
	if(m_FocalFlag && auth_wo){
		i->addChild(m_Focal.tag());
	}

	if( m_pCamBase )
	{
		if( m_pCamBase->GetAdvAttr().get_AdvAttr( defCamAdvAttr_motion_track ) && auth_wo )
		{
			Tag *pTrack = new Tag( i, "track" );
			new Tag(pTrack,"enable", m_pCamBase->GetPTZState( GSPTZ_MOTION_TRACK_Enable, false )?"true":"false" );
		}

		if( m_pCamBase->GetAdvAttr().get_AdvAttr( defCamAdvAttr_manual_ptzsel ) && auth_wo )
		{
			new Tag( i, "ptzsel" );
		}

		if( m_pCamBase->GetAdvAttr().get_AdvAttr( defCamAdvAttr_manual_zoomrng ) && auth_wo )
		{
			new Tag( i, "zoomrng" );
		}

		if( m_pCamBase->GetAdvAttr().get_AdvAttr( defCamAdvAttr_PTZ_Preset ) && auth_wo )
		{
			new Tag( i, "preset" );
		}

		if( m_pCamBase->GetAdvAttr().get_AdvAttr( defCamAdvAttr_PTZ_ParkAction ) && auth_wo )
		{
			Tag *pTrack = new Tag( i, "parkaction" );
			pTrack->addAttribute("enable",m_pCamBase->GetPTZState( GSPTZ_PTZ_ParkAction_Enable, false ));
		}

		if( m_pCamBase->GetAdvAttr().get_AdvAttr( defCamAdvAttr_CamTalk ) && auth_wo )
		{
			new Tag( i, "talk" );
		}

		if( defAudioSource_Null != m_pCamBase->GetAudioCfg().get_Audio_Source() )
		{
			new Tag( i, "sound", util::int2string(m_pCamBase->GetAudioCfg().get_Audio_Source()) );
		}

		if( defRecMod_NoRec != m_pCamBase->GetRecCfg().getrec_mod() )
		{
			new Tag( i, "record", util::int2string(m_pCamBase->GetRecCfg().getrec_mod()) );
		}

		if( m_pCamBase->GetAdvAttr().get_AdvAttr( defCamAdvAttr_SupportAlarm ) )
		{
			new Tag( i, "alarm" );
		}
	}

	return i;
}

ControlBase* CameraControl::clone( bool CreateLock ) const
{
	CameraControl *cc = new CameraControl(*this);

    return cc;
}

const std::string CameraControl::getStatus()
{
	gloox::util::MutexGuard mutexguard( m_camctl_mutex );

	return this->m_state;
}

void CameraControl::setStatus(const std::string& state)
{
	gloox::util::MutexGuard mutexguard( m_camctl_mutex );

	this->m_state = state;
}

const std::string CameraControl::getFromUrl()
{
	gloox::util::MutexGuard mutexguard( m_camctl_mutex );

	return this->m_FromUrl;
}

//void CameraControl::setUrl( const std::string& url )
//{
//	gloox::util::MutexGuard mutexguard( m_camctl_mutex );
//
//	this->m_url = url;
//}

