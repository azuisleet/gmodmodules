#ifdef WINDOWS
	#define WIN32_LEAN_AND_MEAN
	#include <WinSock2.h>
#else
	#include <string.h>
	#include <sys/select.h>
#endif

#include <map>
#include <queue>
#include <vector>
#include <libpq-fe.h>

#include <boost/thread.hpp>

#include "common/GMLuaModule.h"

class ManagedPGconn;

const int TYPE_PGCONN = 50;
const int TYPE_PGRESULT = TYPE_PGCONN + 1;

boost::recursive_mutex connections_mutex;
std::map< lua_State*, std::vector< ManagedPGconn* > > connections;

boost::thread* update_thread;

class ManagedPGconn {	
public:
	ManagedPGconn(PGconn* conn)
		: m_conn(conn), m_selectState(PGRES_POLLING_FAILED),
		  m_queryActive(false), m_query_complete(false) {};
	~ManagedPGconn();

	void update();

	int lua_reset(lua_State* L);
	
	int lua_status(lua_State* L);
	int lua_transaction_status(lua_State* L);
	int lua_error_message(lua_State* L);

	int lua_query(lua_State* L);
	int lua_query_params(lua_State* L);
	int lua_query_complete(lua_State* L);
	int lua_cancel_query(lua_State* L);

	int lua_get_results(lua_State* L);
	int lua_get_notifications(lua_State* L);
	
	int lua_escape_string(lua_State* L);

private:
	PGconn* m_conn;
	PostgresPollingStatusType m_selectState;

	bool m_queryActive;
	bool m_query_complete;
	
	std::queue< PGresult* > m_results;
	std::queue< std::string > m_notifications;

	boost::recursive_mutex m_conn_mutex;
	boost::recursive_mutex m_data_mutex;

	bool select(PostgresPollingStatusType mode);	
	
	// Hopefully will save somebody from doing something
	// stupid at some point along the line. Probably me.
	ManagedPGconn(ManagedPGconn&);
	void operator=(ManagedPGconn&);
};

ManagedPGconn::~ManagedPGconn()
{
	boost::recursive_mutex::scoped_lock conn_lock(m_conn_mutex);
	PQfinish(m_conn);	

	// Free up any result objects that never
	// made it into the lua state. Those that
	// did will be freed by the garbage collector.
	boost::recursive_mutex::scoped_lock data_lock(m_data_mutex);
	while ( !m_results.empty() )
	{
		PGresult* res = m_results.front();
		m_results.pop();

		PQclear(res);
	}	
}

bool ManagedPGconn::select(PostgresPollingStatusType mode)
{
	int socket = PQsocket(m_conn);

	fd_set set;
	timeval time = { 0, 0 };

	FD_ZERO(&set);
	FD_SET(socket, &set);

	if ( mode == PGRES_POLLING_READING )
		return ::select(socket + 1, &set, NULL, NULL, &time) == 1;
	else if ( mode == PGRES_POLLING_WRITING )
		return ::select(socket + 1, NULL, &set, NULL, &time) == 1;

	return false;
}

void ManagedPGconn::update()
{
	boost::recursive_mutex::scoped_lock conn_lock(m_conn_mutex);

	ConnStatusType status = PQstatus(m_conn);

	if ( status == CONNECTION_OK )
	{
		if ( select(PGRES_POLLING_READING) )
		{
			boost::recursive_mutex::scoped_lock data_lock(m_data_mutex);

			PQconsumeInput(m_conn);

			while ( PGnotify* notification = PQnotifies(m_conn) )
				m_notifications.push(std::string(notification->relname));

			PGresult* res;
			while ( m_queryActive && !PQisBusy(m_conn) && (res = PQgetResult(m_conn)) )
			{
				m_results.push(res);

				m_queryActive = PQisBusy(m_conn) == 1;

				if ( !m_queryActive )
					m_query_complete = true;
			}
		}
	}
	else if ( status != CONNECTION_BAD ) // connecting
	{
		// This avoids stalling the connection
		if ( m_selectState != PGRES_POLLING_FAILED && !select(m_selectState) )
			return;

		PostgresPollingStatusType connectStatus = PQconnectPoll(m_conn);

		if ( connectStatus == PGRES_POLLING_READING || connectStatus == PGRES_POLLING_WRITING )
		{
			m_selectState = connectStatus;
		}
		else if ( connectStatus == PGRES_POLLING_OK )
		{
			m_selectState = PGRES_POLLING_FAILED;
		}
	}
}

