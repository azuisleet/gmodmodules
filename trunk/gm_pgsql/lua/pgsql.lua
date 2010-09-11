require("libpgsql")
require("hook")

local libpgsql = libpgsql
local hook = hook
local table = table

local error = error
local ErrorNoHalt = ErrorNoHalt
local pairs = pairs
local ipairs = ipairs
local print = print
local select = select
local tostring = tostring
local setmetatable = setmetatable
local pcall = pcall
local unpack = unpack
local SysTime = SysTime
local type = type

module("pgsql")

CONNECTION = {
	OK = 0,
	BAD = 1,
	STARTED = 2,
	MADE = 3,
	AWAITING_RESPONSE = 4,
	AUTH_OK = 5,
	SETENV = 6,
	SSL_STARTUP = 7,
}

POLLING = {
	FAILED = 0,
	READING = 1,
	WRITING = 2,
	OK = 3,
}

COMMAND = {
	EMPTY_QUERY = 0,
	COMMAND_OK = 1,
	TUPLES_OK = 2,
	COPY_OUT = 3,
	COPY_IN = 4,
	BAD_RESPONSE = 5,
	FATAL_ERROR = 7,
}

local _connections = {}

local pg_result = {}
function pg_result:__index(k)
	if pg_result[k] then return pg_result[k] end
	local func = self._res[k]
	
	if not func then return end
	
	// wrap to the internal result object
	return function(...)
		return func(self._res, select(2, ...))
	end
end

// needless to say, nesting this would be a bad idea.
// if you feel the need to nest loops of the 
// same result set... (why would you do this)
// use rows() instead!
function pg_result:__call()
	if not self.i then self.i = 1 end
	
	local ret = self:row(self.i)
			
	if not ret then
		self.i = nil // reset so that another loop can be run
	else
		self.i = self.i + 1
	end
	
	return ret
end

function pg_result:rows()
	local i = 1
	
	return function()
		local ret = self:row(i)
		i = i + 1
		
		return ret
	end
end

function pg_result:row(row)
	if row > self._num_rows or row <= 0 then return end
	
	local ret = {}
	
	for i = 0, self._num_columns - 1 do
		ret[i+1] = self._res:get_value(row-1, i)
	end
	
	return setmetatable(ret, {
		__index = function(t, k)
			if type(k) == "string" then
				local col = self._column_names[k]
				if not col then return end
				
				return t[col]
			end
		end,
	})
end

function pg_result:success()
	if self.status == COMMAND.TUPLES_OK or self.status == COMMAND.COMMAND_OK then
		return true
	end
	
	return false
end

local function _wrap_result(result)
	local obj = {
		_res = result,
		_column_names = {},
		
		_num_rows = result:num_rows(),
		_num_columns = result:num_columns(),
		
		status = result:status(),
		error = result:error(),
	}
	
	for i = 0, obj._num_columns - 1 do
		local name = result:column_name(i)
		
		obj._column_names[name] = i + 1
		obj._column_names[i + 1] = name 
	end
	
	return setmetatable(obj, pg_result)
end

local pg_conn_skel = {
	_id = 0,
	_conn = nil,
	
	_connectStart = nil, // used for connection timeouts	
	
	_connectionFailedCount = 0,
	_isInitialConnection = true,
	
	_queryActive = false,	
	
	_listening = {}, // { event = "", callback = function() end, }
	_queue = {}, // { query = "", callback = function() end, args = {} }
}

local pg_conn = {}
function pg_conn:__index(k)
	if pg_conn[k] then return pg_conn[k] end
	if k == "_conn" then return end
	
	local func = self._conn[k]	
	if not func then return end
	
	// wrap to the internal connection object
	return function(...)
		return func(self._conn, select(2, ...))
	end
end

