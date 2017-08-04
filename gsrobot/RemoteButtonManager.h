#pragma once
#include "typedef.h"
#include "gloox/tag.h"
#include "RemoteButtonClass.h"

using namespace gloox;

typedef std::list<RemoteButton*> defButtonQueue;

class RemoteButtonManager
{
public:
	RemoteButtonManager(void);
	RemoteButtonManager( const RemoteButtonManager& RightSides );
	RemoteButtonManager& operator= ( const RemoteButtonManager &RightSides );
	~RemoteButtonManager(void);

public:
	static void CloneButtonQueue_Spec( defButtonQueue &quedest, const defButtonQueue &quesrc );
	static void DeleteButtonQueue_Spec( defButtonQueue &que );
	static bool DeleteButton_Spec( defButtonQueue &que, uint32_t buttonid );
	static void ButtonQueueChangeToOne_Spec( defButtonQueue &que, uint32_t buttonid=0 );

public:
	
	void UntagAllBtn( const Tag* iSrc );
	void tagAllBtn( Tag* iDest, const struTagParam &TagParam ) const;

	void AddButton( RemoteButton *pButton )
	{
		m_ButtonQueue.push_back(pButton);
	}

	RemoteButton* GetButton( uint32_t buttonid ) const;

	const defButtonQueue& GetButtonList() const
	{
		return this->m_ButtonQueue;
	}
	
	uint32_t GetEnableCount() const
	{
		uint32_t count = 0;
		defButtonQueue::const_iterator it = m_ButtonQueue.begin();
		defButtonQueue::const_iterator itEnd = m_ButtonQueue.end();
		for( ; it!=itEnd; ++it )
		{
			RemoteButton *pCurButton = *it;

			if( pCurButton->GetEnable() )
			{
				count++;
			}
		}

		return count;
	}

	void DetachAll()
	{
		m_ButtonQueue.clear();
	}

	void DeleteButtonQueue();
	bool DeleteButton( uint32_t buttonid );
	void ButtonQueueChangeToOne( uint32_t buttonid=0 );

	RemoteButton* GetFristButton() const;

	RemoteButton* UpdateButton( const RemoteButton *pButton );

	void SortUseNo();
	void Sort( std::map<int,int> &sortlist ); //<ID,sort_no>
	void SortByButtonList( const defButtonQueue &ButtonList );

protected:
	defButtonQueue m_ButtonQueue;
};

