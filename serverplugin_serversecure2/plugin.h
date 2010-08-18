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

#include <time.h>

#ifdef WIN32
#include <winsock2.h>
#include <detours.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "filecheck.h"
#include "netfilter.h"
#include "validation.h"

extern IVEngineServer		*engine; 
extern IPlayerInfoManager	*playerinfomanager;
extern IServerPluginHelpers *helpers;
extern CGlobalVars			*gpGlobals;
