#include "AutoSendSMSEvent.h"
//#include "windows.h"


AutoSendSMSEvent::AutoSendSMSEvent( std::string &Phone, std::string &SMS, uint32_t flag, uint32_t SendCount )
	:m_flag(flag), m_Phone(Phone), m_SMS(SMS)
{
	m_ts = timeGetTime();
	this->SetSendCount( SendCount );
}

AutoSendSMSEvent::AutoSendSMSEvent(const Tag* tag)
	:ControlEvent(tag), m_flag(0)
{
	m_ts = timeGetTime();
	this->SetSendCount(1);

	if( !tag || tag->name() != "smsthing" )
		return;

	this->untagBase( tag );

	if(tag->hasAttribute("to"))
		m_Phone = tag->findAttribute("to");

	Tag *tText = tag->findChild("text");
	if( tText )
	{
		m_SMS = UTF8ToASCII( tText->cdata() );
	}

	this->UntagEditAttr( tag );
}

AutoSendSMSEvent::~AutoSendSMSEvent(void)
{
}

bool AutoSendSMSEvent::doEditAttrFromAttrMgr( const EditAttrMgr &attrMgr, defCfgOprt_ oprt )
{
	if( attrMgr.GetEditAttrMap().empty() )
		return false;

	bool doUpdate = this->doEditAttrFromAttrMgr_EvtBase( attrMgr, oprt );
	std::string outAttrValue;

	if( attrMgr.FindEditAttr( "to", outAttrValue ) )
	{
		doUpdate = true;
		this->SetPhone( outAttrValue );
	}

	if( attrMgr.FindEditAttr( "text", outAttrValue ) )
	{
		doUpdate = true;
		this->SetSMS( outAttrValue );
	}

	return doUpdate;
}

bool AutoSendSMSEvent::isTimeOver() const
{
	return ( timeGetTime()-m_ts > 120*1000 );
}

Tag* AutoSendSMSEvent::tag(const struTagParam &TagParam) const
{
	Tag* i = new Tag( "smsthing" );

	tagBase( i, TagParam );

	if( TagParam.isValid && TagParam.isResult )
	{
		this->tagEditAttr( i, TagParam );
	}
	else
	{
		i->addAttribute( "to", this->m_Phone );

		new Tag( i, "text", ASCIIToUTF8(this->m_SMS) );
	}

	return i;
}

bool AutoSendSMSEvent::EventParamIsComplete()
{
	// 为空时内容自动生成
	//if( m_SMS.empty() )
	//{
	//   return false;
	//}

	return true;
}
