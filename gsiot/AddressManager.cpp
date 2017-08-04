#include "AddressManager.h"


AddressManager::AddressManager(void)
{
}

AddressManager& AddressManager::operator= ( const AddressManager &RightSides )
{
	if( this == &RightSides )
	{
		return *this;
	}
	
	DeleteAddressQueue();

	CloneAddressQueue_Spec( this->m_AddressQueue, RightSides.m_AddressQueue );

	return *this;
}

AddressManager::AddressManager( const AddressManager &RightSides )
{
	*this = RightSides;
}

AddressManager::~AddressManager(void)
{
	DeleteAddressQueue();
}

void AddressManager::DeleteAddressQueue_Spec( defAddressQueue &que )
{
	while( !que.empty() )
	{
		DeviceAddress *addr = que.front();
		delete(addr);
		que.pop_front();
	}
}

bool AddressManager::DeleteAddress_Spec( defAddressQueue &que, uint32_t address )
{
	defAddressQueue::const_iterator it = que.begin();
	defAddressQueue::const_iterator itEnd = que.end();
	for( ; it!=itEnd; ++it ){
		DeviceAddress *addr = (*it);
		if( addr->GetAddress() == address ){
			delete(addr);
			//que.erase(it); //jyc20160603
			return true;
		}
	}
	return false;
}

void AddressManager::CloneAddressQueue_Spec( defAddressQueue &quedest, const defAddressQueue &quesrc )
{
	defAddressQueue::const_iterator it = quesrc.begin();
	defAddressQueue::const_iterator itEnd = quesrc.end();
	for( ; it!=itEnd; ++it )
	{
		quedest.push_back( (DeviceAddress*)(*it)->clone() );
	}
}

void AddressManager::AddressQueueChangeToOneAddr_Spec( defAddressQueue &que, uint32_t address )
{
	bool isFound = false;
	while( que.size()>1 )
	{
		DeviceAddress *addr = que.back();
		que.pop_back();

		if( !isFound && address>0 )
		{
			if( addr->GetAddress() == address )
			{
				isFound = true;
				que.push_front(addr);
				continue;
			}
		}

		delete(addr);
	}
}

void AddressManager::EncodeAddressNum_Spec( defAddressQueue &que, uint32_t firstAddressNum )
{
	defAddressQueue::const_iterator it = que.begin();
	defAddressQueue::const_iterator itEnd = que.end();
	for( ; it!=itEnd; ++it )
	{
		(*it)->SetAddress( firstAddressNum++ );
	}
}

void AddressManager::DeleteAddressQueue()
{
	AddressManager::DeleteAddressQueue_Spec( this->m_AddressQueue );
}

bool AddressManager::DeleteAddress( uint32_t address )
{
	return AddressManager::DeleteAddress_Spec( this->m_AddressQueue, address );
}

void AddressManager::AddressQueueChangeToOneAddr( uint32_t address )
{
	AddressManager::AddressQueueChangeToOneAddr_Spec( this->m_AddressQueue, address );
}

void AddressManager::EncodeAddressNum( uint32_t firstAddressNum )
{
	AddressManager::EncodeAddressNum_Spec( this->m_AddressQueue, firstAddressNum );
}

DeviceAddress* AddressManager::GetFristAddress() const
{
	if( !m_AddressQueue.empty() )
	{
		return ( *m_AddressQueue.begin() );
	}

	return NULL;
}

DeviceAddress *AddressManager::GetAddress(uint32_t address) const
{
	defAddressQueue::const_iterator it = m_AddressQueue.begin();
	defAddressQueue::const_iterator itEnd = m_AddressQueue.end();
	for( ; it!=itEnd; ++it )
	{
		if( (*it)->GetAddress()== address )
		{
			return (*it);
		}
	}

	return NULL;
}

// return NULL Failed, else success
DeviceAddress* AddressManager::UpdateAddress( DeviceAddress *pAddr )
{
	if( !pAddr )
		return NULL;

	defAddressQueue::const_iterator it = m_AddressQueue.begin();
	defAddressQueue::const_iterator itEnd = m_AddressQueue.end();
	for( ; it!=itEnd; ++it )
	{
		if( (*it)->GetAddress()== pAddr->GetAddress() )
		{
			DeviceAddress *pCurAddr = (*it);

			pCurAddr->SetName( pAddr->GetName() );
			pCurAddr->SetDefualtValue( pAddr->GetDefualtValue() );
			pCurAddr->SetMinValue( pAddr->GetMinValue() );
			pCurAddr->SetMaxValue( pAddr->GetMaxValue() );

			return pCurAddr;
		}
	}

	return NULL;
}

bool AddressManager::IsSameAllAddress( const AddressManager &other ) const
{
	const defAddressQueue& OtherAddressQueue = other.GetAddressList();

	if( m_AddressQueue.size() != OtherAddressQueue.size() )
		return false;

	defAddressQueue::const_iterator it = m_AddressQueue.begin();
	defAddressQueue::const_iterator itEnd = m_AddressQueue.end();

	defAddressQueue::const_iterator itOther = OtherAddressQueue.begin();
	defAddressQueue::const_iterator itOtherEnd = OtherAddressQueue.end();

	for( ; it!=itEnd && itOther!=itOtherEnd; ++it, ++itOther )
	{
		if( (*it)->GetAddress() != (*itOther)->GetAddress() )
		{
			return false;
		}
	}

	return true;
}

