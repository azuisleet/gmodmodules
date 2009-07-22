require("luaprof")

local hook = hook

local StartProfiler = StartProfiler
local ResolveTimer = ResolveTimer
local StopProfiler = StopProfiler
local SetHook = SetHook

local print = print
local table = table
local next = next
local pairs = pairs
local file = file
local string = string
local math = math
local tostring = tostring
local PrintTable = PrintTable
local type = type

module("profiler")

// lua_run require("profiler") profiler.Start()
// lua_run function a() for i=1,100000000 do end end a()
// lua_run profiler.Dump()

local function WriteTableToFile(maxlen, filen, sorttable)
	local buffer = {}
	local format = "%" .. tostring(maxlen) .. "s avg time: %.6f max time: %.6f num calls: %d"

	for k,v in pairs(sorttable) do
		table.insert(buffer, string.format(format, v[1], v[2], v[3], v[4]))
	end
	file.Write(filen .. ".txt", table.concat(buffer, "\n"))
end

function Dump()
	StopProfiler()

	PrintTable(FuncData)

	local maxlen = 1
	local sorttable = {}
	for k,v in pairs(FuncData) do
		maxlen = math.max(maxlen, #k)
		table.insert(sorttable, {k, v.avg, v.max, v.numCalls})
	end

	table.sort(sorttable, function(a, b) return a[3]>b[3] end)
	WriteTableToFile(maxlen, "profiler_functions_sorted_by_time", sorttable)

	table.sort(sorttable, function(a, b) return a[4]>b[4] end)
	WriteTableToFile(maxlen, "profiler_functions_sorted_by_calls", sorttable)
end

function Start()
	StartProfiler()
end

function Stop()
	StopProfiler()
end