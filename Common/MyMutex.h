//	MyMutex.h  Wrapped CMutex with automatic locking.
//
#pragma once

class CMyMutex : public CMutex
{
public:
	CMyMutex()
	: CMutex()
	{
	}
	CMyMutex( LPCTSTR in_name )
	: CMutex( FALSE, in_name )
	{
	}

	virtual ~CMyMutex() {}
};

class CMutexLock
{
public:
	CMutexLock( CMyMutex* mutex )
	{
		m_oMutex = mutex;
		mutex->Lock();
	}

	CMutexLock( CMyMutex& mutex )
		: m_oMutex( &mutex )
	{
		mutex.Lock();
	}

	virtual ~CMutexLock()
	{
		m_oMutex->Unlock();
	}

	CMyMutex * m_oMutex;
};