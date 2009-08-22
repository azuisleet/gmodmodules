#define _RETAIL
#define GAME_DLL
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

/* oh dear */

class CTakeDamageInfo;
struct touchlink_t;
struct groundlink_t;
enum notify_system_event_t;
#include "predictioncopy.h"
#include "predictable_entity.h"
class ISave;
class IRestore;
#include "variant_t.h"
#include "entitylist.h"
#include "cbase.h"

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

extern int umsgStringTableOffset;
extern std::bitset<MAX_EDICTS> sentEnts[MAX_GMOD_PLAYERS];