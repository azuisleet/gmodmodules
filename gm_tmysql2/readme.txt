New from SVN:
	- added debug mode
	- you can pass an arg to be passed to the callback function
		tmysql.query("SELECT 1+1", _R.Player.Kill, 0, Entity(1))
	- removed forcing numConns = numThreads + 1, instead made it error

Installation:
copy gm_tmysql.dll to garrysmod/garrysmod/lua/includes/modules
copy libmysql.dll to garrysmod/

libmysql is provided for your convenience, you can get it from the mysql website

Functions:

tmysql.initialize(host, user, pass, database, [port], [number of connections], [number of threads])
	Starts the library, number of connections is how many connections to make to the server, the number of threads is how many worker threads

tmysql.query(sqlquery, [callback], [flags], [callbackarg])
	Sends a query to the server, callback is a function, flags are
		1	Assoc results	Your callback will be sent a table with field names for keys
		2	Last ID		Your callback will be sent the last id as the third parameter
	Callbacks are func([callbackarg], resulttable, querystatus, error or lastid)

tmysql.escape(sqlquery)
	Returns an escaped string

tmysql.escape_real(sqlquery)
	Returns an escaped string with the real char set, requires numthreads+1 connections

tmysql.setcharset(charset)
	Sets the character set

Use:

require("tmysql")

tmysql.initialize(host, user, pass, db, port, numConns, numThreads) // call this once, you're connected.

function callbackfunc(result, status, error)

end

tmysql.query(strquery, callbackfunc)

x = tmysql.escape(z)

Also see the two test files included

Compiling:

You need the mysql source headers and link against libmysql.lib