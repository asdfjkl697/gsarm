#include "RemoteButtonManager.h"
#include <functional>


struct sort_RemoteButton : std::greater < RemoteButton* >
{
	bool operator()(const RemoteButton *_X, const RemoteButton *_Y) const
	{
		return (_X->GetSortNo() < _Y->GetSortNo());
	}
};

RemoteButtonManager::RemoteButtonManager(void)
{
}

RemoteButtonManager& RemoteButtonManager::operator= ( const RemoteButtonManager &RightSides )
{
	if( this == &RightSides )
	{
		return *this;
	}
	
	DeleteButtonQueue();
	
	CloneButtonQueue_Spec( this->m_ButtonQueue, RightSides.m_ButtonQueue );

	return *this;
}

RemoteButtonManager::RemoteButtonManager( const RemoteButtonManager &RightSides )
{
	*this = RightSides;
}

RemoteButtonManager::~RemoteButtonManager(void)
{
	DeleteButtonQueue();
}

void RemoteButtonManager::DeleteButtonQueue_Spec( defButtonQueue &que )
{
	while( !que.empty() )
	{
		RemoteButton *button = que.front();
		delete(button);
		que.pop_front();
	}
}

bool RemoteButtonManager::DeleteButton_Spec( defButtonQueue &que, uint32_t buttonid )
{
	defButtonQueue::const_iterator it = que.begin();
	defButtonQueue::const_iterator itEnd = que.end();
	for( ; it!=itEnd; ++it )
	{
		RemoteButton *button = (*it);
		if( button->GetId() == buttonid )
		{
			delete(button);
			//que.erase(it); //jyc20170224 notice
			return true;
		}
	}

	return false;
}

void RemoteButtonManager::CloneButtonQueue_Spec( defButtonQueue &quedest, const defButtonQueue &quesrc )
{
	defButtonQueue::const_iterator it = quesrc.begin();
	defButtonQueue::const_iterator itEnd = quesrc.end();
	for( ; it!=itEnd; ++it )
	{
		quedest.push_back( (*it)->clone() );
	}
}

void RemoteButtonManager::ButtonQueueChangeToOne_Spec( defButtonQueue &que, uint32_t buttonid )
{
	bool isFound = false;
	while( que.size()>1 )
	{
		RemoteButton *pButton = que.back();
		que.pop_back();

		if( !isFound && buttonid>0 )
		{
			if( pButton->GetId() == buttonid )
			{
				isFound = true;
				que.push_front(pButton);
				continue;
			}
		}

		delete(pButton);
	}
}

void RemoteButtonManager::UntagAllBtn( const Tag* iSrc )
{
	if( !iSrc )
	{
		return;
	}

	const TagList& l = iSrc->children();
	TagList::const_iterator it = l.begin();
	TagList::const_iterator itEnd = l.end();
	for( ; it != itEnd; ++it )
	{
		const std::string& name = (*it)->name();
		if( name == "button" )
		{
			this->m_ButtonQueue.push_back(new RemoteButton(*it));
		}
	}
}

void RemoteButtonManager::tagAllBtn( Tag* iDest, const struTagParam &TagParam ) const
{
	if( !iDest )
	{
		return;
	}

	if( !m_ButtonQueue.empty() )
	{
		defButtonQueue::const_iterator it = m_ButtonQueue.begin();
		defButtonQueue::const_iterator itEnd = m_ButtonQueue.end();
		for( ; it!=itEnd; ++it )
		{
			if( (*it)->GetEnable() )
			{
				iDest->addChild( (*it)->tag(TagParam) );
			}
		}
	}
}

void RemoteButtonManager::DeleteButtonQueue()
{
	RemoteButtonManager::DeleteButtonQueue_Spec( this->m_ButtonQueue );
}

bool RemoteButtonManager::DeleteButton( uint32_t buttonid )
{
	return RemoteButtonManager::DeleteButton_Spec( this->m_ButtonQueue, buttonid );
}

void RemoteButtonManager::ButtonQueueChangeToOne( uint32_t buttonid )
{
	RemoteButtonManager::ButtonQueueChangeToOne_Spec( this->m_ButtonQueue, buttonid );
}

RemoteButton* RemoteButtonManager::GetButton( uint32_t buttonid ) const
{
	defButtonQueue::const_iterator it = m_ButtonQueue.begin();
	defButtonQueue::const_iterator itEnd = m_ButtonQueue.end();
	for( ; it!=itEnd; ++it )
	{
		if( (*it)->GetId() == buttonid )
		{
			return (*it);
		}
	}

	return NULL;
}

RemoteButton* RemoteButtonManager::GetFristButton() const
{
	if( !m_ButtonQueue.empty() )
	{
		return ( *m_ButtonQueue.begin() );
	}

	return NULL;
}

RemoteButton* RemoteButtonManager::UpdateButton( const RemoteButton *pButton )
{
	if( !pButton )
	{
		return NULL;
	}

	defButtonQueue::iterator it = m_ButtonQueue.begin();
	defButtonQueue::iterator itEnd = m_ButtonQueue.end();
	for( ; it!=itEnd; ++it )
	{
		RemoteButton *pCurButton = *it;

		if( pCurButton->GetId() == pButton->GetId() )
		{
			pCurButton->SetName( pButton->GetObjName() );

			return pCurButton;
		}
	}

	return NULL;
}

void RemoteButtonManager::SortUseNo()
{
	m_ButtonQueue.sort( sort_RemoteButton() );
}

// 输入的列表已经排好序，将排序号更新到成员变量里
void RemoteButtonManager::Sort( std::map<int,int> &sortlist )
{
	defButtonQueue::iterator it = m_ButtonQueue.begin();
	defButtonQueue::const_iterator itEnd = m_ButtonQueue.end();
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

	this->SortUseNo();
}

// 用已经排好序的信息列表对配置更新
void RemoteButtonManager::SortByButtonList( const defButtonQueue &ButtonList )
{
	std::map<int,int> sortlist;
	int i=1;

	defButtonQueue::const_iterator it = ButtonList.begin();
	defButtonQueue::const_iterator itEnd = ButtonList.end();
	for( ; it!=itEnd; ++it, ++i )
	{
		sortlist[ (*it)->GetId() ] = i;
	}

	this->Sort( sortlist );
}