int ManagedPGconn::lua_reset(lua_State* L)
{
	boost::recursive_mutex::scoped_lock conn_lock(m_conn_mutex);
	boost::recursive_mutex::scoped_lock data_lock(m_data_mutex);

#ifdef DEBUG
	Lua()->Msg("[D] libpgsql: ManagedPGconn* 0x%x lua_reset\n", this);
#endif

	if ( !PQresetStart(m_conn) )
		Lua()->Error("libpgsql: failed to reset connection");

	m_queryActive = false;
	m_query_complete = false;

	while ( !m_results.empty() )
	{
		PGresult* res = m_results.front();
		m_results.pop();

		PQclear(res);
	}

	return 0;
}

int ManagedPGconn::lua_status(lua_State* L)
{
	boost::recursive_mutex::scoped_lock conn_lock(m_conn_mutex);
	ConnStatusType status = PQstatus(m_conn);

#ifdef DEBUG
	if ( status != CONNECTION_OK )
		Lua()->Msg("[D] libpgsql: ManagedPGconn* 0x%x lua_status (%i)\n", this, status);
#endif

	Lua()->Push( (float) status );

	return 1;
}

int ManagedPGconn::lua_transaction_status(lua_State* L)
{
	boost::recursive_mutex::scoped_lock conn_lock(m_conn_mutex);
	PGTransactionStatusType status = PQtransactionStatus(m_conn);

#ifdef DEBUG
	Lua()->Msg("[D] libpgsql: ManagedPGconn* 0x%x lua_transaction_status (%i)\n", this, status);
#endif

	Lua()->Push( (float) status );

	return 1;
}

int ManagedPGconn::lua_error_message(lua_State* L)
{
	boost::recursive_mutex::scoped_lock conn_lock(m_conn_mutex);
	char* error = PQerrorMessage(m_conn);

#ifdef DEBUG
	if ( strlen(error) > 0 )
		Lua()->Msg("[D] libpgsql: ManagedPGconn* 0x%x lua_error_message\n", this);
#endif

	if ( strlen(error) > 0 )
	{
		Lua()->Push( error );
		return 1;
	}

	return 0;
}

int ManagedPGconn::lua_query(lua_State* L)
{
	boost::recursive_mutex::scoped_lock conn_lock(m_conn_mutex);

	Lua()->CheckType(2, GLua::TYPE_STRING);
	const char* query = Lua()->GetString(2);

	if ( m_queryActive || m_query_complete )
		Lua()->Error("libpgsql: can't query until active query completes");
		
	int res = PQsendQuery(m_conn, query);

#ifdef DEBUG
	Lua()->Msg("[D] libpgsql: ManagedPGconn* 0x%x lua_query %s (%i)\n", this, query, res);
#endif

	if ( !res )
		Lua()->Error("libpgsql: error sending query");

	m_queryActive = true;

	return 0;
}

int ManagedPGconn::lua_query_params(lua_State* L)
{
	boost::recursive_mutex::scoped_lock conn_lock(m_conn_mutex);

	Lua()->CheckType(2, GLua::TYPE_STRING);
	const char* query = Lua()->GetString(2);

	if ( m_queryActive || m_query_complete )
		Lua()->Error("libpgsql: can't query until active query completes");

	int items = Lua()->GetStackTop() - 2;
	int res;

	if ( items == 0 )
	{
		res = PQsendQueryParams(m_conn, query, 0, NULL, NULL, NULL, NULL, 0);
	}
	else
	{
		for ( int i = 0; i < items; i++ )
			Lua()->CheckType(3 + i, GLua::TYPE_STRING);

		const char** params = new const char*[items];

		for ( int i = 0; i < items; i++ )
			params[i] = Lua()->GetString(3 + i);

		res = PQsendQueryParams(m_conn, query, items, NULL, params, NULL, NULL, 0);

		delete[] params;
	}

#ifdef DEBUG
	Lua()->Msg("[D] libpgsql: ManagedPGconn* 0x%x lua_query_params %s (%i params) (%i)\n", this, query, items, res);
#endif

	if ( !res )
		Lua()->Error("libpgsql: error sending query");

	m_queryActive = true;

	return 0;
}

