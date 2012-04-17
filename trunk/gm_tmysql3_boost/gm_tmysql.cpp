#include "gm_tmysql.h"

#define USERDATA_PTR "p_MySQLdb"

GMOD_MODULE(Start, Close);

LUA_FUNCTION( initialize );
LUA_FUNCTION( escape );
LUA_FUNCTION( setcharset );
LUA_FUNCTION( query );
LUA_FUNCTION( poll );

void DispatchCompletedQueries( ILuaInterface* gLua, Database* mysqldb, bool requireSync );
bool PopulateTableFromQuery( ILuaInterface* gLua, ILuaObject* table, Query* query );
void HandleQueryCallback( ILuaInterface* gLua, Query* query );
Database* GetMySQL( ILuaInterface* gLua );

int Start(lua_State *L)
{
	mysql_library_init( 0, NULL, NULL );

	ILuaInterface *gLua = Lua();

	gLua->SetGlobal( "QUERY_SUCCESS", QUERY_SUCCESS );
	gLua->SetGlobal( "QUERY_FAIL", QUERY_FAIL );

	gLua->SetGlobal( "QUERY_FLAG_ASSOC", (float)QUERY_FLAG_ASSOC );
	gLua->SetGlobal( "QUERY_FLAG_LASTID", (float)QUERY_FLAG_LASTID );

	ILuaObject* mfunc = gLua->GetNewTable();
		mfunc->SetMember("initialize", initialize);
		mfunc->SetMember("query", query);
		mfunc->SetMember("escape", escape);
		mfunc->SetMember("setcharset", setcharset);
		mfunc->SetMember("poll", poll);
	gLua->SetGlobal( "tmysql", mfunc );
	mfunc->UnReference();

	return 0;
}

int Close(lua_State *L)
{
	ILuaInterface* gLua = Lua();
	Database* mysqldb = GetMySQL( gLua );

	if ( mysqldb )
	{
		while ( !mysqldb->WaitForSafeShutdown() )
		{
			DispatchCompletedQueries( gLua, mysqldb, true );
		}

		mysqldb->Shutdown();

		delete mysqldb;
	}

	mysql_library_end();

	return 0;
}

LUA_FUNCTION( initialize )
{
	ILuaInterface* gLua = Lua();

	if ( GetMySQL( gLua ) )
		return 0;

	gLua->CheckType(1, GLua::TYPE_STRING);
	gLua->CheckType(2, GLua::TYPE_STRING);
	gLua->CheckType(3, GLua::TYPE_STRING);
	gLua->CheckType(4, GLua::TYPE_STRING);
	gLua->CheckType(5, GLua::TYPE_NUMBER);

	const char* host = gLua->GetString(1);
	const char* user = gLua->GetString(2);
	const char* pass = gLua->GetString(3);
	const char* db = gLua->GetString(4);
	int port = gLua->GetInteger(5);

	if(port == 0)
		port = 3306;

	Database* mysqldb = new Database( host, user, pass, db, port );

	std::string error;

	if ( !mysqldb->Initialize( error ) )
	{
		try
		{
			std::string ferr = str( format( "Error connecting to DB: %s" ) % error );

			gLua->Msg( "%s\n", ferr.c_str() );

			gLua->Push( false );
			gLua->Push( error.c_str() );

			return 2;
		}
		catch(std::exception e)
		{
			gLua->Push( false );

			return 1;
		}
	}

	ILuaObject* database = gLua->GetGlobal( "tmysql" );
	database->SetMemberUserDataLite( USERDATA_PTR, mysqldb );

	database->UnReference();

	// hook.Add("Think", "TMysqlPoll", tmysql.poll)
	ILuaObject *hookt = gLua->GetGlobal("hook");
	ILuaObject *addf = hookt->GetMember("Add");
	addf->Push();
	gLua->Push("Think");
	gLua->Push("TMysqlPoll");
	gLua->Push(poll);
	gLua->Call(3);

	hookt->UnReference();
	addf->UnReference();

	gLua->Push( true );
	return 1;
}

LUA_FUNCTION( escape )
{
	ILuaInterface* gLua = Lua();
	Database* mysqldb = GetMySQL( gLua );

	if ( !mysqldb )
		return 0;

	gLua->CheckType( 1, GLua::TYPE_STRING );

	const char* query = gLua->GetString( 1 );

	char* escaped = mysqldb->Escape( query );
	gLua->Push( escaped );

	delete escaped;
	return 1;
}

LUA_FUNCTION( setcharset )
{
	ILuaInterface* gLua = Lua();
	Database* mysqldb = GetMySQL( gLua );

	if ( !mysqldb )
		return 0;

	gLua->CheckType( 1, GLua::TYPE_STRING );

	const char* set = gLua->GetString( 1 );

	std::string error;
	mysqldb->SetCharacterSet( set, error );

	return 0;
}

