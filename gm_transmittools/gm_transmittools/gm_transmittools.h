#define GAME_DLL 1
#define NO_MALLOC_OVERRIDE 1

#define WIN32_LEAN_AND_MEAN

#define MAX_GMOD_PLAYERS 128

#include "GMLuaModule.h"
extern ILuaInterface *g_pLua;

#include <vector>
#include <bitset>
#include <unordered_map>

//hl2sdk-ob-2e2ec01be7aa off the sourcemod mercurial
#include <server/cbase.h>
#include <inetchannel.h>

#include "bitbuf.h"

#include "nwtypes.h"
#include "nwrepl.h"
#include "nwentities.h"
#include "nwnetworking.h"

#include <windows.h>
#include "vfnhook.h"

int ResolveEntInfoOwner(EntInfo *ent);
int ResolveEHandleForEntity(int index);
int ResolveEHandleForEntity(ILuaObject *luaobject);

extern IVEngineServer *engine;

extern int umsgStringTableOffset;
extern std::bitset<MAX_EDICTS> sentEnts[MAX_GMOD_PLAYERS];
extern std::bitset<MAX_EDICTS> entityParity;