int ManagedPGconn::lua_query_complete(lua_State* L)
{
	boost::recursive_mutex::scoped_lock data_lock(m_data_mutex);

	Lua()->Push( m_query_complete == true );

#ifdef DEBUG
	if ( m_query_complete )
		Lua()->Msg("[D] libpgsql: ManagedPGconn* 0x%x lua_query_complete\n", this);
#endif

	return 1;
}

int ManagedPGconn::lua_cancel_query(lua_State* L)
{
	boost::recursive_mutex::scoped_lock conn_lock(m_conn_mutex);

#ifdef DEBUG
	Lua()->Msg("[D] libpgsql: ManagedPGconn* 0x%x lua_cancel_query\n", this);
#endif

	// no query is active or has pending results,
	// so error (no reason to call)
	if ( !(m_queryActive || m_query_complete) )
		Lua()->Error("libpgsql: attempt to cancel non-existent query");

	// if the query has completed and just not
	// been asked for the results by Lua, do
	// nothing; there is no internal query to
	// cancel.
	if ( !m_query_complete )
	{
		char err[256];

		PGcancel* cancel = PQgetCancel(m_conn);
		int res = PQcancel(cancel, err, sizeof(err));
		PQfreeCancel(cancel);

		// documentation doesn't say exactly why
		// this might ever fail...
		if ( !res )
			Lua()->ErrorNoHalt("libpgsql: error sending cancel: %s\n", err);
	}

	return 0;
}

int ManagedPGconn::lua_get_results(lua_State* L)
{
	boost::recursive_mutex::scoped_lock data_lock(m_data_mutex);

	if ( !m_query_complete )
		return 0;

	ILuaObject* meta = Lua()->GetMetaTable("PGresult", TYPE_PGRESULT);

	int results = 0;
	while ( !m_results.empty() )
	{
		Lua()->PushUserData(meta, (void *) m_results.front());
		m_results.pop();
		results++;
	}

	
#ifdef DEBUG
	Lua()->Msg("[D] libpgsql: ManagedPGconn* 0x%x lua_get_results (%i results)\n", this, results);
#endif

	m_query_complete = false;
	return results;
}

int ManagedPGconn::lua_get_notifications(lua_State* L)
{
	boost::recursive_mutex::scoped_lock data_lock(m_data_mutex);

	int notifications = 0;
	while ( !m_notifications.empty() )
	{
		Lua()->Push(m_notifications.front().c_str());
		m_notifications.pop();
		notifications++;
	}

#ifdef DEBUG
	if ( notifications > 0 )
		Lua()->Msg("[D] libpgsql: ManagedPGconn* 0x%x lua_get_notifications (%i notifications)\n", this, notifications);
#endif

	return notifications;
}

int ManagedPGconn::lua_escape_string(lua_State* L)
{
	boost::recursive_mutex::scoped_lock conn_lock(m_conn_mutex);

	Lua()->CheckType(2, GLua::TYPE_STRING);
	const char* str = Lua()->GetString(2);

	size_t len = strlen(str)*2 + 1;	
	char* escaped = new char[len];

	int size = PQescapeStringConn(m_conn, escaped, str, len, NULL);

#ifdef DEBUG
	Lua()->Msg("[D] libpgsql: ManagedPGconn* 0x%x lua_escape_string (%i -> %i)\n", this, strlen(str), size);
#endif

	Lua()->Push( escaped );
	delete[] escaped;

	return 1;
}

