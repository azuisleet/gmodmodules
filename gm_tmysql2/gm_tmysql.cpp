#include "gm_tmysql.h"

#define NUM_THREADS_DEFAULT 2
#define NUM_CON_DEFAULT 2

GMOD_MODULE(Start, Close)

LUA_FUNCTION(minit);
LUA_FUNCTION(query);
LUA_FUNCTION(poll);
LUA_FUNCTION(escape);
LUA_FUNCTION(setcharset);

Database* GetMySQL( ILuaInterface* gLua )
{
	ILuaObject* mysql = gLua->GetGlobal( "tmysql" );
	if (!mysql) return NULL;

	Database* pData = (Database*)mysql->GetMemberUserDataLite( "p_MySQLdb" );
	return pData;
}

SyncList<DBQuery *>* GetCompletedQueries( ILuaInterface *gLua )
{
	ILuaObject* mysql = gLua->GetGlobal( "tmysql" );
	if (!mysql) return NULL;

	SyncList<DBQuery *>* pData = (SyncList<DBQuery *>*)mysql->GetMemberUserDataLite( "p_CompletedQueries" );
	return pData;
}

// from here it places the query in a list, one of the worker threads will pick it up,
// the thread will choose a mysql connection, and when it's done, calls Lua

// tmysql.initialize(host, user, pass, database, [port], [number of connections], [number of threads])
LUA_FUNCTION(minit)
{
	ILuaInterface *gLua = Lua();
	if(GetMySQL(gLua))
		return 0;

	const char *host = gLua->GetString(1);
	const char *user = gLua->GetString(2);
	const char *pass = gLua->GetString(3);
	const char *db = gLua->GetString(4);
	int port = gLua->GetInteger(5);
	int numConns = gLua->GetInteger(6);
	int numThreads = gLua->GetInteger(7);
	
	if(port == 0)
		port = 3306;
	if(numConns == 0)
		numConns = NUM_CON_DEFAULT;
	if(numThreads == 0)
		numThreads = NUM_THREADS_DEFAULT;

	Database *mysql = new Database(host, user, pass, db, port, numConns,gLua);
	if(mysql->num_connections < numConns) {
		delete mysql;
		mysql = NULL;
		return 0;
	}

	mysql->StartThreads(numThreads);

	ILuaObject* database = gLua->GetGlobal( "tmysql" );
		database->SetMemberUserDataLite( "p_MySQLdb", mysql );
	SyncList<DBQuery *>* completed = new SyncList<DBQuery *>();
		database->SetMemberUserDataLite( "p_CompletedQueries", completed );

	// hook.Add("Think", "TMysqlPoll", tmysql.poll)
	ILuaObject *hookt = gLua->GetGlobal("hook");
	ILuaObject *addf = hookt->GetMember("Add");
	addf->Push();
	gLua->Push("Think");
	gLua->Push("TMysqlPoll");
	gLua->Push(poll);
	gLua->Call(3);
	return 0;
}

// tmysql.query(sqlquery, [callback], [flags])
LUA_FUNCTION(query)
{
	ILuaInterface *gLua = Lua();
	Database *mysql = GetMySQL(gLua);
	if(!mysql || gLua->StringLength(1) == 0)
		return 0;

	char *query = strdup(gLua->GetString(1));

	int callbackfunc = -1;
	if(gLua->GetType(2) == GLua::TYPE_FUNCTION) 
	{
		callbackfunc = gLua->GetReference(2);
	}
	int flags = gLua->GetInteger(3);

	DBQuery *x = new DBQuery;
	x->callback = callbackfunc;
	x->flags = flags;
	x->query = query;
	x->lastid = 0;
	x->completed = GetCompletedQueries(gLua);
	mysql->queries.add(x);
	return 0;
}

// tmysql.escape(sqlquery)
LUA_FUNCTION(escape)
{
	ILuaInterface *gLua = Lua();
	Database *mysql = GetMySQL(gLua);
	if(!mysql)
		return 0;

	const char *query = gLua->GetString(1);
	bool real = gLua->GetBool(2);

	char *escaped = mysql->Escape(query, real);
	gLua->Push(escaped);
	delete escaped;
	return 1;
}

// tmysql.setcharset(charset)
LUA_FUNCTION(setcharset)
{
	ILuaInterface *gLua = Lua();
	Database *mysql = GetMySQL(gLua);
	if(!mysql)
		return 0;
	const char *set = gLua->GetString(1);
	gLua->Push((float)mysql->SetCharacterSet(set, gLua));
	return 1;
}

