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

NW_ENTITY_DATA = {}

if SERVER then
	require("transmittools")

	hook.Add("Tick", "NWTick", transmittools.NWTick)

	hook.Add("EntityRemoved", "NWCleanup", function(ent)
		if !IsValid(ent) then return end
		
		local index = ent:EntIndex()
		NW_ENTITY_DATA[index] = nil
		
		transmittools.EntityDestroyed(index)

		if ent:IsPlayer() then
			transmittools.PlayerDestroyed(index)
		end
	end)

	hook.Add("PlayerDisconnected", "NWCleanupPlayer", function(ply)
		local entindex = ply:EntIndex()
		transmittools.PlayerDestroyed(entindex)
		transmittools.EntityDestroyed(entindex)
	end)
end

local ApplyTableToTarget
if SERVER then
	ApplyTableToTarget = function(target)	
		local _t = target:GetTable()
		local etable = { _t = _t }
		local index = target:EntIndex()
		
		local mt = {
			__index = function (t, k)
				if NW_ENTITY_DATA[index][k] then 
					return NW_ENTITY_DATA[index][k]
				end
				
				return t._t[k]
			end,

			__newindex = function (t,k,v)
				local nwtable = t._t.__nwtable

				if nwtable[k] ~= nil then
					local entitydata = NW_ENTITY_DATA[index]
					
					if entitydata[k] ~= v then
						local ent = t.Entity or t.Weapon
						local nwvar = nwtable[k]

						if nwvar.type == NWTYPE_ENTITY then
							local nentindex = -1
							if IsValid(v) || v == GetWorldEntity() then
								nentindex = v:EntIndex()
							end
							transmittools.EntityVariableUpdated(index, nwvar.index-1, nentindex)
						end

						transmittools.VariableUpdated(index, nwvar.index-1)
						
						ent.__nwvalues[nwvar.index] = v
						NW_ENTITY_DATA[index][k] = v
					end
					
					return
				end

				t._t[k] = v
			end
		}

		setmetatable(etable, mt)
		target:SetTable(etable)
	end
else
	ApplyTableToTarget = function(target)	
		local _t = target:GetTable()
		local etable = { _t = _t }
		local index = target:EntIndex()
		
		local mt = {
			__index = function (t, k)
				if NW_ENTITY_DATA[index][k] then 
					return NW_ENTITY_DATA[index][k]
				end
				
				return t._t[k]
			end
		}

		setmetatable(etable, mt)
		target:SetTable(etable)
	end
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

	local index = target:EntIndex()
	if not NW_ENTITY_DATA[index] then 
		NW_ENTITY_DATA[index] = {} 
		if CLIENT then NW_ENTITY_DATA[index].__entity = target end
	end
	
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

		NW_ENTITY_DATA[index][name] = default

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

	if SERVER then transmittools.NetworkedEntityCreatedFinish(index) end

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

	if ent:IsPlayer() then
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