LUA_FUNCTION(pgconn___gc)
{
	Lua()->CheckType(1, TYPE_PGCONN);
	ManagedPGconn* conn = (ManagedPGconn *) Lua()->GetUserData(1);

#ifdef DEBUG
	Lua()->Msg("[D] libpgsql: ManagedPGconn* 0x%x __gc\n", conn);
#endif

	{
		boost::recursive_mutex::scoped_lock lock(connections_mutex);

		std::vector< ManagedPGconn* >& remove_from = connections[L];
		std::vector< ManagedPGconn* >::iterator i;

		for ( i = remove_from.begin(); i != remove_from.end(); i++ )
		{
			if ( *i == conn )
			{
				remove_from.erase(i);
				break;
			}
		}
	}

	delete conn;

	return 0;
}

LUA_FUNCTION(pgconn_reset)
{
	Lua()->CheckType(1, TYPE_PGCONN);
	ManagedPGconn* conn = (ManagedPGconn *) Lua()->GetUserData(1);

	return conn->lua_reset(L);
}

LUA_FUNCTION(pgconn_status)
{
	Lua()->CheckType(1, TYPE_PGCONN);
	ManagedPGconn* conn = (ManagedPGconn *) Lua()->GetUserData(1);

	return conn->lua_status(L);
}

LUA_FUNCTION(pgconn_transaction_status)
{
	Lua()->CheckType(1, TYPE_PGCONN);
	ManagedPGconn* conn = (ManagedPGconn *) Lua()->GetUserData(1);

	return conn->lua_transaction_status(L);
}

LUA_FUNCTION(pgconn_error)
{
	Lua()->CheckType(1, TYPE_PGCONN);
	ManagedPGconn* conn = (ManagedPGconn *) Lua()->GetUserData(1);

	return conn->lua_error_message(L);
}

LUA_FUNCTION(pgconn_query)
{
	Lua()->CheckType(1, TYPE_PGCONN);
	ManagedPGconn* conn = (ManagedPGconn *) Lua()->GetUserData(1);

	return conn->lua_query(L);
}

LUA_FUNCTION(pgconn_query_params)
{
	Lua()->CheckType(1, TYPE_PGCONN);
	ManagedPGconn* conn = (ManagedPGconn *) Lua()->GetUserData(1);

	return conn->lua_query_params(L);
}

LUA_FUNCTION(pgconn_query_complete)
{
	Lua()->CheckType(1, TYPE_PGCONN);
	ManagedPGconn* conn = (ManagedPGconn *) Lua()->GetUserData(1);

	return conn->lua_query_complete(L);
}

LUA_FUNCTION(pgconn_cancel_query)
{
	Lua()->CheckType(1, TYPE_PGCONN);
	ManagedPGconn* conn = (ManagedPGconn *) Lua()->GetUserData(1);

	return conn->lua_cancel_query(L);
}

LUA_FUNCTION(pgconn_get_results)
{
	Lua()->CheckType(1, TYPE_PGCONN);
	ManagedPGconn* conn = (ManagedPGconn *) Lua()->GetUserData(1);

	return conn->lua_get_results(L);
}

LUA_FUNCTION(pgconn_get_notifications)
{
	Lua()->CheckType(1, TYPE_PGCONN);
	ManagedPGconn* conn = (ManagedPGconn *) Lua()->GetUserData(1);

	return conn->lua_get_notifications(L);
}

LUA_FUNCTION(pgconn_escape)
{
	Lua()->CheckType(1, TYPE_PGCONN);
	ManagedPGconn* conn = (ManagedPGconn *) Lua()->GetUserData(1);
	
	return conn->lua_escape_string(L);
}

LUA_FUNCTION(pgres___gc)
{
	Lua()->CheckType(1, TYPE_PGRESULT);

	PGresult* res = (PGresult *) Lua()->GetUserData(1);

#ifdef DEBUG
	Lua()->Msg("[D] libpgsql: PGresult* 0x%x clear\n", res);
#endif

	PQclear(res);

	return 0;
}

