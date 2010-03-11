#pragma once

#include <stdio.h>

#include "interface.h"
#include "engine/iserverplugin.h"
#include "game/server/iplayerinfo.h"
#include "eiface.h"
#include "igameevents.h"
#include "convar.h"
#include "tier1.h"
#include "inetchannel.h"
#include "netadr.h"

#include <winsock2.h>
#include <time.h>

#include <detours.h>
#include <boost/unordered_map.hpp>

#include "validation.h"
#include "filecheck.h"
#include "netfilter.h"

bool lookup_userid(int userid);

extern IVEngineServer		*engine; 
extern IPlayerInfoManager	*playerinfomanager;
extern IServerPluginHelpers *helpers;
extern CGlobalVars			*gpGlobals;