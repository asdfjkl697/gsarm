#ifndef AddressManager_H_
#define AddressManager_H_
#pragma once

#include "DeviceAddress.h"

typedef std::list<DeviceAddress*> defAddressQueue;

class AddressManager
{
public:
	AddressManager( void );
	AddressManager( const AddressManager& RightSides );
	AddressManager& operator= ( const AddressManager &RightSides );
	virtual ~AddressManager( void );

public:
	static void CloneAddressQueue_Spec( defAddressQueue &quedest, const defAddressQueue &quesrc );
	static void DeleteAddressQueue_Spec( defAddressQueue &que );
	static bool DeleteAddress_Spec( defAddressQueue &que, uint32_t address );
	static void AddressQueueChangeToOneAddr_Spec( defAddressQueue &que, uint32_t address=0 );
	static void EncodeAddressNum_Spec( defAddressQueue &que, uint32_t firstAddressNum );

public:
	void AddAddress( DeviceAddress *addr )
	{
		m_AddressQueue.push_back(addr);
	}

	const defAddressQueue& GetAddressList() const
	{
		return this->m_AddressQueue;
	}
	DeviceAddress *GetAddress( uint32_t address ) const;

	void DeleteAddressQueue();
	bool DeleteAddress( uint32_t address );
	void AddressQueueChangeToOneAddr( uint32_t address=0 );
	void EncodeAddressNum( uint32_t firstAddressNum );

	DeviceAddress* GetFristAddress() const;

	DeviceAddress* UpdateAddress( DeviceAddress *pAddr );

	bool IsSameAllAddress( const AddressManager &other ) const;

protected:
	defAddressQueue m_AddressQueue;
};

#endif
