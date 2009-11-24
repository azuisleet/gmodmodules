require("restartround")

local preserve = {"player",
	"viewmodel",
	"worldspawn",
	"predicted_viewmodel",
	"player_manager",
	"soundent",
	"ai_network",
	"ai_hint",
	"env_soundscape",
	"env_soundscape_proxy",
	"env_soundscape_triggerable",
	"env_sprite",
	"env_sun",
	"env_wind",
	"env_fog_controller",
	"func_wall",
	"func_illusionary",
	"func_brush",
	"info_node",
	"info_target",
	"info_node_hint",
	"point_commentary_node",
	"point_viewcontrol",
	"func_precipitation",
	"func_team_wall",
	"shadow_control",
	"sky_camera",
	"scene_manager",
	"trigger_soundscape",
	"commentary_auto",
	"point_commentary_node",
	"point_commentary_viewpoint"}

function PreserveEntity(classname)
	return table.HasValue(preserve, classname)
end

function DoRoundRestart()
	restart_round()
	GAMEMODE.SpawnPoints = nil
end