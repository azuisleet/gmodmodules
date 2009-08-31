local player = player
local hook = hook
local RealTime = RealTime
local IsValid = IsValid
local GAMEMODE = GAMEMODE

module("nwvarcrashtest")

// crash code, when they crash we need to update
hook.Add("PlayerThink", "PlayerCrashTest", function(ply)
	if RealTime() > ply.__mtime + 2 then
		if ply.__moves == 0 && !ply:InVehicle() then
			ply.__crashtrasfrophy = true
		elseif ply.__crashtrasfrophy && ply.__moves > 20 then
			hook.Call("PlayerCrashedHorribly", GAMEMODE, ply)
			ply.__crashtrasfrophy = false
		end

		ply.__mtime = RealTime()
		ply.__moves = 0
	end
end)

hook.Add("OnEntityCreated", "SetupCrashTest", function(ent)
	if !IsValid(ent) then return end
	if ent:GetClass() == "player" then
		ent.__crashtrasfrophy = false
		ent.__moves = 0
		ent.__mtime = 0
	end
end)

hook.Add("Move", "CrashTestGo", function(ply)
	if IsValid(ply) && ply.__moves then
		ply.__moves = ply.__moves + 1
	end
end)