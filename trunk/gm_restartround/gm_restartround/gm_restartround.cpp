#define _RETAIL 1
#define GAME_DLL 1

//hl2sdk-ob-2e2ec01be7aa off the sourcemod mercurial
#include <server/cbase.h>
#include <server/eventqueue.h>
#include <server/mapentities.h>

#define WIN32_LEAN_AND_MEAN
#include "GMLuaModule.h"
#include "gm_restartround.h"
#include "sigscan.h"

IVEngineServer *engine = NULL;

// g_EventQueue
#define LETSSTEALGEVENTQUEUE "\xC7\x40\x0C\xFF\xFF\xFF\xFF\x52\xB9"
#define LETSSTEALGEVENTQUEUEMASK "xxxxxxxxx"
#define LETSSTEALGEVENTQUEUELEN 9
CSigScan LETSSTEALGEVENTQUEUE_Sig;
CEventQueue *g_eventQueue_f;
// g_EventQueue


// g_MapEntityRefs
#define LETSSTEALGMAPENTITYREFS "\x89\x4A\x48\xB9"
#define LETSSTEALGMAPENTITYREFSMASK "xxxx"
#define LETSSTEALGMAPENTITYREFSLEN 4
CSigScan LETSSTEALGMAPENTITYREFS_Sig;
class CMapEntityRef
{
public:
	int		m_iEdict;
	int		m_iSerialNumber;
};
CUtlLinkedList<CMapEntityRef, unsigned short> *g_MapEntityRefs_f;	
//g_MapEntityRefs

// gEntList
#define LETSSTEALGENTLIST "\xFF\xD5\x83\xC4\x08\x6A\x00\xB9"
#define LETSSTEALGENTLISTMASK "xxxxxxxx"
#define LETSSTEALGENTLISTLEN 8
CSigScan LETSSTEALGENTLIST_Sig;
CGlobalEntityList *gEntList_f;

#define CLEANUP "\x56\xC6\x05\x00\x00\x00\x00\x01\xE8\x00\x00\x00\x00\x33\xF6\x39\x35"
#define CLEANUPMASK "xxx????xx????xxxx"
#define CLEANUPLEN 16
CSigScan CLEANUP_Sig;
typedef void (*CleanupEnts_t)(void);
void (*CleanupEnts)();
// gEntList

//UTIL_Remove
#define UTILREMOVE "\x8B\x44\x24\x04\x85\xC0\x74\x0C\x83\xC0\x0C"
#define UTILREMOVEMASK "xxxxxxxxxxx"
#define UTILREMOVELEN 11
CSigScan UTILREMOVE_Sig;
typedef void (*UTIL_Remove_t)(CBaseEntity *);
void (*UTIL_Remove_f)(CBaseEntity *);
//UTIL_Remove

//CreateEntityByName
#define CREATEENTITY "\x56\x8B\x74\x24\x0C\x57\x8B\x7C\x24\x0C\x83\xFE\xFF"
#define CREATEENTITYMASK "xxxxxxxxxxxxx"
#define CREATEENTITYLEN 13
CSigScan CREATEENTITY_Sig;
typedef CBaseEntity *(*CreateEntityByName_t)(const char *, int);
CBaseEntity *(*CreateEntityByName_f)(const char *, int);
//CreateEntityByName


//MapEntity_ParseAllEntities
#define MAPENTITY "\xB8\x28\x10\x00\x00"
#define MAPENTITYMASK "xxxxx"
#define MAPENTITYLEN 5
CSigScan MAPENTITY_Sig;
typedef void (*MapEntity_ParseAllEntities_t)(const char *, IMapEntityFilter *, bool);
void (*MapEntity_ParseAllEntities_f)(const char *, IMapEntityFilter *, bool);
//MapEntity_ParseAllEntities

void CEventQueue::Clear( void )
{
	// delete all the events in the queue
	EventQueuePrioritizedEvent_t *pe = m_Events.m_pNext;

	while ( pe != NULL )
	{
		EventQueuePrioritizedEvent_t *next = pe->m_pNext;
		delete pe;
		pe = next;
	}

	m_Events.m_pNext = NULL;
}

CBaseEntity *CGlobalEntityList::NextEnt( CBaseEntity *pCurrentEnt ) 
{ 
	if ( !pCurrentEnt )
	{
		const CEntInfo *pInfo = FirstEntInfo();
		if ( !pInfo )
			return NULL;

		return (CBaseEntity *)pInfo->m_pEntity;
	}

	const CEntInfo *pList = GetEntInfoPtr( pCurrentEnt->GetRefEHandle() );
	if ( pList )
		pList = NextEntInfo(pList);

	while ( pList )
	{
		return (CBaseEntity *)pList->m_pEntity;
		pList = pList->m_pNext;
	}

	return NULL; 

}

