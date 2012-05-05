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

class Query
{
public:
	Query( const char* query, int callback = -1, int flags = 0, int callbackref = -1 ) : m_strQuery( query ), m_iCallback( callback ), m_iFlags( flags ), m_iCallbackRef( callbackref ), m_iLastID( 0 )
	{
	}

	~Query( void )
	{
	}

	const char* GetQuery( void ) { return m_strQuery; }
	int			GetQueryLength( void ) { return m_strQuery.Length(); }

	int			GetFlags( void ) { return m_iFlags; }
	int			GetCallback( void ) { return m_iCallback; }
	int			GetCallbackRef( void ) { return m_iCallbackRef; }

	void		SetError( const char* error ) { m_strError.Set( error ); }
	const char* GetError( void ) { return m_strError.Get(); }

	void		SetStatus( bool status ) { m_bStatus = status; }
	bool		GetStatus( void ) { return m_bStatus; }

	void		SetResult( MYSQL_RES* result ) { m_myResult = result; }
	MYSQL_RES*	GetResult( void ) { return m_myResult; }

	void		SetLastID( int ID ) { m_iLastID = ID; }
	int			GetLastID( void ) { return m_iLastID; }

#ifdef ENABLE_QUERY_TIMERS
	double		GetQueryTime( void ) { return m_queryTimer.GetElapsedSeconds(); }
#else
	double		GetQueryTime( void ) { return 0; }
#endif

private:

	CUtlString	m_strQuery;
	int			m_iCallback;
	int			m_iCallbackRef;
	
	int			m_iFlags;
	bool		m_bStatus;

	MYSQL_RES*	m_myResult;
	int			m_iLastID;

	CUtlString	m_strError;

#ifdef ENABLE_QUERY_TIMERS
	Timer		m_queryTimer;
#endif
};

class Database
{
public:
	Database( const char* host, const char* user, const char* pass, const char* db, int port );
	~Database( void );

	bool	Initialize( CUtlString& error );
	bool	IsSafeToShutdown( void );
	void	Shutdown( void );

	bool		SetCharacterSet( const char* charset, CUtlString& error );
	char*		Escape( const char* query );
	void		QueueQuery( const char* query, int callback = -1, int flags = 0, int callbackref = -1 );

	CUtlVectorMT<CUtlVector<Query*> >& CompletedQueries( void ) { return m_vecCompleted; }

private:
	bool Connect( MYSQL* mysql, CUtlString& error );

	void QueueQuery( Query* query );

	void DoExecute( Query* query );
	void YieldPostCompleted( Query* query );

	IThreadPool*	m_pThreadPool;

	CUtlVectorMT<CUtlVector<Query*> >						m_vecCompleted;
	CUtlVectorMT<CUtlVector<MYSQL*> >						m_vecAvailableConnections;
	CUtlVector<MYSQL*>										m_vecAllConnections;

	CUtlString		m_strHost;
	CUtlString		m_strUser;
	CUtlString		m_strPass;
	CUtlString		m_strDB;
	uint			m_iPort;

	CThreadLocalPtr<MYSQL> pLocalMYSQL;
};