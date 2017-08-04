#ifndef SQLITEHELPER_H_
#define SQLITEHELPER_H_

#include "SQLiteC++.h"

class SQLiteHelper
{
private:
	std::string path;

protected:
	static SQLite::Database *db;

public:
	SQLiteHelper(void);
	~SQLiteHelper(void);

	static void FinalRelease();

	SQLite::Database* get_db() const
	{
		return db;
	}
};

class UseDbTransAction
{
public:
	UseDbTransAction( SQLite::Database *db ) : m_db( db )
	{
		if( m_db )
		{
			m_db->exec( "BEGIN TRANSACTION;" );
		}
	}

	~UseDbTransAction() { Commit(); }

	bool Commit()
	{
		if( m_db )
		{
			m_db->exec( "COMMIT;" );
			m_db = NULL;
			return true;
		}

		return false;
	}

	void Rollback()
	{
		if( m_db )
		{
			m_db->exec( "ROLLBACK TRANSACTION;" );
			m_db = NULL;
		}
	}

private:
	UseDbTransAction& operator=( const UseDbTransAction& );
	SQLite::Database *m_db;
};

#endif