bool PreserveEntity(ILuaInterface *gLua, const char *classname)
{
	if(Q_stricmp( classname, "worldspawn" ) == 0)
		return true;

	ILuaObject *func = gLua->GetGlobal("PreserveEntity");
	gLua->Push(func);
	gLua->Push(classname);
	gLua->Call(1, 1);

	ILuaObject *returno = gLua->GetReturn(0);
	bool preserve = returno->GetBool();

	return preserve;
}

LUA_FUNCTION(restart_round)
{
	ILuaInterface *gLua = Lua();

	CBaseEntity *pCur = gEntList_f->FirstEnt();
	while ( pCur )
	{
		if ( !PreserveEntity(gLua, pCur->GetClassname()) )
		{
			UTIL_Remove_f( pCur );
		}

		pCur = gEntList_f->NextEnt( pCur );
	}

	g_eventQueue_f->Clear();

	_asm mov ecx, gEntList_f
	CleanupEnts();

	class CMapEntityFilter : public IMapEntityFilter
	{
	public:

		virtual bool ShouldCreateEntity( const char *pClassname )
		{
			if ( !PreserveEntity( gLua, pClassname ) )
				return true;

			if ( m_iIterator != g_MapEntityRefs_f->InvalidIndex() )
			{
				m_iIterator = g_MapEntityRefs_f->Next( m_iIterator );
			}

			return false;
		}


		virtual CBaseEntity* CreateNextEntity( const char *pClassname )
		{
			if ( m_iIterator == g_MapEntityRefs_f->InvalidIndex() )
			{
				gLua->Error("ERROR DURING RESPAWN ERROR ERROR");
				return NULL;
			}
			else
			{
				CMapEntityRef &ref = (*g_MapEntityRefs_f)[m_iIterator];
				m_iIterator = g_MapEntityRefs_f->Next( m_iIterator );

				if ( ref.m_iEdict == -1 || engine->PEntityOfEntIndex( ref.m_iEdict ) )
				{
					return CreateEntityByName_f( pClassname, -1 );
				}
				else
				{
					return CreateEntityByName_f( pClassname, ref.m_iEdict );
				}
			}
		}

	public:
		int m_iIterator;
		ILuaInterface *gLua;
	};
	CMapEntityFilter filter;
	filter.m_iIterator = g_MapEntityRefs_f->Head();
	filter.gLua = gLua;

	// DO NOT CALL SPAWN ON info_node ENTITIES!

	MapEntity_ParseAllEntities_f( engine->GetMapEntitiesString(), &filter, true );

	return 0;
}

int Start(lua_State *L)
{
	CreateInterfaceFn interfaceFactory = Sys_GetFactory( "engine.dll" );
	CreateInterfaceFn gameServerFactory = Sys_GetFactory( "server.dll" );

	engine = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL);

	CSigScan::sigscan_dllfunc = (CreateInterfaceFn)gameServerFactory(INTERFACEVERSION_PLAYERINFOMANAGER,NULL);
	bool scan = CSigScan::GetDllMemInfo();

	// g_EventQueue
	LETSSTEALGEVENTQUEUE_Sig.Init((unsigned char *)LETSSTEALGEVENTQUEUE, LETSSTEALGEVENTQUEUEMASK, LETSSTEALGEVENTQUEUELEN);
	unsigned char *addr = (unsigned char *)LETSSTEALGEVENTQUEUE_Sig.sig_addr;
	g_eventQueue_f = *((CEventQueue **)(addr + LETSSTEALGEVENTQUEUELEN)); 
	// g_EventQueue

	// g_MapEntityRefs
	LETSSTEALGMAPENTITYREFS_Sig.Init((unsigned char *)LETSSTEALGMAPENTITYREFS, LETSSTEALGMAPENTITYREFSMASK, LETSSTEALGMAPENTITYREFSLEN);
	addr = (unsigned char *)LETSSTEALGMAPENTITYREFS_Sig.sig_addr;
	g_MapEntityRefs_f = *((CUtlLinkedList<CMapEntityRef, unsigned short> **)(addr + LETSSTEALGMAPENTITYREFSLEN)); 
	// g_MapEntityRefs

	// gEntList
	LETSSTEALGENTLIST_Sig.Init((unsigned char *) LETSSTEALGENTLIST,  LETSSTEALGENTLISTMASK,  LETSSTEALGENTLISTLEN);
	addr = (unsigned char *)LETSSTEALGENTLIST_Sig.sig_addr;
	gEntList_f = *((CGlobalEntityList **)(addr +  LETSSTEALGENTLISTLEN));

	CLEANUP_Sig.Init((unsigned char *) CLEANUP, CLEANUPMASK, CLEANUPLEN);
	CleanupEnts = (CleanupEnts_t)(CLEANUP_Sig.sig_addr);
	// gEntList

	// UTIL_Remove
	UTILREMOVE_Sig.Init((unsigned char *) UTILREMOVE, UTILREMOVEMASK, UTILREMOVELEN);
	UTIL_Remove_f = (UTIL_Remove_t)(UTILREMOVE_Sig.sig_addr);
	// UTIL_Remove

	// CreateEntityByName
	CREATEENTITY_Sig.Init((unsigned char *) CREATEENTITY, CREATEENTITYMASK, CREATEENTITYLEN);
	CreateEntityByName_f = (CreateEntityByName_t)(CREATEENTITY_Sig.sig_addr);
	// CreateEntityByName

	// MapEntity_ParseAllEntities
	MAPENTITY_Sig.Init((unsigned char *) MAPENTITY, MAPENTITYMASK, MAPENTITYLEN);
	MapEntity_ParseAllEntities_f = (MapEntity_ParseAllEntities_t)(MAPENTITY_Sig.sig_addr);
	// MapEntity_ParseAllEntities

	ILuaInterface *gLua = Lua();

	gLua->SetGlobal("restart_round", restart_round);

	return 0;
}

