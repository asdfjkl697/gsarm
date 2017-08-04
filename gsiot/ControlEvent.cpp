#include "ControlEvent.h"
#include "gloox/util.h"
//#include <Windows.h>
#include "RunCode.h"
#include "GSIOTConfig.h"

bool ControlEvent::doEditAttrFromAttrMgr_EvtBase( const EditAttrMgr &attrMgr, defCfgOprt_ oprt )
{
	if( attrMgr.GetEditAttrMap().empty() )
		return false;

	bool doUpdate = false;
	std::string outAttrValue;

	if( attrMgr.FindEditAttr( "enable", outAttrValue ) )
	{
		doUpdate = true;
		this->SetEnable( atoi(outAttrValue.c_str()) );
	}
	
	return doUpdate;
}

bool ControlEvent::IsCanDo( const GSIOTConfig *pcfg, uint32_t &outDoInterval ) const
{
	if( IsRUNCODEEnable(defCodeIndex_TEST_DoEvent_NoInterval) )
	{
		return true;
	}

	if( SMS_Event==GetType() )
	{
		outDoInterval = pcfg ? pcfg->GetDoInterval_ForSMS() : 120;
		outDoInterval = macUseDoInterval(outDoInterval);
	}
	else
	{
		return true;
	}
	// uint32_t sysdefault 

	if( this->m_lastTimeTick_Do != 0 
		&& timeGetTime() - this->m_lastTimeTick_Do < outDoInterval*1000 )
	{
		return false;
	}

	return true;
}

void ControlEvent::SetDo()
{
	this->m_lastTimeTick_Do = timeGetTime();

	if( 0==this->m_lastTimeTick_Do )
	{
		this->m_lastTimeTick_Do++;
	}
}

void ControlEvent::untagBase( const Tag *t )
{
	if(t->hasAttribute("id"))
		this->SetID( atoi(t->findAttribute("id").c_str()) );

	if(t->hasAttribute("enable"))
		this->SetEnable( atoi(t->findAttribute("enable").c_str()) );

	if(t->hasAttribute("lv"))
		this->SetLevel( atoi(t->findAttribute("lv").c_str()) );

	if(t->hasAttribute("force"))
		this->SetForce( atoi(t->findAttribute("force").c_str())?true:false );
}

void ControlEvent::tagBase( Tag *t, const struTagParam &TagParam ) const
{
	if( !t )
		return;

	t->addAttribute( "id", (int)this->GetID() );

	if( TagParam.isValid && TagParam.isResult )
	{
	}
	else
	{
		t->addAttribute( "enable", (int)this->GetEnable() );
		t->addAttribute( "lv", (int)this->GetLevel() );
		if( this->isForce() ) t->addAttribute( "force", (int)1 );
	}
}