LUA_FUNCTION(pgres_status)
{
	Lua()->CheckType(1, TYPE_PGRESULT);

	PGresult* res = (PGresult *) Lua()->GetUserData(1);

	ExecStatusType status = PQresultStatus(res);

#ifdef DEBUG
	Lua()->Msg("[D] libpgsql: PGresult* 0x%x status (%i)\n", res, status);
#endif

	Lua()->Push( (float) status );

	return 1;
}

LUA_FUNCTION(pgres_error)
{
	Lua()->CheckType(1, TYPE_PGRESULT);

	PGresult* res = (PGresult *) Lua()->GetUserData(1);

	char* err = PQresultErrorMessage(res);

#ifdef DEBUG
	if ( strlen(err) > 0 )
		Lua()->Msg("[D] libpgsql: PGresult* 0x%x error", res);
#endif

	if ( strlen(err) > 0 )
	{
		Lua()->Push(err);
		return 1;
	}

	return 0;
}

LUA_FUNCTION(pgres_ntuples)
{
	Lua()->CheckType(1, TYPE_PGRESULT);

	PGresult* res = (PGresult *) Lua()->GetUserData(1);

	int count = PQntuples(res);

#ifdef DEBUG
	Lua()->Msg("[D] libpgsql: PGresult* 0x%x ntuples (%i)\n", res, count);
#endif

	Lua()->Push( (float) count );
	return 1;
}

LUA_FUNCTION(pgres_nfields)
{
	Lua()->CheckType(1, TYPE_PGRESULT);

	PGresult* res = (PGresult *) Lua()->GetUserData(1);

	int count = PQnfields(res);

#ifdef DEBUG
	Lua()->Msg("[D] libpgsql: PGresult* 0x%x nfields (%i)\n", res, count);
#endif

	Lua()->Push( (float) count );
	return 1;
}

LUA_FUNCTION(pgres_fname)
{
	Lua()->CheckType(1, TYPE_PGRESULT);
	Lua()->CheckType(2, GLua::TYPE_NUMBER);

	PGresult* res = (PGresult *) Lua()->GetUserData(1);
	int field = Lua()->GetInteger(2);

	char* name = PQfname(res, field);

#ifdef DEBUG
	Lua()->Msg("[D] libpgsql: PGresult* 0x%x fname (%s)\n", res, name ? name : "NULL");
#endif

	if ( name )
	{
		Lua()->Push( name );
		return 1;
	}

	return 0;
}

LUA_FUNCTION(pgres_fnumber)
{
	Lua()->CheckType(1, TYPE_PGRESULT);
	Lua()->CheckType(2, GLua::TYPE_STRING);

	PGresult* res = (PGresult *) Lua()->GetUserData(1);
	const char* name = Lua()->GetString(2);

	int num = PQfnumber(res, name);

#ifdef DEBUG
	Lua()->Msg("[D] libpgsql: PGresult* 0x%x fnumber %s (%i)\n", res, name, num);
#endif

	if ( num >= 0 )
	{
		Lua()->Push( (float) num );
		return 1;
	}

	return 0;
}

LUA_FUNCTION(pgres_get_value)
{
	Lua()->CheckType(1, TYPE_PGRESULT);
	Lua()->CheckType(2, GLua::TYPE_NUMBER);
	Lua()->CheckType(3, GLua::TYPE_NUMBER);

	PGresult* res = (PGresult *) Lua()->GetUserData(1);
	int row = Lua()->GetInteger(2);
	int column = Lua()->GetInteger(3);

	if ( PQgetisnull(res, row, column) )
	{
#ifdef DEBUG
		Lua()->Msg("[D] libpgsql: PGresult* 0x%x get_value %i, %i (NULL)\n", res, row, column);
#endif
		return 0;
	}

	char* val = PQgetvalue(res, row, column);

#ifdef DEBUG
	Lua()->Msg("[D] libpgsql: PGresult* 0x%x get_value %i, %i (%s)\n", res, row, column, val);
#endif

	Lua()->Push(val);
	return 1;
}