int Close(lua_State *L)
{
	return 0;
}

/*
static const char *s_PreserveEnts[] =
{
	"player",
	"viewmodel",
	"worldspawn",
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
	"point_commentary_viewpoint",
	"", // END Marker
};

void CTeamplayRoundBasedRules::CleanUpMap()
{
	// Get rid of all entities except players.
	CBaseEntity *pCur = gEntList.FirstEnt();
	while ( pCur )
	{
		if ( !RoundCleanupShouldIgnore( pCur ) )
		{
			UTIL_Remove( pCur );
		}

		pCur = gEntList.NextEnt( pCur );
	}

	// Clear out the event queue
	g_EventQueue.Clear();

	// Really remove the entities so we can have access to their slots below.
	gEntList.CleanupDeleteList();

	engine->AllowImmediateEdictReuse();

	// Now reload the map entities.
	class CTeamplayMapEntityFilter : public IMapEntityFilter
	{
	public:

		virtual bool ShouldCreateEntity( const char *pClassname )
		{
			// Don't recreate the preserved entities.
			if ( m_pRules->ShouldCreateEntity( pClassname ) )
				return true;

			// Increment our iterator since it's not going to call CreateNextEntity for this ent.
			if ( m_iIterator != g_MapEntityRefs.InvalidIndex() )
			{
				m_iIterator = g_MapEntityRefs.Next( m_iIterator );
			}

			return false;
		}


		virtual CBaseEntity* CreateNextEntity( const char *pClassname )
		{
			if ( m_iIterator == g_MapEntityRefs.InvalidIndex() )
			{
				// This shouldn't be possible. When we loaded the map, it should have used 
				// CTeamplayMapEntityFilter, which should have built the g_MapEntityRefs list
				// with the same list of entities we're referring to here.
				Assert( false );
				return NULL;
			}
			else
			{
				CMapEntityRef &ref = g_MapEntityRefs[m_iIterator];
				m_iIterator = g_MapEntityRefs.Next( m_iIterator );	// Seek to the next entity.

				if ( ref.m_iEdict == -1 || engine->PEntityOfEntIndex( ref.m_iEdict ) )
				{
					// Doh! The entity was delete and its slot was reused.
					// Just use any old edict slot. This case sucks because we lose the baseline.
					return CreateEntityByName( pClassname );
				}
				else
				{
					// Cool, the slot where this entity was is free again (most likely, the entity was 
					// freed above). Now create an entity with this specific index.
					return CreateEntityByName( pClassname, ref.m_iEdict );
				}
			}
		}

	public:
		int m_iIterator; // Iterator into g_MapEntityRefs.
		CTeamplayRoundBasedRules *m_pRules;
	};
	CTeamplayMapEntityFilter filter;
	filter.m_iIterator = g_MapEntityRefs.Head();

	// DO NOT CALL SPAWN ON info_node ENTITIES!

	MapEntity_ParseAllEntities( engine->GetMapEntitiesString(), &filter, true );
}
*/