#include "gm_tmysql.h"

Database::Database(const char *host, const char *user, const char *pass, const char *db, int port, int numConn, ILuaInterface *gLua)
{
	db_connections = numConn;
	db_host = host;
	db_user = user;
	db_pass = pass;
	db_db = db;
	db_port = port;
	dead_threads = 0;
	num_connections = 0;
	Connections = new MysqlConnection[numConn];
	for(int i=0;i < numConn; i++)
	{
		if(!Database::Connect(&Connections[i], gLua)) {
			break;
		}
	}
}

char *Database::Escape(const char *query, bool real)
{
	size_t len = strlen(query);
	char *buffer = new char[len*2+1];
	if(real)
	{
		MysqlConnection *conn = FreeConnection();
		mysql_real_escape_string(conn->con, buffer, query, (unsigned long)len);
		conn->m_inuse->Release();
	} else {
		mysql_escape_string(buffer, query, len);
	}
	return buffer;
}

bool Database::SetCharacterSet(const char *set, ILuaInterface *gLua)
{
	bool ret = true;
	for(int i=0;i < db_connections; i++)
	{
		MysqlConnection *conn = &Connections[i];
		conn->m_inuse->Acquire();
		if(mysql_set_character_set(conn->con, set))
		{
			ret = false;
			gLua->Msg("Error setting character set: %s\n", mysql_error(conn->con));
		}
		conn->m_inuse->Release();
	}
	return ret;
}
bool Database::Connect(MysqlConnection *conn, ILuaInterface *gLua)
{
	++num_connections;
	conn->m_inuse = new Mutex;
	conn->con = mysql_init(NULL);
	my_bool bTrue = true;
	mysql_options(conn->con, MYSQL_OPT_RECONNECT, &bTrue);
	if(!mysql_real_connect(conn->con, db_host, db_user, db_pass, db_db, db_port, NULL, 0))
	{
		gLua->Msg("Error connecting: %s\n", mysql_error(conn->con));
		return false;
	}
	return true;
}

void Database::Disconnect(MysqlConnection *conn)
{
	mysql_close(conn->con);
	delete conn->m_inuse;
}

MysqlConnection *Database::FreeConnection()
{
	for(int i = 0;;i++)
	{
		MysqlConnection *con = &Connections[i%db_connections];
		if(con->m_inuse->AttemptAcquire())
			return con;
	}
	return NULL;
}

bool Database::Alive(MYSQL *m, char **error)
{
	const char *err = mysql_error(m);
	*error = strdup(err);
	if((err[0]) || (mysql_ping(m) > 0) )
	{
		for(int i = 0; i < 4; i++)
		{
			if(mysql_ping(m) == 0)
				return true;
			Sleep(500);
		}
		// no connection
		shutdownflag = true;
		return false;
	}
	return true;
}

void Database::StartThreads(int num_threads)
{
	Threads = new HANDLE[num_threads];
	thread_count = num_threads;
	for(int i=0;i<num_threads;i++)
	{
		Threads[i] = CreateThread(NULL, 0, &Database::tProc, this, 0, NULL); 
	}
}

void Database::Shutdown()
{
	shutdownflag = true;
	while(dead_threads < thread_count)
		Sleep(50);

	for(int i=0;i < db_connections; i++)
	{
		Database::Disconnect(&Connections[i]);
	}
}

Database::~Database(void)
{
	delete [] Threads;
	delete [] Connections;
}