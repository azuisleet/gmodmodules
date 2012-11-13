#include <deque>
#include <atomic>
#include <mutex>
#include <thread>
#include <asio.hpp>

#define QUERY_SUCCESS true
#define QUERY_FAIL false

#define QUERY_FLAG_ASSOC 1
#define QUERY_FLAG_LASTID 2

#define NUM_THREADS_DEFAULT 2
#define NUM_CON_DEFAULT NUM_THREADS_DEFAULT

#undef ENABLE_QUERY_TIMERS
#undef USE_THREAD_LOCAL

#ifdef ENABLE_QUERY_TIMERS
class Timer
{
public:
	Timer()
	{
		QueryPerformanceFrequency(&frequency);
		QueryPerformanceCounter(&startTick);
	}

	double GetElapsedSeconds( void )
	{
		LARGE_INTEGER currentTick;

		QueryPerformanceCounter(&currentTick);
		return (double)(currentTick.QuadPart - startTick.QuadPart) / frequency.QuadPart;
	}

private:
	LARGE_INTEGER frequency;
	LARGE_INTEGER startTick;
};
#endif

// From the boost atomic examples
template<typename T>
class waitfree_query_queue {
public:
  void push(T * data)
  {
    T * stale_head = head_.load(std::memory_order_relaxed);
    do {
      data->next = stale_head;
    } while (!head_.compare_exchange_weak(stale_head, data, std::memory_order_release, std::memory_order_relaxed));
  }
  
  T * pop_all(void)
  {
    T * last = pop_all_reverse(), * first = 0;
    while(last) {
      T * tmp = last;
      last = last->next;
      tmp->next = first;
      first = tmp;
    }
    return first;
  }
  
  waitfree_query_queue() : head_(0) {}

  T * pop_all_reverse(void)
  {
    return head_.exchange(0, std::memory_order_consume);
  }
private:
	std::atomic<T *> head_;
};

class Query
{
public:
	Query( const char* query, int callback = -1, int flags = 0, int callbackref = -1 ) : 
		m_strQuery( query ), m_iCallback( callback ), m_iFlags( flags ), m_iCallbackRef( callbackref ), m_iLastID( 0 )
	{
	}

	~Query( void )
	{
	}

	const std::string& GetQuery( void ) { return m_strQuery; }
	size_t		GetQueryLength( void ) { return m_strQuery.length(); }

	int			GetFlags( void ) { return m_iFlags; }
	int			GetCallback( void ) { return m_iCallback; }
	int			GetCallbackRef( void ) { return m_iCallbackRef; }

	void		SetError( const char* error ) { m_strError.assign( error ); }
	const std::string& GetError( void ) { return m_strError; }

	void		SetStatus( bool status ) { m_bStatus = status; }
	bool		GetStatus( void ) { return m_bStatus; }

	void		SetResult( MYSQL_RES* result ) { m_myResult = result; }
	MYSQL_RES*	GetResult( void ) { return m_myResult; }

	void		SetLastID( int ID ) { m_iLastID = ID; }
	int			GetLastID( void ) { return m_iLastID; }

#ifdef ENABLE_QUERY_TIMERS
	double		GetQueryTime( void ) { return m_queryTimer.GetElapsedSeconds(); }
#else
	double		GetQueryTime( void ) { return 0; }
#endif

private:

	std::string	m_strQuery;
	int			m_iCallback;
	int			m_iCallbackRef;
	
	int			m_iFlags;
	bool		m_bStatus;

	MYSQL_RES*	m_myResult;
	int			m_iLastID;

	std::string	m_strError;

#ifdef ENABLE_QUERY_TIMERS
	Timer		m_queryTimer;
#endif

public:
	Query*		next;
};

class Database
{
public:
	Database( const char* host, const char* user, const char* pass, const char* db, int port );
	~Database( void );

	bool	Initialize( std::string& error );
	void	Shutdown( void );
	std::size_t RunShutdownWork( void );
	void	Release( void );

	bool		SetCharacterSet( const char* charset, std::string& error );
	char*		Escape( const char* query );
	void		QueueQuery( const char* query, int callback = -1, int flags = 0, int callbackref = -1 );

	Query*		GetCompletedQueries();

private:
	bool Connect( MYSQL* mysql, std::string& error );

	void QueueQuery( Query* query );

	void DoExecute( Query* query );
	void PushCompleted( Query* query );

	MYSQL* GetAvailableConnection() 
	{
		std::lock_guard<std::recursive_mutex> guard(m_AvailableMutex);

		MYSQL* result = m_vecAvailableConnections.front();
		m_vecAvailableConnections.pop_front();

		return result;
	}

	void ReturnConnection(MYSQL* mysql)
	{
		std::lock_guard<std::recursive_mutex> guard(m_AvailableMutex);

		m_vecAvailableConnections.push_back( mysql );
	}

	waitfree_query_queue<Query> m_completedQueries;
	std::deque< MYSQL* > m_vecAvailableConnections;

	mutable std::recursive_mutex m_AvailableMutex;

	std::vector<std::thread> thread_group;
	asio::io_service io_service;
	std::auto_ptr<asio::io_service::work> work;

	std::string		m_strHost;
	std::string		m_strUser;
	std::string		m_strPass;
	std::string		m_strDB;
	int				m_iPort;
};