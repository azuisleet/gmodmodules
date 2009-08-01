#define QUERY_SUCCESS 1
#define QUERY_FAIL 0

#define QUERY_FLAG_ASSOC 1
#define QUERY_FLAG_LASTID 2

struct DBQuery {
	char *query;
	int callback;
	int callbackref;
	SyncList<DBQuery *>* completed;
	MYSQL_RES *result;
	int state;
	int flags;
	char *error;
	my_ulonglong lastid;
};


struct MysqlConnection {
	MYSQL *con;
	Mutex *m_inuse;
};
class Database
{
	volatile long dead_threads;
	volatile bool shutdownflag;
public:
	int thread_count;
	int db_connections;
private:
	const char *db_host;
	const char *db_user;
	const char *db_pass;
	const char *db_db;
	int db_port;
	MysqlConnection *Connections;
	HANDLE *Threads;

	MysqlConnection *FreeConnection();
	static unsigned long WINAPI tProc(void *param);
	bool Database::Connect(MysqlConnection *conn, ILuaInterface *gLua);
	void Database::Disconnect(MysqlConnection *conn);
	bool Database::Alive(MYSQL *m, char **error);
public:
	int num_connections;
	Database::Database(const char *host, const char *user, const char *pass, const char *db, int port, int numConn, ILuaInterface *gLua);
	~Database();
	void Database::StartThreads(int num_threads);
	void Database::Shutdown();

	char *Database::Escape(const char *query, bool real);
	bool Database::SetCharacterSet(const char *set, ILuaInterface *gLua);

	SyncList<DBQuery *> queries;

	bool debugmode;
};