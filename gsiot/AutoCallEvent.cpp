#include "AutoCallEvent.h"


AutoCallEvent::AutoCallEvent( std::string &Phone, std::string &content, uint32_t try_count, uint32_t play_count, uint32_t flag1, uint32_t flag2 )
	:m_Phone(Phone), m_content(content), m_flag1(flag1), m_flag2(flag2)
{
	this->Set_try_count( try_count );
	this->Set_play_count( play_count );
}

AutoCallEvent::AutoCallEvent(const Tag* tag)
	:ControlEvent(tag), m_flag1(0), m_flag2(0)
{
	this->Set_try_count( 1 );
	this->Set_play_count( 1 );

	if( !tag || tag->name() != "callthing" )
		return;

	this->untagBase( tag );

	if(tag->hasAttribute("to"))
		m_Phone = tag->findAttribute("to");

	if(tag->hasAttribute("try_count"))
		m_try_count = atoi(tag->findAttribute("try_count").c_str());

	if(tag->hasAttribute("play_count"))
		m_play_count = atoi(tag->findAttribute("play_count").c_str());

	Tag *tText = tag->findChild("text");
	if( tText )
	{
		m_content = UTF8ToASCII( tText->cdata() );
	}

	this->UntagEditAttr( tag );
}

AutoCallEvent::~AutoCallEvent(void)
{
}

bool AutoCallEvent::doEditAttrFromAttrMgr( const EditAttrMgr &attrMgr, defCfgOprt_ oprt )
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
		this->SetContent( outAttrValue );
	}

	if( attrMgr.FindEditAttr( "try_count", outAttrValue ) )
	{
		doUpdate = true;
		this->Set_try_count( atoi(outAttrValue.c_str()) );
	}

	if( attrMgr.FindEditAttr( "play_count", outAttrValue ) )
	{
		doUpdate = true;
		this->Set_play_count( atoi(outAttrValue.c_str()) );
	}

	return doUpdate;
}

Tag* AutoCallEvent::tag(const struTagParam &TagParam) const
{
	Tag* i = new Tag( "callthing" );

	tagBase( i, TagParam );

	if( TagParam.isValid && TagParam.isResult )
	{
		this->tagEditAttr( i, TagParam );
	}
	else
	{
		i->addAttribute( "to", this->m_Phone );
		i->addAttribute( "try_count", (int)this->Get_try_count() );
		i->addAttribute( "play_count", (int)this->Get_play_count() );

		new Tag( i, "text", ASCIIToUTF8(this->m_content) );
	}

	return i;
}

bool AutoCallEvent::EventParamIsComplete()
{
	if( m_content.empty() )
	{
		return false;
	}

	return true;
}
