#include "mysql.h"
#define QUERY_SUCCESS true
#define QUERY_FAIL false

#define QUERY_FLAG_ASSOC 1
#define QUERY_FLAG_LASTID 2

#define NUM_THREADS_DEFAULT 2
#define NUM_CON_DEFAULT NUM_THREADS_DEFAULT

#undef ENABLE_QUERY_TIMERS
#undef USE_THREAD_LOCAL

#ifdef ENABLE_QUERY_TIMERS
class Timer
{
public:
	Timer()
	{
		QueryPerformanceFrequency(&frequency);
		QueryPerformanceCounter(&startTick);
	}

	double GetElapsedSeconds( void )
	{
		LARGE_INTEGER currentTick;

		QueryPerformanceCounter(&currentTick);
		return (double)(currentTick.QuadPart - startTick.QuadPart) / frequency.QuadPart;
	}

private:
	LARGE_INTEGER frequency;
	LARGE_INTEGER startTick;
};
#endif

class Query;
class Database;

class DoQueryTask
{
private:
	Query* m_pQuery;
	Database* m_pDatabase;

public:
	DoQueryTask( Database* database, Query* query ) : m_pDatabase( database ), m_pQuery( query ) {}

	void operator() () const;
};

class Query
{
public:
	Query( const char* query, int callback = -1, int flags = 0, int callbackref = -1 ) : m_iCallback( callback ), m_iFlags( flags ), m_iCallbackRef( callbackref ), m_iLastID( 0 )
	{
		m_strQuery.assign( query );
	}

	~Query( void )
	{
	}

	const std::string& GetQuery( void ) { return m_strQuery; }
	size_t		GetQueryLength( void ) { return m_strQuery.length(); }

	int			GetFlags( void ) { return m_iFlags; }
	int			GetCallback( void ) { return m_iCallback; }
	int			GetCallbackRef( void ) { return m_iCallbackRef; }

	void		SetError( const char* error ) { m_strError.assign( error ); }
	const std::string& GetError( void ) { return m_strError; }

	void		SetStatus( bool status ) { m_bStatus = status; }
	bool		GetStatus( void ) const { return m_bStatus; }

	void		SetResult( MYSQL_RES* result ) { m_myResult = result; }
	MYSQL_RES*	GetResult( void ) const { return m_myResult; }

	void		SetLastID( int ID ) { m_iLastID = ID; }
	int			GetLastID( void ) const { return m_iLastID; }

#ifdef ENABLE_QUERY_TIMERS
	double		GetQueryTime( void ) { return m_queryTimer.GetElapsedSeconds(); }
#else
	double		GetQueryTime( void ) { return 0; }
#endif

private:

	std::string	m_strQuery;
	int			m_iCallback;
	int			m_iCallbackRef;
	
	int			m_iFlags;
	bool		m_bStatus;

	MYSQL_RES*	m_myResult;
	int			m_iLastID;

	std::string	m_strError;

#ifdef ENABLE_QUERY_TIMERS
	Timer		m_queryTimer;
#endif
};

class Database
{
	friend class DoQueryTask;

public:
	Database( const char* host, const char* user, const char* pass, const char* db, int port );
	~Database( void );

	bool	Initialize( std::string& error );
	bool	WaitForSafeShutdown( void );
	void	Shutdown( void );

	bool		SetCharacterSet( const char* charset, std::string& error );
	char*		Escape( const char* query );
	void		QueueQuery( const char* query, int callback = -1, int flags = 0, int callbackref = -1 );

	std::deque< Query* >& CompletedQueries( void ) { return m_vecCompleted; }
	recursive_mutex& CompletedMutex( void ) { return m_CompletedMutex; }

private:
	bool Connect( MYSQL* mysql, std::string& error );

	void QueueQuery( Query* query );

	void DoExecute( Query* query );
	void YieldPostCompleted( Query* query );

	pool*	m_pThreadPool;

	mutable recursive_mutex m_CompletedMutex;

	mutable recursive_mutex m_AvailableMutex;

	std::deque< Query* > m_vecCompleted;
	std::deque< MYSQL* > m_vecAvailableConnections;
	std::vector< MYSQL* > m_vecAllConnections;

	std::string		m_strHost;
	std::string		m_strUser;
	std::string		m_strPass;
	std::string		m_strDB;
	int				m_iPort;
};