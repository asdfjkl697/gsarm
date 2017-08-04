#pragma once

#define macUseGSObjLocker(obj) UseGSObjLocker uselock(obj->m_ObjLocker)

class GSObjLocker
{
public:
	GSObjLocker(void) : m_lock(0){};
	~GSObjLocker(void){};
	
	void lock()
	{
		m_lock = 1;
	};

    bool islock()
	{
		return ( 0 != m_lock );
	}

	void unlock()
	{
		m_lock = 0;
	};

protected:
	int m_lock;
};

class UseGSObjLocker
{
public:
	UseGSObjLocker( GSObjLocker& locker ) : m_locker( locker ) { m_locker.lock(); }
	~UseGSObjLocker() { m_locker.unlock(); }

private:
	UseGSObjLocker& operator=( const UseGSObjLocker& );
	GSObjLocker& m_locker;
};
