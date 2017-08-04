#include "RemoteButtonClass.h"
#include "common.h"

RemoteButton::RemoteButton( const RFSignal &signal )
{
	Reset();

	this->m_signal = signal;
}

RemoteButton::RemoteButton( const Tag* tag )
{
	Reset();

	if( !tag || tag->name() != "button")
		return;

	if(tag->hasAttribute("name"))
		this->m_name = UTF8ToASCII(tag->findAttribute("name"));

	if(tag->hasAttribute("code"))
		this->m_id = atoi( tag->findAttribute("code").c_str() );

	this->UntagEditAttr( tag );
}

bool RemoteButton::doEditAttrFromAttrMgr( const EditAttrMgr &attrMgr, defCfgOprt_ oprt )
{
	if( attrMgr.GetEditAttrMap().empty() )
		return false;

	bool doUpdate = false;
	std::string outAttrValue;

	if( attrMgr.FindEditAttr( "name", outAttrValue ) )
	{
		doUpdate = true;
		this->SetName( outAttrValue );
	}

	if( attrMgr.FindEditAttr( "enable", outAttrValue ) )
	{
		doUpdate = true;
		this->SetEnable( atoi(outAttrValue.c_str()) );
	}

	return doUpdate;
}

void RemoteButton::Reset( bool all )
{
	m_id = 0;
	m_sort_no = 0;
	m_enable = defDeviceEnable;
	memset( &m_signal, 0, sizeof(m_signal) );
	m_result = defGSReturn_Null;

	m_BaseAttr.set_AdvAttr( GSRemoteObjBaseAttr::defAttr_cmd_Open, true );
}

Tag* RemoteButton::tag(const struTagParam &TagParam)
{
	Tag* i = new Tag( "button" );

	i->addAttribute("code",(int)this->m_id);
	
	if( TagParam.isValid && TagParam.isResult )
	{
		this->tagEditAttr( i, TagParam );
		return i;
	}

	i->addAttribute("name",ASCIIToUTF8(this->m_name));
	return i;
}


/////////////////////////////////////////


GSRemoteObj_AC_Door::GSRemoteObj_AC_Door( const RFSignal &signal )
	:RemoteButton(signal)
{
	this->Reset(false);

	this->Save_RefreshFromLoad();
}

GSRemoteObj_AC_Door::GSRemoteObj_AC_Door( const Tag* tag )
	:RemoteButton(tag)
{
	this->Reset(false);
}

void GSRemoteObj_AC_Door::Reset( bool all )
{
	if( all )
	{
		RemoteButton::Reset( all );
	}
}

Tag* GSRemoteObj_AC_Door::tag( const struTagParam &TagParam )
{
	Tag* i = RemoteButton::tag( TagParam );

	i->addAttribute( "extype", this->GetExType() );

	Tag* pDoor = new Tag( "door" );
	//pDoor->addAttribute( "no", (int)this->m_param.DoorNo );
	pDoor->addAttribute( "st", (int)this->m_state.DoorState );
	i->addChild( pDoor );

	return i;
}

void GSRemoteObj_AC_Door::Save_RefreshToSave()
{
	const int saveid = this->GetSignalSafe().id;

	RFSignal::Init( this->GetSignal() );
	
	this->GetSignal().id = saveid;
	this->GetSignal().signal_type = defRFSignalType_RemoteObj_ExType;
	this->GetSignal().code = IOTDevice_AC_Door;

	this->m_param.size = sizeof( m_param );
	this->m_param.flagsave_base = m_BaseAttr.get_AdvAttr_uintfull();
	this->m_param.flagsave_ex = 0;

	this->GetSignal().original_len = sizeof(m_param);
	memcpy( &this->GetSignal().original, &m_param, sizeof(m_param) );
}

void GSRemoteObj_AC_Door::Save_RefreshFromLoad()
{
	memcpy( &m_param, &this->GetSignal().original, sizeof( m_param ) );
	m_BaseAttr.set_AdvAttr_uintfull( this->m_param.flagsave_base );
}