function pg_conn:query(query, ...)
	if not self._conn then error("pgsql: attempt to query closed connection") end
	if not query then error("pgsql: query string required") end
	
	local args = {...}
	local callback
	
	// allows for calls to parameterized queries
	// to support callback being passed normally
	if type(args[#args]) == "function" then
		callback = table.remove(args, #args)
	end
	
	// make sure the queries aren't passing anything
	// particularly dumb into the query, and then
	// converting it all to a string so it can be
	// easily handled inside libpgsql
	for i,v in ipairs(args) do
		if type(v) ~= "string" then
			if type(v) == "boolean" or type(v) == "number" then
				args[i] = tostring(v)
			else
				error("pgsql: attempt to use unsupported type '" .. type(v) .. "' as query parameter (use strings!)")
			end
		end
	end

	table.insert(self._queue, { query = query, callback = callback, args = args })
	
	// handles the actual call into libpgsql
	// and will immediately call into it if
	// the connection isn't busy
	self:_run_next_query()
end

function pg_conn:listen(event, callback)
	if not self._conn then error("pgsql: attempt to listen with closed connection") end
	if self._isInitialConnection then error("pgsql: attempt to query before connected") end
	
	if self._listening[event] then
		ErrorNoHalt("pgsql: Overriding listener for " .. event .. "!\n")
	end
	
	self._listening[event] = callback
	self:query("LISTEN \"" .. event .. "\";")
end

function pg_conn:unlisten(event, ignoretest)
	if not self._conn then error("pgsql: attempt to unlisten with closed connection") end
	if not self._listening[event] and not ignoretest then
		ErrorNoHalt("pgsql: Removing non-existent listener for '" .. event .. "'!\n")
	end
	
	self._listening[event] = nil
	self:query("UNLISTEN \"" .. event .. "\";")
end

function pg_conn:_update()
	local status = self._conn:status()
	
	if self._lastStatus ~= status then
		// connect (re)established
		if status == CONNECTION.OK then
			self._connectionFailedCount = 0
			
			// listen to anything we've been asked to listen
			// to, since on reconnection it's all lost
			for k, _ in pairs(self._listening) do
				self:query("LISTEN \"" .. k .. "\";");
			end			
			
			if self._isInitialConnection then
				self._isInitialConnection = false
				
				if self.onInitialConnect then
					local ret = { pcall(self.onInitialConnect, self) }
				
					if not ret[1] then
						ErrorNoHalt("pgsql: onInitialConnect callback failed: " .. tostring(ret[2]) .. "\n")
					end	
				end
			end
			
			if self.onConnect then
				local ret = { pcall(self.onConnect, self) }
				
				if not ret[1] then
					ErrorNoHalt("pgsql: onConnection callback failed: " .. tostring(ret[2]) .. "\n")
				end
			end
		end
				
		self._lastStatus = status
	end
	
	if status == CONNECTION.OK then
		for _,v in ipairs({ self._conn:get_notifications() }) do
			local func = self._listening[v]
			
			if func then
				local ret = { pcall(func) }
				
				if not ret[1] then
					ErrorNoHalt("pgsql: Event '" .. v .. "' failed: " .. tostring(ret[2]) .. "\n")
				end
			else
				ErrorNoHalt("pgsql: No listener for '" .. v .. "'; removing\n")
				self:unlisten(v, true)
			end
		end
		
		if self._queryActive and self._conn:query_complete() then
			self._queryActive = false
			
			local results = { self._conn:get_results() }
			local top = table.remove(self._queue, 1)			
			
			for i,v in ipairs(results) do
				local res = _wrap_result(v)
				results[i] = res
				
				if not res:success() and self.onError then
					local ret = { pcall(self.onError, self, top.query, res:error()) }
			
					if not ret[1] then
						ErrorNoHalt("pgsql: Query error callback failed: " .. tostring(ret[2]) .. "\n")
					end
				end
			end
				
			if top.callback then
				local ret = { pcall(top.callback, unpack(results)) }
				
				if not ret[1] then
					ErrorNoHalt("pgsql: Query result callback failed: " .. tostring(ret[2]) .. "\n")
				end
			end
		elseif self._queryActive then			
			if SysTime() - self._queryStart > 30 then
				ErrorNoHalt("pgsql: Terminating slow query! (" .. self._queue[1].query .. ")\n")
				
				self:cancelQuery()
				return
			elseif not self._queryWarn and SysTime() - self._queryStart > 10 then
				ErrorNoHalt("pgsql: Slow query detected! (" .. self._queue[1].query .. ")\n")
				
				self._queryWarn = true
			end
		end
		
		// this checks if a query is already running,
		// so no worries
		self:_run_next_query()
	elseif status == CONNECTION.BAD then
		if not self._nextConnectAttempt then
			ErrorNoHalt("pgsql: connection #" .. tostring(self._id) .. " is bad; " .. self._conn:error():gsub("\n", "\n   "):sub(1, -4))
			
			self._connectionFailedCount = self._connectionFailedCount + 1
			
			if self._connectionFailedCount >= 100 then
				ErrorNoHalt("pgsql: terminating connection\n")
				return false
			end
			
			ErrorNoHalt("pgsql: resetting connection (attempt "..tostring(self._connectionFailedCount)..")\n")			
			
			// attempt to reconnect in 5 seconds
			self._nextConnectAttempt = SysTime() + 5
		elseif SysTime() > self._nextConnectAttempt then
			self._nextConnectAttempt = nil
			self:reset()			
		end
	else
		if not self._isInitialConnection and SysTime() - self._connectStart > 10 then
			ErrorNoHalt("pgsql: connection #" .. tostring(self._id) .. " timed out during reconnect; resetting\n")
			self:reset()			
		end
	end
end

function pg_conn:_run_next_query()
	if not self._queryActive then
		local top = self._queue[1]
		
		if top then
			self._queryActive = true
			self._queryStart = SysTime()
			self._queryWarn = false
			
			// query exists, but I don't use it. only preferable
			// when your query would return multiple result sets.
			// support for this does exist in the update loop.
			self._conn:query_params(top.query, unpack(top.args))
		end
	end
end

function pg_conn:shutdown()
	// free up the userdata
	self._conn = nil
	
	// empty everything, but continue to function. for whatever reason.
	self._queue = {}
	self._listening = {}	
	
	// this results in the next connection in the table getting skipped
	// for a single frame. not a big deal.
	for i,conn in ipairs(_connections) do
		if self._id == conn.id then
			table.remove(_connections, i)
			
			return
		end
	end
end

function pg_conn:reset()
	if not self._conn then error("pgsql: attempt to reset closed connection") end
	self._connectStart = SysTime()
	
	// discard the query that was running, just in case
	// it was a write or insert that ran successfully
	// before we handled it.
	if self._queryActive then
		table.remove(self._queue, 1)
		self._queryActive = false
		self._queryStart = nil
		self._queryWarn = nil
	end
	
	self._conn:reset()
end

local lastid = 0
function connect(conninfo)
	local conn = libpgsql.create(conninfo)
	
	local obj = setmetatable(table.Copy(pg_conn_skel), pg_conn)
		obj._conn = conn
		obj.id = lastid + 1
		obj._connectStart = SysTime()
	
	lastid = obj.id
	
	table.insert(_connections, obj)
	return obj
end

local function __update()
	for i, conn in ipairs(_connections) do
		local ret = { pcall(conn._update, conn) }
		
		if not ret[1] or ret[2] == false then
			ErrorNoHalt("pgsql: Updating connection #" .. conn.id .. " failed; removing.\n")
			
			if not ret[1] then
				ErrorNoHalt("pgsql: " .. tostring(ret[2]) .. "\n")
			end
		
			conn:shutdown()
		end
	end
end
hook.Add("Think", "pgsql.__update", __update)
hook.Add("GetGameDescription", "pgsql.__update", __update) // I'm sorry I'm sorry I'm sorry