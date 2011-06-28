require("slog")

SLOGBlocked = {
	"dump_hooks",
	"soundscape_flush",
	"hammer_update_entity",
	"physics_constraints",
	"physics_debug_entity",
	"physics_select",
	"physics_budget",
	"sv_soundemitter_flush",
	"rr_reloadresponsesystems",
	"sv_soundemitter_filecheck",
	"sv_soundscape_printdebuginfo",
	"dumpentityfactories",
	"dump_globals",
	"dump_entity_sizes",
	"dumpeventqueue",
	"dbghist_addline",
	"dbghist_dump",
	"groundlist",
	"report_simthinklist",
	"report_entities",
	"server_game_time",
	"npc_thinknow"
}

local DontShow = {
	"headcrab",
	"say",
	"gmt_",
	"gtower_",
	"gm_",
	"dr_",
	"st_",
	"rp_",
	"rs_",
	"wire_",
	"gms_",
	"status",
	"kill",
	"myinfo",
	"phys_swap",
	"+ass_menu",
	"-ass_menu",
	"explode",
	"suitzoom",
	"feign_death",
	"gm_showhelp",
	"gm_showteam",
	"gm_showspare1",
	"gm_showspare2",
	"+gm_special",
	"-gm_special",
	"vban",
	"vmodenable",
	"se auth",
	"ulib_cl_ready",
	"noclip",
	"kill",
	"undo",
	"jukebox",
	"debugplayer",
	"gmod_undo",
	"cnc"
}

function Cmd_RecvCommand(Name, Buffer)
	local Ply = false
	
	for k,v in ipairs(player.GetAll()) do
		if IsValid(v) && v:Name() == Name then
			Ply = v
			break
		end
	end
	
	// don't bother with any commands executed by a NULL player, including say or crash commands
	if !IsValid(Ply) then
		return true
	end

	local clean = string.Trim(Buffer)
	local lower = string.lower(clean)

	// don't log these commands
	for k,v in ipairs(DontShow) do
		if string.sub(lower, 1, string.len(v)) == string.lower(v) then
			return false
		end
	end
	
	local name = Ply:Name()
	local steamid = Ply:SteamID()
	local blocked = false
	local printmessage = nil

	for k,v in pairs(SLOGBlocked) do
		if lower == v || string.find(lower, v) != nil then
			blocked = true
			printmessage = "#Found Blocked Command: "..string.format("%s (%s) Ran: %s\n", name, steamid, clean)
			break
		end
	end
	
	local log = string.format("[%s] %s (%s) Ran: %s\r\n", os.date("%c"), name, steamid, clean)
	AppendLog(string.Replace(steamid, ":", "_"), log)

	if blocked then
		AppendLog("Blocked", log)
	else
		printmessage = "#"..log
	end
	
	if printmessage then
		for k,v in ipairs(player.GetAll()) do
			if IsValid(v) && v:IsSuperAdmin() then
				if blocked then
					v:PrintMessage(HUD_PRINTTALK, printmessage)
				else
					v:PrintMessage(HUD_PRINTCONSOLE, printmessage)
				end
			end
		end
	end

	if string.find(lower, "lua_run_cl") &&
	 ( string.find(lower, "select v from ratings") ||  string.find(lower, "http") || string.find(lower, "slob") || string.find(lower, "runstring") ) then

		Ply:Kick("You have a malicious bind, type 'key_findbinding lua_run_cl' and rebind those keys")
		return true
	end

	return blocked
end

hook.Add("SetupMove", "InfAngleGuard", function(ply, cmd)
	local ang = cmd:GetMoveAngles()

	if not (ang.p >= 0 or ang.p <= 0) or not (ang.y >= 0 or ang.y <= 0) or not (ang.r >= 0 or ang.r <= 0) then
		cmd:SetMoveAngles(Angle(0, 0, 0))
		ply:SetEyeAngles(Angle(0, 0, 0))
	end
end)