LUA_FUNCTION( query )
{
	ILuaInterface* gLua = Lua();
	Database* mysqldb = GetMySQL( gLua );

	if ( !mysqldb )
		return 0;

	gLua->CheckType( 1, GLua::TYPE_STRING );

	const char* query = gLua->GetString( 1 );

	int callbackfunc = -1;
	if(gLua->GetType(2) == GLua::TYPE_FUNCTION)
	{
		callbackfunc = gLua->GetReference(2);
	}

	int flags = gLua->GetInteger(3);

	int callbackref = -1;
	if(gLua->GetStackTop() == 5)
	{
		callbackref = gLua->GetReference(4);
	}

	mysqldb->QueueQuery( query, callbackfunc, flags, callbackref );
	return 0;
}


LUA_FUNCTION( poll )
{
	ILuaInterface* gLua = Lua();
	Database* mysqldb = GetMySQL( gLua );

	if ( !mysqldb )
		return 0;

	DispatchCompletedQueries( gLua, mysqldb, false );
	return 0;
}

Database* GetMySQL( ILuaInterface* gLua )
{
	ILuaObject* mysql = gLua->GetGlobal( "tmysql" );
	if (!mysql) return NULL;

	Database* pData = (Database*)mysql->GetMemberUserDataLite( USERDATA_PTR );
	gLua->Pop(); // BUG: GetMemberUserDataLite does not balance

	mysql->UnReference();
	return pData;
}

void DispatchCompletedQueries( ILuaInterface* gLua, Database* mysqldb, bool requireSync )
{
	typedef std::deque< Query* > QueryCollection;

	QueryCollection& completed = mysqldb->CompletedQueries();
	recursive_mutex& mutex = mysqldb->CompletedMutex();

	// peek at the size, the query threads will only add to it, so we can do this and not end up locking it for nothing
	if ( !requireSync && completed.empty() )
		return;

	{
		recursive_mutex::scoped_lock lock( mutex );

		for( QueryCollection::const_iterator iter = completed.begin(); iter != completed.end(); ++iter )
		{
			Query* query = *iter;

			if ( query->GetCallback() >= 0 )
			{
				HandleQueryCallback( gLua, query );
			}

			if ( query->GetResult() != NULL )
			{
				mysql_free_result( query->GetResult() );
			}

			delete query;
		}

		completed.clear();
	}
}

void HandleQueryCallback( ILuaInterface* gLua, Query* query )
{
	ILuaObject *resultTable = gLua->GetNewTable();

	if( !PopulateTableFromQuery( gLua, resultTable, query ) )
	{
		gLua->Error("Unable to populate result table");
	}

	gLua->PushReference( query->GetCallback() );
	gLua->FreeReference( query->GetCallback() );

	int args = 3;
	if( query->GetCallbackRef() >= 0)
	{
		args = 4;
		gLua->PushReference( query->GetCallbackRef() );
		gLua->FreeReference( query->GetCallbackRef() );
	}

	gLua->Push( resultTable );
	resultTable->UnReference();
	gLua->Push( query->GetStatus() );

	if ( query->GetStatus() )
	{
		gLua->Push( (float)query->GetLastID() );
	}
	else
	{
		gLua->Push( query->GetError().c_str() );
	}

	gLua->Call(args);
}

bool PopulateTableFromQuery( ILuaInterface* gLua, ILuaObject* table, Query* query )
{
	MYSQL_RES* result = query->GetResult();

	// no result to push, continue, this isn't fatal
	if ( result == NULL )
		return true;

	MYSQL_FIELD* fields;
	MYSQL_ROW row = mysql_fetch_row( result );
	int field_count = mysql_num_fields( result );

	if ( query->GetFlags() & QUERY_FLAG_ASSOC )
	{
		fields = mysql_fetch_fields( result );

		if ( fields == NULL )
			return false;
	}

	int rowid = 1;
	ILuaObject* resultrow = gLua->NewTemporaryObject();

	while ( row != NULL )
	{
		// black magic warning: we use a temp and assign it so that we avoid consuming all the temp objects and causing horrible disasters
		gLua->NewTable();
		resultrow->SetFromStack(-1);
		gLua->Pop();

		for ( int i = 0; i < field_count; i++ )
		{
			if ( query->GetFlags() & QUERY_FLAG_ASSOC )
			{
				resultrow->SetMember( fields[i].name, row[i] );
			} 
			else
			{
				resultrow->SetMember( (float)i+1, row[i] );
			}
		}

		table->SetMember( (float)rowid, resultrow );
		resultrow->UnReference();

		row = mysql_fetch_row( result );
		rowid++;
	}

	return true;
}

/*
int main(int argc, char** argv)
{
	Database* database = new Database("127.0.0.1", "root", "", "test", 3306);

	std::string error;
	if ( !database->Initialize( error ) )
	{
		printf("error: %s\n", error.c_str());
		return 0;
	}

}
*/