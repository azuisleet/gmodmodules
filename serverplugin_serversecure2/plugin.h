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
#include "tier1/utlstack.h"

#include <winsock2.h>
#include <time.h>

#include <detours.h>

#include "filecheck.h"
#include "netfilter.h"
#include "validation.h"

extern IVEngineServer		*engine; 
extern IPlayerInfoManager	*playerinfomanager;
extern IServerPluginHelpers *helpers;
extern CGlobalVars			*gpGlobals;
