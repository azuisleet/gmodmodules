#include "gm_tmysql.h"

Database::Database( const char* host, const char* user, const char* pass, const char* db, int port ) :
	m_strHost(host), m_strUser(user), m_strPass(pass), m_strDB(db), m_iPort(port),
	m_pThreadPool(NULL)
{
}

Database::~Database( void )
{
}

bool Database::Initialize( std::string& error )
{
	static my_bool bTrue = true;

	for( int i = NUM_CON_DEFAULT; i; i-- )
	{
		MYSQL* mysql = mysql_init(NULL);
		mysql_options(mysql, MYSQL_OPT_RECONNECT, &bTrue);

		if ( !Connect( mysql, error ) )
		{
			return false;	
		}

		m_vecAvailableConnections.push_back( mysql );
		m_vecAllConnections.push_back( mysql );
	}

	m_pThreadPool = new pool( NUM_THREADS_DEFAULT );

	if ( !m_pThreadPool )
	{
		error.assign( "Unable to start thread pool" );

		return false;
	}

	return true;
}

bool Database::Connect( MYSQL* mysql, std::string& error )
{
	if ( !mysql_real_connect( mysql, m_strHost.c_str(), m_strUser.c_str(), m_strPass.c_str(), m_strDB.c_str(), m_iPort, NULL, 0 ) )
	{
		error.assign( mysql_error( mysql ) );
		return false;
	}

	return true;
}

inline boost::xtime delay(int secs, int msecs=0, int nsecs=0)
{
	const int MILLISECONDS_PER_SECOND = 1000;
	const int NANOSECONDS_PER_SECOND = 1000000000;
	const int NANOSECONDS_PER_MILLISECOND = 1000000;

	boost::xtime xt;
	if (boost::TIME_UTC != boost::xtime_get (&xt, boost::TIME_UTC))
		assert ("boost::xtime_get != boost::TIME_UTC");

	nsecs += xt.nsec;
	msecs += nsecs / NANOSECONDS_PER_MILLISECOND;
	secs += msecs / MILLISECONDS_PER_SECOND;
	nsecs += (msecs % MILLISECONDS_PER_SECOND) *
		NANOSECONDS_PER_MILLISECOND;
	xt.nsec = nsecs % NANOSECONDS_PER_SECOND;
	xt.sec += secs + (nsecs / NANOSECONDS_PER_SECOND);

	return xt;
} 

bool Database::WaitForSafeShutdown( void )
{
	bool waitCompletion = m_pThreadPool->wait( delay( 0, 50 ) );
	bool completedDispatch;

	{
		recursive_mutex::scoped_lock lock( m_CompletedMutex );
		completedDispatch = m_vecCompleted.empty();
	}
	
	return waitCompletion && completedDispatch;
}

void Database::Shutdown( void )
{
	if ( m_pThreadPool != NULL )
	{
		m_pThreadPool->wait();

		delete m_pThreadPool;
		m_pThreadPool = NULL;
	}

	for( std::vector< MYSQL* >::const_iterator iter = m_vecAllConnections.begin(); iter != m_vecAllConnections.end(); ++iter )
	{
		mysql_close( *iter );
	}

	m_vecAllConnections.clear();
	m_vecAvailableConnections.clear();
	m_vecCompleted.clear();
}

char* Database::Escape( const char* query )
{
	size_t len = strlen( query );
	char* escaped = new char[len*2+1];

	mysql_escape_string( escaped, query, len );

	return escaped;
}

bool Database::SetCharacterSet( const char* charset, std::string& error )
{
	for( std::vector< MYSQL* >::const_iterator iter = m_vecAllConnections.begin(); iter != m_vecAllConnections.end(); ++iter )
	{
		if ( mysql_set_character_set( *iter, charset ) > 0 )
		{
			error.assign( mysql_error( *iter ) );
			return false;
		}
	}

	return true;
}

void Database::QueueQuery( const char* query, int callback, int flags, int callbackref )
{
	Query* newquery = new Query( query, callback, flags, callbackref );

	QueueQuery( newquery );
}

void Database::QueueQuery( Query* query )
{
	if ( m_pThreadPool != NULL )
	{
		m_pThreadPool->schedule( DoQueryTask( this, query ) );
	}
}



void Database::YieldPostCompleted( Query* query )
{
	recursive_mutex::scoped_lock lock( m_CompletedMutex );

	m_vecCompleted.push_back( query );
}

void Database::DoExecute( Query* query )
{
	MYSQL* pMYSQL = pLocalMYSQL.get();

	if ( pMYSQL == NULL )
	{
		recursive_mutex::scoped_lock lock( m_AvailableMutex );

		pMYSQL = m_vecAvailableConnections.front();
		m_vecAvailableConnections.pop_front();

		pLocalMYSQL.reset( pMYSQL );
	}

	const char* strquery = query->GetQuery().c_str();
	size_t len = query->GetQueryLength();

	int err = mysql_real_query( pMYSQL, strquery, len );

	if ( err > 0 )
	{
		mysql_ping( pMYSQL );

		err = mysql_real_query( pMYSQL, strquery, len );
	}

	if ( err > 0 )
	{
		query->SetError( mysql_error( pMYSQL ) );
		query->SetStatus( QUERY_FAIL );
		query->SetResult( NULL );
	}
	else
	{
		query->SetStatus( QUERY_SUCCESS );
		query->SetResult( mysql_store_result( pMYSQL ) );

		if ( query->GetFlags() & QUERY_FLAG_LASTID )
		{
			query->SetLastID( mysql_insert_id( pMYSQL ) );
		}
	}

	YieldPostCompleted( query );
}

void DoQueryTask::operator() () const
{
	m_pDatabase->DoExecute( m_pQuery );
}