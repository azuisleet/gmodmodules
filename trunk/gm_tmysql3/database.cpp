#include "gm_tmysql.h"

Database::Database( const char* host, const char* user, const char* pass, const char* db, int port ) :
	m_strHost(host), m_strUser(user), m_strPass(pass), m_strDB(db), m_iPort(port),
	m_pThreadPool(NULL)
{
}

Database::~Database( void )
{

}

bool Database::Initialize( CUtlString& error )
{
	static my_bool bTrue = true;

	for( int i = NUM_CON_DEFAULT; i; i-- )
	{
		MYSQL* mysql = mysql_init(NULL);

		if ( !Connect( mysql, error ) )
		{
			return false;	
		}

		m_vecAvailableConnections.AddToTail( mysql );
		m_vecAllConnections.AddToTail( mysql );
	}

	m_pThreadPool = CreateThreadPool();

	ThreadPoolStartParams_t params;
	params.nThreads = NUM_THREADS_DEFAULT;

	if ( !m_pThreadPool->Start( params ) )
	{
		SafeRelease( m_pThreadPool );
		error.Set( "Unable to start thread pool" );

		return false;
	}

	return true;
}

bool Database::Connect( MYSQL* mysql, CUtlString& error )
{
	if ( !mysql_real_connect( mysql, m_strHost, m_strUser, m_strPass, m_strDB, m_iPort, NULL, 0 ) )
	{
		error.Set( mysql_error( mysql ) );
		return false;
	}

	return true;
}

bool Database::IsSafeToShutdown( void )
{
	bool completedDispatch;
	{
		AUTO_LOCK_FM( m_vecCompleted );
		completedDispatch = m_vecCompleted.Count() == 0;
	}

	return completedDispatch && m_pThreadPool->GetJobCount() == 0 && m_pThreadPool->NumIdleThreads() == m_pThreadPool->NumThreads();
}

void Database::Shutdown( void )
{
	if ( m_pThreadPool != NULL )
	{
		m_pThreadPool->Stop();

		DestroyThreadPool( m_pThreadPool );
		m_pThreadPool = NULL;
	}

	FOR_EACH_VEC( m_vecAllConnections, i )
	{
		mysql_close( m_vecAllConnections[i] );
	}

	m_vecAllConnections.Purge();
	m_vecAvailableConnections.Purge();
	m_vecCompleted.Purge();
}

char* Database::Escape( const char* query )
{
	size_t len = Q_strlen( query );
	char* escaped = new char[len*2+1];

	mysql_escape_string( escaped, query, len );

	return escaped;
}

bool Database::SetCharacterSet( const char* charset, CUtlString& error )
{
	FOR_EACH_VEC( m_vecAllConnections, i )
	{
		if ( mysql_set_character_set( m_vecAllConnections[i], charset ) > 0 )
		{
			error.Set( mysql_error( m_vecAllConnections[i] ) );
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
	CJob* job = m_pThreadPool->QueueCall( this, &Database::DoExecute, query );

	SafeRelease( job );
}



void Database::YieldPostCompleted( Query* query )
{
	AUTO_LOCK_FM( m_vecCompleted );

	m_vecCompleted.AddToTail( query );
}

void Database::DoExecute( Query* query )
{
	MYSQL* pMYSQL;
	
	{
		AUTO_LOCK_FM( m_vecAvailableConnections );
		Assert( m_vecAvailableConnections.Size() > 0 );

		pMYSQL = m_vecAvailableConnections.Head();
		m_vecAvailableConnections.Remove( 0 );

		Assert( pMYSQL );
	}


	const char* strquery = query->GetQuery();
	size_t len = query->GetQueryLength();

	int err = mysql_real_query( pMYSQL, strquery, len );

	if ( err > 0 )
	{
		int ping = mysql_ping( pMYSQL );
		if ( ping > 0 )
		{
			mysql_close( pMYSQL );
			if(mysql_real_connect( pMYSQL, m_strHost, m_strUser, m_strPass, m_strDB, m_iPort, NULL, 0 ))
			{
				err = mysql_real_query( pMYSQL, strquery, len );
			} else {
				err = 1;
			}
		}
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

	{
		AUTO_LOCK_FM( m_vecAvailableConnections );
		m_vecAvailableConnections.AddToTail( pMYSQL );
	}
}