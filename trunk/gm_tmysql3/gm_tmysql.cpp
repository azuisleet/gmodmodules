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
#ifdef GMOD_BETA
	ILuaObject *G = gLua->Global();
#else
	ILuaObject *G = gLua->GetGlobal("_G");
#endif

	G->SetMember( "QUERY_SUCCESS", QUERY_SUCCESS );
	G->SetMember( "QUERY_FAIL", QUERY_FAIL );

	G->SetMember( "QUERY_FLAG_ASSOC", (float)QUERY_FLAG_ASSOC );
	G->SetMember( "QUERY_FLAG_LASTID", (float)QUERY_FLAG_LASTID );

	ILuaObject* mfunc = gLua->GetNewTable();
		mfunc->SetMember("initialize", initialize);
		mfunc->SetMember("query", query);
		mfunc->SetMember("escape", escape);
		mfunc->SetMember("setcharset", setcharset);
		mfunc->SetMember("poll", poll);
	G->SetMember( "tmysql", mfunc );
	mfunc->UnReference();

#ifndef GMOD_BETA
	G->UnReference();
#endif
	return 0;
}

int Close(lua_State *L)
{
	ILuaInterface* gLua = Lua();
	Database* mysqldb = GetMySQL( gLua );

	if ( mysqldb )
	{
		while ( !mysqldb->IsSafeToShutdown() )
		{
			DispatchCompletedQueries( gLua, mysqldb, true );
			ThreadSleep( 10 );
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

	CUtlString error;

	if ( !mysqldb->Initialize( error ) )
	{
		char buffer[1024];

		Q_snprintf( buffer, sizeof(buffer), "Error connecting to DB: %s", error.Get() );

		gLua->Push( false );
		gLua->Push( buffer );

		delete mysqldb;
		return 2;
	}

#ifdef GMOD_BETA
	ILuaObject *G = gLua->Global();
#else
	ILuaObject *G = gLua->GetGlobal("_G");
#endif
	ILuaObject* database = G->GetMember( "tmysql" );
	database->SetMemberUserDataLite( USERDATA_PTR, mysqldb );
	database->UnReference();

	// hook.Add("Think", "TMysqlPoll", tmysql.poll)
	ILuaObject *hookt = G->GetMember("hook");
	ILuaObject *addf = hookt->GetMember("Add");
	addf->Push();
	gLua->Push("Think");
	gLua->Push("TMysqlPoll");
	gLua->Push(poll);
	gLua->Call(3);

	hookt->UnReference();
	addf->UnReference();
#ifndef GMOD_BETA
	G->UnReference();
#endif

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

	CUtlString error;
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
	int callbackobj = gLua->GetType(4);
	if(callbackobj != GLua::TYPE_NIL)
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
#ifdef GMOD_BETA
	ILuaObject *G = gLua->Global();
#else
	ILuaObject *G = gLua->GetGlobal("_G");
#endif
	if (!G) return NULL;

	ILuaObject* mysql = G->GetMember( "tmysql" );

#ifndef GMOD_BETA
	G->UnReference();
#endif
	if (!mysql) return NULL;

	Database* pData = (Database*)mysql->GetMemberUserDataLite( USERDATA_PTR );
#ifndef GMOD_BETA
	gLua->Pop(); // BUG: GetMemberUserDataLite does not balance
#endif

	mysql->UnReference();
	return pData;
}

void DispatchCompletedQueries( ILuaInterface* gLua, Database* mysqldb, bool requireSync )
{
	CUtlVectorMT<CUtlVector<Query*> >& completed = mysqldb->CompletedQueries();

	// peek at the size, the query threads will only add to it, so we can do this and not end up locking it for nothing
	if( !requireSync && completed.Size() <= 0 )
		return;

	{
		AUTO_LOCK_FM( completed );

		FOR_EACH_VEC( completed, i )
		{
			Query* query = completed[i];

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

		completed.RemoveAll();
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
		gLua->Push( query->GetError() );
	}

	if ( gLua->PCall( args ) != 0 )
	{
		const char* err = gLua->GetString();
		gLua->ErrorNoHalt("tmysql callback failure: %s\n", err);
	}
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
#ifndef GMOD_BETA
	ILuaObject* resultrow = gLua->NewTemporaryObject();
#endif

	while ( row != NULL )
	{
#ifdef GMOD_BETA
		ILuaObject* resultrow = gLua->GetNewTable();
#else
		// black magic warning: we use a temp and assign it so that we avoid consuming all the temp objects and causing horrible disasters
		gLua->NewTable();
		resultrow->SetFromStack(-1);
		gLua->Pop();
#endif

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

	CUtlString error;
	if ( !database->Initialize( error ) )
	{
		printf("error: %s\n", error);
		return 0;
	}

	CUtlVectorMT<CUtlVector<Query*>>& completed = database->CompletedQueries();

	static int i = 0;
	while ( true )
	{
		{
			AUTO_LOCK_FM( completed );

			FOR_EACH_VEC( completed, i )
			{
				Query* query = completed[i];
				printf( "query: %s; callback %d; status %d time: %f\n", query->GetQuery(), query->GetCallback(), query->GetStatus(), query->GetQueryTime() );

				BuildTableFromQuery( NULL, query );
			}

			completed.RemoveAll();
		}
		Sleep( 1 );
	}
}
*/