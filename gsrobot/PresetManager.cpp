#include "PresetManager.h"
#include "common.h"
#include <functional>


struct sort_PresetObj : std::greater<CPresetObj*>
{
	bool operator()(const CPresetObj *_X, const CPresetObj *_Y) const
	{
		return (_X->GetSortNo() < _Y->GetSortNo());
	}
};

CPresetObj::CPresetObj( const std::string &name )
{
	Reset();
	m_name = name;
}

CPresetObj::CPresetObj( const struDBSavePreset &obj )
{
	Reset();

	m_id = obj.id;
	m_index = obj.preset_index;
	m_sort_no = obj.sort_no;
	m_enable = obj.enable;
	m_name = obj.name;
}

CPresetObj::CPresetObj( const Tag* tag )
{
	Reset();

	if( !tag || tag->name() != "preset")
		return;

	if(tag->hasAttribute("id"))
		this->m_id = atoi( tag->findAttribute("id").c_str() );

	if(tag->hasAttribute("nm"))
		this->m_name = UTF8ToASCII(tag->findAttribute("nm"));
}

void CPresetObj::Reset()
{
	m_id = 0;
	m_index = 0;
	m_sort_no = 0;
	m_enable = defDeviceEnable;
	m_name = "";
}

Tag* CPresetObj::tag(const struTagParam &TagParam)
{
	Tag* i = new Tag( "preset" );

	i->addAttribute("id",(int)this->m_id);
	i->addAttribute("nm",ASCIIToUTF8(this->m_name));
	return i;
}

void CPresetObj::SwapInfo( CPresetObj &PresetOther )
{
	const int thisID = this->GetId();
	const int OtherID = PresetOther.GetId();

	const CPresetObj temp = PresetOther;
	PresetOther = *this;
	*this = temp;

	this->SetId(thisID);
	PresetOther.SetId(OtherID);
}

//===============================================

CPresetManager::CPresetManager(void)
{
}

CPresetManager& CPresetManager::operator= ( const CPresetManager &RightSides )
{
	if( this == &RightSides )
	{
		return *this;
	}

	DeletePresetQueue();

	ClonePresetQueue_Spec( this->m_PresetQueue, RightSides.m_PresetQueue );

	return *this;
}

CPresetManager::CPresetManager( const CPresetManager &RightSides )
{
	*this = RightSides;
}

CPresetManager::~CPresetManager(void)
{
	DeletePresetQueue();
}

// 是否已存在等信息判断
defGSReturn CPresetManager::CheckExist( const int PresetID, const std::string &newname, std::string *strerr )
{
	defPresetQueue::const_iterator it = m_PresetQueue.begin();
	defPresetQueue::const_iterator itEnd = m_PresetQueue.end();
	for( ; it!=itEnd; ++it )
	{
		CPresetObj *pPreset = (*it);

		if( PresetID == pPreset->GetId() )
			continue;
		
		if( newname == pPreset->GetObjName() )
		{
			if( strerr ) *strerr = "名称已存在";
			return defGSReturn_SameName;
		}
	}

	return defGSReturn_Success;
}

void CPresetManager::DeletePresetQueue_Spec( defPresetQueue &que )
{
	while( !que.empty() )
	{
		CPresetObj *pPreset = que.front();
		delete(pPreset);
		que.pop_front();
	}
}

bool CPresetManager::DeletePreset_Spec( defPresetQueue &que, uint32_t PresetID )
{
	//defPresetQueue::const_iterator it = que.begin();
	//defPresetQueue::const_iterator itEnd = que.end();
	defPresetQueue::iterator it = que.begin();  //jyc20160922
	defPresetQueue::iterator itEnd = que.end();
	for( ; it!=itEnd; ++it )
	{
		CPresetObj *pPreset = (*it);
		if( pPreset->GetId() == PresetID )
		{
			delete(pPreset);
			que.erase(it);
			return true;
		}
	}

	return false;
}

void CPresetManager::ClonePresetQueue_Spec( defPresetQueue &quedest, const defPresetQueue &quesrc )
{
	defPresetQueue::const_iterator it = quesrc.begin();
	defPresetQueue::const_iterator itEnd = quesrc.end();
	for( ; it!=itEnd; ++it )
	{
		quedest.push_back( (*it)->clone() );
	}
}

void CPresetManager::PresetQueueChangeToOne_Spec( defPresetQueue &que, uint32_t PresetID )
{
	bool isFound = false;
	while( que.size()>1 )
	{
		CPresetObj *pPreset = que.back();
		que.pop_back();

		if( !isFound && PresetID>0 )
		{
			if( pPreset->GetId() == PresetID )
			{
				isFound = true;
				que.push_front(pPreset);
				continue;
			}
		}

		delete(pPreset);
	}
}