LUA_FUNCTION(create)
{
	Lua()->CheckType(1, GLua::TYPE_STRING);

	const char* conninfo = Lua()->GetString(1);

	PGconn* conn = PQconnectStart(conninfo);

	if ( conn == NULL )
		Lua()->Error("libpgsql: Unable to allocate PGConn structure");

	if ( PQstatus(conn) == CONNECTION_BAD )
	{
		PQfinish(conn);
		Lua()->Error("libpgsql: CONNECTION_BAD on new connection");
	}

	ManagedPGconn* managed = new ManagedPGconn(conn);

	{
		boost::recursive_mutex::scoped_lock lock(connections_mutex);
		connections[L].push_back(managed);
	}

#ifdef DEBUG
	Lua()->Msg("[D] libpgsql: creating ManagedPGconn* 0x%x\n", managed);
#endif

	ILuaObject* meta = Lua()->GetMetaTable("PGconn", TYPE_PGCONN);
	Lua()->PushUserData(meta, (void *) managed);

	return 1;
}

void connection_updater_thread()
{
	while ( true )
	{
		{
			boost::recursive_mutex::scoped_lock lock(connections_mutex);

			std::map < lua_State*, std::vector< ManagedPGconn* > >::iterator state;

			for ( state = connections.begin(); state != connections.end(); state++ )
			{
				std::vector< ManagedPGconn* >::iterator i;

				for ( i = state->second.begin(); i != state->second.end(); i++ )
					(*i)->update();
			}
		}

		// this doubles as the interrupt point
		boost::this_thread::sleep(boost::posix_time::milliseconds(50));
	}
}

GMOD_MODULE(start, close);

int start(lua_State* L)
{
	ILuaObject* table = Lua()->GetNewTable();

	ILuaObject* metaConn = Lua()->GetMetaTable("PGconn", TYPE_PGCONN);		
		metaConn->SetMember("__index", metaConn);
		metaConn->SetMember("__gc", pgconn___gc);
		
		metaConn->SetMember("reset", pgconn_reset);

		metaConn->SetMember("status", pgconn_status);
		metaConn->SetMember("transaction_status", pgconn_transaction_status);
		metaConn->SetMember("error", pgconn_error);

		metaConn->SetMember("query", pgconn_query);
		metaConn->SetMember("query_params", pgconn_query_params);
		metaConn->SetMember("query_complete", pgconn_query_complete);
		metaConn->SetMember("cancel_query", pgconn_cancel_query);

		metaConn->SetMember("get_results", pgconn_get_results);
		metaConn->SetMember("get_notifications", pgconn_get_notifications);

		metaConn->SetMember("escape", pgconn_escape);

	ILuaObject* metaRes = Lua()->GetMetaTable("PGresult", TYPE_PGRESULT);
		metaRes->SetMember("__index", metaRes);
		metaRes->SetMember("__gc", pgres___gc);

		metaRes->SetMember("status", pgres_status);
		metaRes->SetMember("error", pgres_error);

		metaRes->SetMember("num_rows", pgres_ntuples);
		metaRes->SetMember("num_columns", pgres_nfields);

		metaRes->SetMember("column_name", pgres_fname);
		metaRes->SetMember("column_num", pgres_fnumber);

		metaRes->SetMember("get_value", pgres_get_value);		

	ILuaObject* pgsql = Lua()->GetNewTable();
		pgsql->SetMember("create", create);

	Lua()->SetGlobal("libpgsql", pgsql);
	pgsql->UnReference();

	if ( connections.empty() )
		update_thread = new boost::thread(connection_updater_thread);
	
	{
		boost::recursive_mutex::scoped_lock lock(connections_mutex);
		connections[L] = std::vector< ManagedPGconn* >();
	}

	return 0;
}

int close(lua_State* L)
{
	{
		boost::recursive_mutex::scoped_lock lock(connections_mutex);
		connections.erase(L);
	}

	if ( connections.empty() )
	{
		update_thread->interrupt();
		update_thread->join();

		delete update_thread;
	}

	return 0;
}