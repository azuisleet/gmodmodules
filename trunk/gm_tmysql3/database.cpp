#include "gm_tmysql.h"

Database::Database( const char* host, const char* user, const char* pass, const char* db, int port ) :
	m_strHost(host), m_strUser(user), m_strPass(pass), m_strDB(db), m_iPort(port)
{
	m_taskGroup = new tbb::task_group();
}

Database::~Database( void )
{
	delete m_taskGroup;
}

bool Database::Initialize( CUtlString& error )
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

		m_vecAvailableConnections.AddToTail( mysql );
		m_vecAllConnections.AddToTail( mysql );
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

void Database::FinishAllQueries( void )
{
	tbb::task_group_status status = m_taskGroup->wait();
}

void Database::Shutdown( void )
{
	m_taskGroup->cancel();
	m_taskGroup->wait();

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
	m_taskGroup->run( DoQueryTask( this, query ) );
}



void Database::YieldPostCompleted( Query* query )
{
	AUTO_LOCK_FM( m_vecCompleted );

	m_vecCompleted.AddToTail( query );
}


void Database::DoExecute( Query* query )
{
#ifdef USE_THREAD_LOCAL
	static THREAD_LOCAL MYSQL* pMYSQL;
#else
	static CThreadLocalPtr<MYSQL> pMYSQL;
#endif

	if ( pMYSQL == NULL )
	{
		AUTO_LOCK_FM( m_vecAvailableConnections );

		Assert( m_vecAvailableConnections.Size() > 0 );

		if ( m_vecAvailableConnections.Size() <= 0 )
		{
			Msg( "No available connections!\n" );
		}

		pMYSQL = m_vecAvailableConnections.Head();
		m_vecAvailableConnections.Remove( 0 );

		mysql_thread_init();

		Assert( pMYSQL );
	}

	const char* strquery = query->GetQuery();
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