ILuaObject *TableResult(DBQuery *qres, ILuaInterface *gLua)
{
	MYSQL_FIELD *qfields;

	ILuaObject *restable = gLua->GetNewTable();

	if(qres->result == NULL)
		return restable;

	MYSQL_RES *result = qres->result;
	int field_count = mysql_num_fields(result);
	int xrow = 1;
	MYSQL_ROW row = mysql_fetch_row(result);

	if(qres->flags & QUERY_FLAG_ASSOC)
		qfields = mysql_fetch_fields(result);
	if(!qfields)
	{
		gLua->ErrorNoHalt("NULL fields\n");
		return restable;
	}

	while(row != NULL)
	{
		ILuaObject *tablerow = gLua->GetNewTable();
		for(int y=0; y < field_count; y++)
		{
			if(qres->flags & QUERY_FLAG_ASSOC)
			{
				tablerow->SetMember(qfields[y].name, row[y]);
			} else {
				tablerow->SetMember((float)y+1, row[y]);
			}
		}
		restable->SetMember((float)xrow, tablerow);
		tablerow->UnReference();
		row = mysql_fetch_row(result);
		xrow++;
	}

	return restable;	
}

/*
Posted by Armin Schöffmann on March 6 2005 10:27pm

According to dll.c:
Within win32, LibMain calls mysql_thread_init() and mysql_thread_end() automatically for each newly created thread after the lib has been loaded.
Therefore it shouldn't be necessary to call these functions again. As I said, only for win32 and only if we use the dynamic library.
*/
unsigned long WINAPI Database::tProc(void *param)
{
	char *error;
	Database *db = (Database *)param;
	SyncList<DBQuery *> *list = &db->queries;
	for(;;)
	{
		if(list->getSize() == 0)
		{
			if(db->shutdownflag == true) // we can only shutdown when we can't find any more queries
			{
				InterlockedIncrement(&db->dead_threads);
				break;
			}
			__asm pause;
			Sleep(1);
			continue;
		}
		DBQuery *dbq = list->remove();
		if(dbq == NULL) // possibly got removed between the check and this
			continue;
		const char *query = dbq->query;
		MysqlConnection *conn = db->FreeConnection();

		int result = mysql_real_query(conn->con, query, strlen(query));
		if(result > 0 && db->Alive(conn->con, &error))
		{
			dbq->state = QUERY_FAIL;
			dbq->result = NULL;
			dbq->error = error;
			conn->m_inuse->Release();
		} else if(result == 0) {
			dbq->state = QUERY_SUCCESS;
			MYSQL_RES *data = mysql_store_result(conn->con);
			if(dbq->flags & QUERY_FLAG_LASTID)
				dbq->lastid = mysql_insert_id(conn->con);
			conn->m_inuse->Release();
			dbq->result = data;
			dbq->error = NULL;
		} else {
			while(list->getSize() > 0) // eat up all the queries in a frenzy if we're panicking
			{
				DBQuery *qres = list->remove();
				if(qres == NULL)
					continue;
				free(qres->query);
				delete qres;
			}
			free(dbq->query);		
			delete dbq;
			continue;
		}
		dbq->completed->add(dbq);
	}
	return 0;
}

// tmysql.poll()
// do not call this
LUA_FUNCTION(poll)
{
	ILuaInterface *gLua = Lua();
	Database *mysql = GetMySQL(gLua);
	if(!mysql)
		return 0;

	SyncList<DBQuery *> *results = GetCompletedQueries(gLua);

	while(results->getSize() > 0)
	{
		DBQuery *qres = results->remove(); // you don't need a callback, but it can still report the status
		if(qres == NULL)
			continue;
		if(qres->callback >= 0)
		{
			ILuaObject *restable = TableResult(qres, gLua);

			gLua->PushReference(qres->callback);
			gLua->Push(restable);

			int param = 1;
			if(qres->state == QUERY_SUCCESS)
			{
				gLua->Push((float)qres->state);
				gLua->Push((float)qres->lastid);
				gLua->Call(3);
				
			} else {
				gLua->Push((float)qres->state);
				gLua->Push(qres->error);
				gLua->Call(3);
				free(qres->error);
			}
			gLua->FreeReference(qres->callback);
			restable->UnReference();
		}
		mysql_free_result(qres->result);
		free(qres->query);
		delete qres;
	}
	return 0;
}

int Start(lua_State *L)
{
	ILuaInterface *gLua = Lua();
	ILuaObject* mfunc = gLua->GetNewTable();
		mfunc->SetMember("initialize", minit);
		mfunc->SetMember("query", query);
		mfunc->SetMember("escape", escape);
		mfunc->SetMember("setcharset", setcharset);
	gLua->SetGlobal("tmysql", mfunc);
	// We're not deleting the table here - we're deleting the reference to it.
	// If you don't do this you're leaking memory.
	mfunc->UnReference();
	return 0;
}

int Close(lua_State *L)
{
	ILuaInterface *gLua = Lua();
	Database *mysql = GetMySQL(gLua);
	if(mysql)
	{
		mysql->Shutdown();
		// We're deleting this here.
		delete mysql;
	}
	SyncList<DBQuery *>* completed = GetCompletedQueries(gLua);
	if(completed)
	{
		delete completed;
	}
	return 0;
}