#include "EditAttrMgr.h"
#include "gloox/util.h"

#define defTagName_EditAttrMgr "attribute"

EditAttrMgr::EditAttrMgr(void):
	m_result(defGSReturn_Null)
{
}

EditAttrMgr::~EditAttrMgr(void)
{
	this->ClearEditAttr();
}

void EditAttrMgr::tagEditAttr( Tag* iDest, const struTagParam &TagParam ) const
{
	if( !iDest )
	{
		return;
	}

	if( TagParam.isValid && TagParam.isResult )
	{
		if( !macGSIsReturnNull(m_result) )
		{
			if( macGSSucceeded(m_result) )
			{
				new Tag( iDest, "result", std::string(defGSReturnStr_Succeed) );
			}
			else
			{
				new Tag( iDest, "result", std::string(defGSReturnStr_Fail) );
				new Tag( iDest, "errcode", util::int2string(m_result) );
			}
		}

		return;
	}

	if( m_AttrMap.empty() )
		return;

	Tag* i = new Tag( iDest, defTagName_EditAttrMgr );

	defmapEditAttr::const_iterator it = m_AttrMap.begin();
	defmapEditAttr::const_iterator itEnd = m_AttrMap.end();
	for( ; it!=itEnd; ++it )
	{
		new Tag( i, it->first, ASCIIToUTF8(it->second) );
	}
}

void EditAttrMgr::UntagEditAttr( const Tag* iSrc )
{
	if( !iSrc )
	{
		return;
	}

	Tag *tEdit = iSrc->findChild(defTagName_EditAttrMgr);
	if( !tEdit )
	{
		return;
	}

	const TagList& l = tEdit->children();
	TagList::const_iterator it = l.begin();
	TagList::const_iterator itEnd = l.end();
	for( ; it != itEnd; ++it )
	{
		AddEditAttr( (*it)->name(), UTF8ToASCII((*it)->cdata()) );
	}
}

void EditAttrMgr::AddEditAttr( const std::string &AttrName, const std::string &AttrValue )
{
	m_AttrMap[AttrName] = AttrValue;
}

bool EditAttrMgr::FindEditAttr( const std::string &AttrName, std::string &outAttrValue ) const
{
	if( m_AttrMap.empty() )
		return false;

	outAttrValue = "";

	defmapEditAttr::const_iterator it = m_AttrMap.find( AttrName );
	if( it != m_AttrMap.end() )
	{
		outAttrValue = it->second;
		return true;
	}

	return false;
}

void EditAttrMgr::ClearEditAttr()
{
	if( !m_AttrMap.empty() )
		m_AttrMap.clear(); 
}
