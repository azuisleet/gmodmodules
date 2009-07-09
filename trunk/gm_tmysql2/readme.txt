Installation:
copy gm_tmysql.dll to garrysmod/garrysmod/lua/includes/modules
copy libmysql.dll to garrysmod/

libmysql is provided for your convenience, you can get it from the mysql website

Functions:

tmysql.initialize(host, user, pass, database, [port], [number of connections], [number of threads])
	Starts the library, number of connections is how many connections to make to the server, the number of threads is how many worker threads

tmysql.query(sqlquery, [callback], [flags])
	Sends a query to the server, callback is a function, flags are
		1	Assoc results	Your callback will be sent a table with field names for keys
		2	Last ID		Your callback will be sent the last id as the third parameter
	Callbacks are func(resulttable, querystatus, error or lastid)

tmysql.escape(sqlquery)
	Returns an escaped string

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