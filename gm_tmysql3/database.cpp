#include "gm_tmysql.h"

Database::Database( const char* host, const char* user, const char* pass, const char* db, int port ) :
	m_strHost(host), m_strUser(user), m_strPass(pass), m_strDB(db), m_iPort(port)
{
	work.reset(new asio::io_service::work(io_service));
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

		if ( !Connect( mysql, error ) )
		{
			return false;
		}

		m_vecAvailableConnections.push_back( mysql );
	}

	for( unsigned int i = 0; i < NUM_THREADS_DEFAULT; ++i )
	{
		thread_group.push_back(std::thread(
			[&]()
			{
				io_service.run();
			}));
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

void Database::Shutdown( void )
{
	work.reset();

	for( auto iter = thread_group.begin(); iter != thread_group.end(); ++iter )
	{
		iter->join();
	}

	assert(io_service.stopped());
}

std::size_t Database::RunShutdownWork( void )
{
	io_service.reset();
	return io_service.run();
}

void Database::Release( void )
{
	assert(io_service.stopped());

	for( auto iter = m_vecAvailableConnections.begin(); iter != m_vecAvailableConnections.end(); ++iter )
	{
		mysql_close( *iter );
	}

	m_vecAvailableConnections.clear();
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
	for( auto iter = m_vecAvailableConnections.begin(); iter != m_vecAvailableConnections.end(); ++iter )
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
	io_service.post(std::bind(&Database::DoExecute, this, query));
}

Query* Database::GetCompletedQueries()
{
	return m_completedQueries.pop_all();
}

void Database::PushCompleted( Query* query )
{
	m_completedQueries.push( query );
}

void Database::DoExecute( Query* query )
{
	MYSQL* pMYSQL = GetAvailableConnection();

	const char* strquery = query->GetQuery().c_str();
	size_t len = query->GetQueryLength();

	int err = mysql_real_query( pMYSQL, strquery, len );

	if ( err > 0 )
	{
		int ping = mysql_ping( pMYSQL );
		if ( ping > 0 )
		{
			std::lock_guard<std::recursive_mutex> guard(m_AvailableMutex);

			mysql_close( pMYSQL );
			pMYSQL = mysql_init( NULL );
			if(mysql_real_connect( pMYSQL, m_strHost.c_str(), m_strUser.c_str(), m_strPass.c_str(), m_strDB.c_str(), m_iPort, NULL, 0 ))
			{
				err = mysql_real_query( pMYSQL, strquery, len );

				if ( err > 0 )
				{
					query->SetError( mysql_error( pMYSQL ) );
					query->SetStatus( QUERY_FAIL );
					query->SetResult( NULL );
				}
			} 
			else
			{
				query->SetError( "Unable to reconnect to database" );
				query->SetStatus( QUERY_FAIL );
				query->SetResult( NULL );
			}
		}
		else
		{
			query->SetError( mysql_error( pMYSQL ) );
			query->SetStatus( QUERY_FAIL );
			query->SetResult( NULL );
		}
	}

	if ( err == 0 )
	{
		query->SetStatus( QUERY_SUCCESS );
		query->SetResult( mysql_store_result( pMYSQL ) );

		if ( query->GetFlags() & QUERY_FLAG_LASTID )
		{
			query->SetLastID( mysql_insert_id( pMYSQL ) );
		}
	}

	PushCompleted(query);

	ReturnConnection( pMYSQL );
}