CPresetObj* CPresetManager::GetPresetForID_Spec( const defPresetQueue &que, uint32_t PresetID )
{
	defPresetQueue::const_iterator it = que.begin();
	defPresetQueue::const_iterator itEnd = que.end();
	for( ; it!=itEnd; ++it )
	{
		if( (*it)->GetId() == PresetID )
		{
			return (*it);
		}
	}

	return NULL;
}

CPresetObj* CPresetManager::GetPresetForIndex_Spec( const defPresetQueue &que, uint32_t PresetIndex )
{
	defPresetQueue::const_iterator it = que.begin();
	defPresetQueue::const_iterator itEnd = que.end();
	for( ; it!=itEnd; ++it )
	{
		if( (*it)->GetIndex() == PresetIndex )
		{
			return (*it);
		}
	}

	return NULL;
}

int CPresetManager::GetUnusedIndex_Spec( const defPresetQueue &que )
{
	for( int i=defGSPresetIndex_Min; i<=defGSPresetIndex_Max; ++i )
	{
		if( !CPresetManager::GetPresetForIndex_Spec( que, i ) )
		{
			return i;
		}
	}

	return 0;
}

void CPresetManager::tagAllPreset( Tag* iDest, const struTagParam &TagParam ) const
{
	if( !iDest )
	{
		return;
	}

	if( !m_PresetQueue.empty() )
	{
		defPresetQueue::const_iterator it = m_PresetQueue.begin();
		defPresetQueue::const_iterator itEnd = m_PresetQueue.end();
		for( ; it!=itEnd; ++it )
		{
			iDest->addChild( (*it)->tag(TagParam) );
		}
	}
}

int CPresetManager::GetUnusedIndex() const
{
	return CPresetManager::GetUnusedIndex_Spec( this->m_PresetQueue );
}

void CPresetManager::DeletePresetQueue()
{
	CPresetManager::DeletePresetQueue_Spec( this->m_PresetQueue );
}

bool CPresetManager::DeletePreset( uint32_t PresetID )
{
	return CPresetManager::DeletePreset_Spec( this->m_PresetQueue, PresetID );
}

void CPresetManager::PresetQueueChangeToOne( uint32_t PresetID )
{
	CPresetManager::PresetQueueChangeToOne_Spec( this->m_PresetQueue, PresetID );
}

CPresetObj* CPresetManager::GetPreset( uint32_t PresetID ) const
{
	return CPresetManager::GetPresetForID_Spec( this->m_PresetQueue, PresetID );
}

//CPresetObj* CPresetManager::GetPresetForIndex( uint32_t PresetIndex ) const
//{
//	return CPresetManager::GetPresetForIndex_Spec( this->m_PresetQueue, PresetIndex );
//}

CPresetObj* CPresetManager::GetFristPreset() const
{
	if( !m_PresetQueue.empty() )
	{
		return ( *m_PresetQueue.begin() );
	}

	return NULL;
}

CPresetObj* CPresetManager::UpdatePreset( const CPresetObj *pPreset )
{
	if( !pPreset )
	{
		return NULL;
	}

	defPresetQueue::iterator it = m_PresetQueue.begin();
	defPresetQueue::iterator itEnd = m_PresetQueue.end();
	for( ; it!=itEnd; ++it )
	{
		CPresetObj *pCurPreset = *it;

		if( pCurPreset->GetId() == pPreset->GetId() )
		{
			pCurPreset->SetName( pPreset->GetObjName() );

			return pCurPreset;
		}
	}

	return NULL;
}

// 输入的列表已经排好序，将排序号更新到成员变量里
void CPresetManager::Sort( std::map<int,int> &sortlist )
{
	defPresetQueue::iterator it = m_PresetQueue.begin();
	defPresetQueue::const_iterator itEnd = m_PresetQueue.end();
	for( ; it!=itEnd; ++it )
	{
		const std::map<int,int>::const_iterator itFind = sortlist.find( (*it)->GetId() );

		if( itFind != sortlist.end() )
		{
			(*it)->SetSortNo( itFind->second );
		}
		else
		{
			(*it)->SetSortNo( 0 );
		}
	}

	m_PresetQueue.sort( sort_PresetObj() );
}

// 用已经排好序的信息列表对配置更新
void CPresetManager::SortByPresetList( const defPresetQueue &PresetSortlist )
{
	std::map<int,int> sortlist;
	int i=1;

	defPresetQueue::const_iterator it = PresetSortlist.begin();
	defPresetQueue::const_iterator itEnd = PresetSortlist.end();
	for( ; it!=itEnd; ++it, ++i )
	{
		sortlist[ (*it)->GetId() ] = i;
	}

	this->Sort( sortlist );
}
