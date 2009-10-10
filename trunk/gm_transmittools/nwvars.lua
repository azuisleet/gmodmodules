AddCSLuaFile("nwvars.lua")
AddCSLuaFile("packet.lua")

REPL_EVERYONE = 0
REPL_PLAYERONLY = 1

NWTYPE_STRING = 0
NWTYPE_NUMBER = 1
NWTYPE_FLOAT = 2
NWTYPE_CHAR = 3
NWTYPE_SHORT = 4
NWTYPE_BOOL = 5
NWTYPE_BOOLEAN = NWTYPE_BOOL
NWTYPE_ANGLE = 6
NWTYPE_VECTOR = 7
NWTYPE_ENTITY = 8

if CLIENT then
	include("packet.lua")
end

NWDEBUG = false

if SERVER then
	require("transmittools")

	hook.Add("Tick", "NWTick", transmittools.NWTick)

	hook.Add("EntityRemoved", "NWCleanup", function(ent)
		if IsValid(ent) && !ent:IsPlayer() then
			transmittools.EntityDestroyed(ent:EntIndex())
		end
	end)

	hook.Add("PlayerDisconnected", "NWCleanupPlayer", function(ply)
		local entindex = ply:EntIndex()
		transmittools.PlayerDestroyed(entindex)
		transmittools.EntityDestroyed(entindex)
	end)

	hook.Add("PlayerThink", "PlayerCrashTest", function(ply)
		local time = transmittools.PlayerTimeout(ply:EntIndex())
		if time == nil then return end

		if time > 2 then
			ply._NetCrash = true
		elseif ply._NetCrash then
			ply._NetCrash = false
			transmittools.InvalidatePlayerCrashed(ply:EntIndex())
		end
	end)
end

local function ApplyTableToTarget(target)
	local _t = target:GetTable()
	local etable = { _t = _t }
    
	local mt = {
		__index = function (t,k)
			return t._t[k]
		end,

		__newindex = function (t,k,v)
			if t._t.__nwtable[k] && t._t[k] != v then
				local ent = t.Entity or t.Weapon
				local index = ent:EntIndex()
				local nwvar = ent.__nwtable[k]

				if nwvar.type == NWTYPE_ENTITY then
					local nentindex = -1
					if IsValid(v) || v == GetWorldEntity() then
						nentindex = v:EntIndex()
					end
					transmittools.EntityVariableUpdated(index, nwvar.index-1, nentindex)
				end

				transmittools.VariableUpdated(index, nwvar.index-1)

				ent.__nwvalues[nwvar.index] = v
			end

			t._t[k] = v
		end
	}

	setmetatable(etable, mt)
	target:SetTable(etable)
end

function RegisterNWTable(target, ntable)
	// make sure it's a table, it has entries, and that it's a table of tables
	if type(ntable) != "table" || #ntable == 0 || type(ntable[1]) != "table" then
		return
	end

	local index = target:EntIndex()

	if index == 0 && target != GetWorldEntity() then
		return
	end

	local nlookup = {}

	if SERVER then
		target.__nwvalues = {}
		transmittools.NetworkedEntityCreated(index, target.__nwvalues)
	end

	for i=1, #ntable do
		local table = ntable[i]

		local name, default, type, repl = table[1], table[2], table[3], table[4]

		if NWDEBUG then
			print(name, type, repl, default)
		end

		target[name] = default

		if CLIENT then
			// {name, type, repl, proxy}
			nlookup[i] = {name=name, type=type, repl=repl, proxy=table[5]}
		else
			// {index, type, repl}
			nlookup[name] = {index=i, type=type, repl=repl}
			transmittools.AddValue(index, i, type, repl)

			target.__nwvalues[i] = default
		end
	end

	target.__nwtable = nlookup


	if CLIENT then return end

	transmittools.NetworkedEntityCreatedFinish(index)

	ApplyTableToTarget(target)
end

if !GetWorldEntity then
	GetWorldEntity = function() return Entity(0) end
end

local GlobalTable = {}
local PlayerTable = {}

local function SetupGlobalTable()
	RegisterNWTable(GetWorldEntity(), GlobalTable)
end

hook.Add("OnEntityCreated", "SetupNWTablePlayer", function(ent)
	if !IsValid(ent) then return end

	if ent:GetClass() == "player" then
		if SERVER then
			transmittools.PlayerCreated(ent:EntIndex())
		end
		if !ent.__nwtable then
			RegisterNWTable(ent, PlayerTable)
		end
	end
end)

local function MergeTablesI(a, b)
	for k,v in ipairs(b) do
		table.insert(a, v)
	end
end

function RegisterNWTableGlobal(nwtable)
	hook.Add("InitPostEntity", "SetupNWTableGlobal", SetupGlobalTable)

	MergeTablesI(GlobalTable, nwtable)
end

function RegisterNWTablePlayer(nwtable)
	MergeTablesI(PlayerTable, nwtable)
end
