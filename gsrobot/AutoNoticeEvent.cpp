#include "AutoNoticeEvent.h"
#include "gloox/util.h"

AutoNoticeEvent::AutoNoticeEvent(void)
{
}

AutoNoticeEvent::AutoNoticeEvent(const Tag* tag)
	:ControlEvent(tag)
{
	if( !tag || tag->name() != "messagething" )
      return;

	this->untagBase( tag );

	if(tag->hasAttribute("to"))
		m_to_jid = tag->findAttribute("to");

	if(tag->hasAttribute("subject"))
		m_msg_subject = UTF8ToASCII( tag->findAttribute("subject") );

	Tag *tText = tag->findChild("text");
	if( tText )
	{
		m_msg_body = UTF8ToASCII( tText->cdata() );
	}

	this->UntagEditAttr( tag );
}

AutoNoticeEvent::~AutoNoticeEvent(void)
{
}

bool AutoNoticeEvent::doEditAttrFromAttrMgr( const EditAttrMgr &attrMgr, defCfgOprt_ oprt )
{
	if( attrMgr.GetEditAttrMap().empty() )
		return false;

	bool doUpdate = this->doEditAttrFromAttrMgr_EvtBase( attrMgr, oprt );
	std::string outAttrValue;

	if( attrMgr.FindEditAttr( "to", outAttrValue ) )
	{
		doUpdate = true;
		this->SetToJid( outAttrValue );
	}

	if( attrMgr.FindEditAttr( "subject", outAttrValue ) )
	{
		doUpdate = true;
		this->SetSubject( outAttrValue );
	}

	if( attrMgr.FindEditAttr( "text", outAttrValue ) )
	{
		doUpdate = true;
		this->SetBody( outAttrValue );
	}

	return doUpdate;
}

Tag* AutoNoticeEvent::tag(const struTagParam &TagParam) const
{
	Tag* i = new Tag( "messagething" );

	tagBase( i, TagParam );

	if( TagParam.isValid && TagParam.isResult )
	{
		this->tagEditAttr( i, TagParam );
	}
	else
	{
		i->addAttribute( "to", this->m_to_jid );
		i->addAttribute( "subject", ASCIIToUTF8(this->m_msg_subject) );

		new Tag( i, "text", ASCIIToUTF8(this->m_msg_body) );
	}

	return i;
}

bool AutoNoticeEvent::EventParamIsComplete()
{
	// 为空时内容自动生成
	//if( m_msg_subject.empty() )
	//{
	//	return false;
	//}

	//if( m_msg_body.empty() )
	//{
	//	return false;
	//}

	return true;
}
