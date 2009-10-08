#define _RETAIL 1
#define GAME_DLL 1
#define WIN32_LEAN_AND_MEAN

#define MAX_GMOD_PLAYERS 128

#include "GMLuaModule.h"
extern ILuaInterface *g_pLua;

#include <vector>
#include <bitset>
#include <unordered_map>

#include "engine/iserverplugin.h"
#include "game/server/iplayerinfo.h"
#include "eiface.h"
#include "irecipientfilter.h"
#include "networkstringtabledefs.h"
#include "inetchannelinfo.h"
#include "inetchannel.h"

/* oh dear */
#include "predictable_entity.h"
class ISave;
class IRestore;
class CTakeDamageInfo;
class touchlink_t;
class groundlink_t;
#include "variant_t.h"
#include "predictioncopy.h"
class CBasePlayer;
#include "baseentity.h"
/* ok */

#include "bitbuf.h"

#include "nwtypes.h"
#include "nwrepl.h"
#include "nwentities.h"
#include "nwnetworking.h"

#include <windows.h>
#include <detours.h>

int ResolveEntInfoOwner(EntInfo *ent);
int ResolveEHandleForEntity(ILuaObject *luaobject);

extern IVEngineServer *engine;

extern int umsgStringTableOffset;
extern std::bitset<MAX_EDICTS> sentEnts[MAX_GMOD_PLAYERS];