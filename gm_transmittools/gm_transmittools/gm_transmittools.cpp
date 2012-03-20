#include "gm_transmittools.h"
#include "sigscan.h"

#define INTERFACEVERSION_VENGINESERVERGMOD	"VEngineServerGMod021"
#define INTERFACEVERSION_SERVERGAMEDLLGMOD "ServerGameDLL_GMOD_007"

GMOD_MODULE(Start, Close)

ILuaInterface *g_pLua = NULL;
IVEngineServer	*engine = NULL;
INetworkStringTableContainer *networkstringtable = NULL;
IServerGameEnts *gameents = NULL;
IServerGameDLL *gamedll = NULL;

int umsgStringTableOffset;
//int tickinterval;


// gEntList, from Sourcemod gamedata
#define LEVELSHUTDOWN "\xE8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\xB9\x00\x00\x00\x00\xE8\x00\x00\x00\x00\xE8"
#define LEVELSHUTDOWNMASK "x????x????x????x????x????x"
#define LEVELSHUTDOWNLEN 26
#define ENTLISTOFFSET 16

CSigScan LEVELSHUTDOWN_Sig;
CBaseEntityList * g_pEntityList;

std::bitset<MAX_EDICTS> sentEnts[MAX_GMOD_PLAYERS];

CBaseEntity *GetBaseEntityFromEntInfo(int index)
{
	edict_t *pEdict = engine->PEntityOfEntIndex( index );
	if(pEdict == NULL)
		return NULL;

	IServerUnknown *pUnknown = pEdict->GetUnknown();
	if(pUnknown == NULL)
		return NULL;

	return pUnknown->GetBaseEntity();
}

int ResolveEntInfoOwner(EntInfo *ent)
{
	CBaseEntity *entity = GetBaseEntityFromEntInfo(ent->entindex);
	if(entity == NULL)
		return -1;

	return -1;

/*	CBaseEntity *owner = entity->m_hOwnerEntity;
	if(owner == NULL)
		return -1;

	edict_t *pOwnerEdict = owner->edict();
	edict_t *pBaseEdict = engine->PEntityOfEntIndex( 0 );
	return pOwnerEdict - pBaseEdict;*/
}

int ResolveEHandleForEntity(ILuaObject *luaobject)
{
	if(luaobject == NULL)
		return 0;

	CBaseHandle *handle = (CBaseHandle*)luaobject->GetUserData();
	if(handle == NULL || !handle->IsValid())
		return 0;

	if ( handle->Get() )
	{
		int iSerialNum = handle->GetSerialNumber() & (1 << NUM_NETWORKED_EHANDLE_SERIAL_NUMBER_BITS) - 1;
		return handle->GetEntryIndex() | (iSerialNum << MAX_EDICT_BITS);
	}
	else
	{
		return INVALID_NETWORKED_EHANDLE_VALUE;
	}
}

DWORD* GetInterfaceFuncPtr(DWORD* pDeviceInterface, const char *fmt, ...) 
{   
	va_list	va_alist;
	char	buf[32];

	memset(buf, 0, sizeof(buf));

	va_start(va_alist, fmt);
	_vsnprintf_s(buf, sizeof(buf), fmt, va_alist);
	va_end(va_alist);

	char *op = (char *)strtoul(buf, NULL, 16);

	while(1)
	{
		if(*op == '\xFF')
		{
			op++;

			if(*op == '\xA0')
			{
				int iIndex = 0;

				op++;
				memcpy(&iIndex, op, (4));
				return (DWORD*)(*pDeviceInterface + iIndex  );
			}

			op++;
			break;
		}
		op++;
	}

	if (((int)*op)  < 0)
		return (DWORD*)*pDeviceInterface;

	return (DWORD*)(*pDeviceInterface + ((int)*op ) ); //Credits: as2 (G-D) 
}

class CDetour
{
public:
	void CheckTransmit(CCheckTransmitInfo *pInfo, const unsigned short *pEdictIndices, int nEdict);
	static void (CDetour::* CheckTrampoline)(CCheckTransmitInfo *pInfo, const unsigned short *pEdictIndices, int nEdict);

};
typedef void (__thiscall CDetour::* *checktransmit_t)(CCheckTransmitInfo *, const unsigned short *, int);

void (CDetour::* CDetour::CheckTrampoline)(CCheckTransmitInfo *pInfo, const unsigned short *pEdictIndices, int nEdict) = 0;

void CDetour::CheckTransmit(CCheckTransmitInfo *pInfo, const unsigned short *pEdictIndices, int nEdict)
{
	(this->*CheckTrampoline)(pInfo, pEdictIndices, nEdict);

	edict_t *pBaseEdict = engine->PEntityOfEntIndex( 0 );
	int client = pInfo->m_pClientEnt - pBaseEdict;

	for ( int i=0; i < nEdict; i++ )
	{
		int iEdict = pEdictIndices[i];

		if(sentEnts[client-1][iEdict])
			continue;

		if( pInfo->m_pTransmitEdict->Get( iEdict ) )
		{
			sentEnts[client-1][iEdict] = true;
			EntityTransmittedToPlayer(iEdict, client);
		}
	}
}

LUA_FUNCTION(DebugDump)
{
	ILuaInterface *gLua = Lua();
	gLua->CheckType(1, GLua::TYPE_NUMBER);

	int debug = gLua->GetNumber(1);

	switch(debug)
	{
	case 1:
		for(actpl::const_iterator iter = ActivePlayers.begin(); iter != ActivePlayers.end(); ++iter)
		{
			printf("Active player %d\n", *iter);	
		}
	break;
	case 2:
		for(netents::const_iterator iter = NetworkedEntities.begin(); iter != NetworkedEntities.end(); ++iter)
		{
			EntInfo *ent = iter->second;
			printf("Net ent %d (complete %d)\n", ent->entindex, ent->complete);
			printf("Ultimate transmit %s\n", ent->ultimateTransmit.to_string().c_str());
			for(ValueVector::const_iterator iter = ent->values.begin(); iter != ent->values.end(); ++iter)
			{
				printf("%d %s\n%s\n", (*iter).tableoffset, (*iter).currentTransmit.to_string().c_str(), (*iter).finalTransmit.to_string().c_str());
			}
		}
	break;
	case 3:
		for(entinfovec::const_iterator iter = IncompleteEntities.begin(); iter != IncompleteEntities.end(); ++iter)
		{
			printf("Incomplete entity %d\n", (*iter)->entindex);
		}
	break;
	}

	return 0;
}
LUA_FUNCTION(DebugVis)
{
	ILuaInterface *gLua = Lua();
	gLua->CheckType(1, GLua::TYPE_NUMBER);
	gLua->CheckType(2, GLua::TYPE_NUMBER);

	int entindex = gLua->GetInteger(1);
	int playerindex = gLua->GetInteger(2);

	sentEnts[playerindex-1][entindex] = true;
	printf("Entity %d transmitted to player %d (%d)\n", entindex, playerindex, (sentEnts[playerindex-1][entindex] == true));

	EntityTransmittedToPlayer(entindex, playerindex);

	return 0;
}

LUA_FUNCTION(DebugIncompleteSize)
{
	printf("Incomplete entities: %d\n", IncompleteEntities.size());
	return 0;
}

LUA_FUNCTION(DebugMasks)
{
	ILuaInterface *gLua = Lua();
	gLua->CheckType(1, GLua::TYPE_NUMBER);

	int index = gLua->GetNumber(1);

	netents::const_iterator iter = NetworkedEntities.find(index);
	if(iter == NetworkedEntities.end())
	{
		printf("Entity %d is not networked\n", index);
		return 0;
	}

	EntInfo *ent = iter->second;

	printf("Net ent %d (complete %d)\n", ent->entindex, ent->complete);
	printf("Ultimate transmit %s\n", ent->ultimateTransmit.to_string().c_str());
	for(ValueVector::const_iterator iter = ent->values.begin(); iter != ent->values.end(); ++iter)
	{
		printf("%d %s\n%s\n", (*iter).tableoffset, (*iter).currentTransmit.to_string().c_str(), (*iter).finalTransmit.to_string().c_str());
	}
	return 0;
}

LUA_FUNCTION(tt_NetworkedEntityCreated)
{
	ILuaInterface *gLua = Lua();
	gLua->CheckType(1, GLua::TYPE_NUMBER);
	gLua->CheckType(2, GLua::TYPE_TABLE);

	int entindex = gLua->GetInteger(1);
	int tableref = gLua->GetReference(2);

	NetworkedEntityCreated(entindex, tableref);

	return 0;
}

LUA_FUNCTION(tt_NetworkEntityCreatedFinish)
{
	ILuaInterface *gLua = Lua();
	gLua->CheckType(1, GLua::TYPE_NUMBER);

	int entindex = gLua->GetInteger(1);

	NetworkedEntityCreatedFinish(entindex);

	return 0;
}

LUA_FUNCTION(tt_PlayerCreated)
{
	ILuaInterface *gLua = Lua();
	gLua->CheckType(1, GLua::TYPE_NUMBER);

	PlayerCreated(gLua->GetInteger(1));

	return 0;
}

LUA_FUNCTION(tt_EntityDestroyed)
{
	ILuaInterface *gLua = Lua();
	gLua->CheckType(1, GLua::TYPE_NUMBER);

	EntityDestroyed(gLua->GetInteger(1));

	return 0;
}

LUA_FUNCTION(tt_PlayerDestroyed)
{
	ILuaInterface *gLua = Lua();
	gLua->CheckType(1, GLua::TYPE_NUMBER);

	PlayerDestroyed(gLua->GetInteger(1));

	return 0;
}

LUA_FUNCTION(tt_AddValue)
{
	ILuaInterface *gLua = Lua();
	gLua->CheckType(1, GLua::TYPE_NUMBER);
	gLua->CheckType(2, GLua::TYPE_NUMBER);
	gLua->CheckType(3, GLua::TYPE_NUMBER);
	gLua->CheckType(4, GLua::TYPE_NUMBER);

	AddValueToEntityValues(gLua->GetInteger(1), gLua->GetInteger(2), gLua->GetInteger(3), gLua->GetInteger(4));

	return 0;
}

LUA_FUNCTION(tt_VariableUpdated)
{
	ILuaInterface *gLua = Lua();
	gLua->CheckType(1, GLua::TYPE_NUMBER);
	gLua->CheckType(2, GLua::TYPE_NUMBER);

	InvalidateEntityVariableUpdate(gLua->GetInteger(1), gLua->GetInteger(2));

	return 0;
}

LUA_FUNCTION(tt_EntityVariableUpdated)
{
	ILuaInterface *gLua = Lua();
	gLua->CheckType(1, GLua::TYPE_NUMBER);
	gLua->CheckType(2, GLua::TYPE_NUMBER);
	gLua->CheckType(3, GLua::TYPE_NUMBER);

	EntitySetValueEntIndex(gLua->GetInteger(1), gLua->GetInteger(2), gLua->GetInteger(3));

	return 0;
}

LUA_FUNCTION(tt_InvalidatePlayerCrashed)
{
	ILuaInterface *gLua = Lua();
	gLua->CheckType(1, GLua::TYPE_NUMBER);

	InvalidatePlayerCrashed(gLua->GetInteger(1));

	return 0;
}

LUA_FUNCTION(tt_PlayerTimeout)
{
	ILuaInterface *gLua = Lua();
	gLua->CheckType(1, GLua::TYPE_NUMBER);

	INetChannelInfo *channel = engine->GetPlayerNetInfo(gLua->GetInteger(1));
	if(channel == NULL)
		return 0;

	gLua->Push(channel->GetTimeSinceLastReceived());

	return 1;
}

int Start(lua_State *L)
{
	ILuaInterface *gLua = Lua();

	CreateInterfaceFn engineFactory = Sys_GetFactory( "engine.dll" );
	CreateInterfaceFn gameServerFactory = Sys_GetFactory( "server.dll" );

	engine = (IVEngineServer *)engineFactory(INTERFACEVERSION_VENGINESERVERGMOD, NULL);
	networkstringtable = (INetworkStringTableContainer *)engineFactory(INTERFACENAME_NETWORKSTRINGTABLESERVER, NULL);

	gameents = (IServerGameEnts *)gameServerFactory(INTERFACEVERSION_SERVERGAMEENTS, NULL);
	gamedll = (IServerGameDLL *)gameServerFactory(INTERFACEVERSION_SERVERGAMEDLLGMOD, NULL);

	g_pLua = gLua;

	DWORD* pFunct = GetInterfaceFuncPtr((DWORD*)gameents,"%p",&IServerGameEnts::CheckTransmit);
	pFunct = (DWORD*)pFunct[0];

	CDetour::CheckTrampoline = *(checktransmit_t)&pFunct;

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	DetourAttach(&(PVOID&)CDetour::CheckTrampoline,
		(PVOID)(&(PVOID&)CDetour::CheckTransmit));

	DetourTransactionCommit();

	CSigScan::sigscan_dllfunc = (CreateInterfaceFn)gameServerFactory(INTERFACEVERSION_PLAYERINFOMANAGER,NULL);
	bool scan = CSigScan::GetDllMemInfo();

	LEVELSHUTDOWN_Sig.Init((unsigned char *) LEVELSHUTDOWN, LEVELSHUTDOWNMASK, LEVELSHUTDOWNLEN);
	unsigned char *addr = (unsigned char *)LEVELSHUTDOWN_Sig.sig_addr;

	if(addr == NULL)
	{
		gLua->Error("NO ENTITY LIST ERROR");
		return 0;
	}

	g_pEntityList = *((CGlobalEntityList **)(addr + ENTLISTOFFSET));

	INetworkStringTable *table = networkstringtable->FindTable("LuaStrings");

	// we need to scan for the LuaStrings table..
	if(table == NULL)
	{
		gLua->RunString("gm_transmittools", "", "umsg.PoolString(\"N\")", true, true);

		for(int i = 0; i < networkstringtable->GetNumTables(); i++)
		{
			INetworkStringTable *table_iter = networkstringtable->GetTable(i);

			for(int x = 0; x < table_iter->GetNumStrings(); x++)
			{
				const char *string_iter = table_iter->GetString(x);
				if( string_iter != NULL && strlen(string_iter) == 1 && strcmp(string_iter, "N") == 0 )
				{
					table = table_iter;
					break;
				}
			}
		}
	}

	if(table == NULL)
	{
		gLua->Error("Could not locate LuaStrings stringtable");
	}

	// umsg pool
	umsgStringTableOffset = table->AddString(true, "N") << 1 | 1; // what teh hug

	ILuaObject *transmittools = gLua->GetNewTable();
		transmittools->SetMember("DebugDump", DebugDump);
		transmittools->SetMember("DebugVis", DebugVis);
		transmittools->SetMember("DebugIncompleteSize", DebugIncompleteSize);
		transmittools->SetMember("DebugMasks", DebugMasks);

		transmittools->SetMember("NetworkedEntityCreated", tt_NetworkedEntityCreated);
		transmittools->SetMember("NetworkedEntityCreatedFinish", tt_NetworkEntityCreatedFinish);
		transmittools->SetMember("PlayerCreated", tt_PlayerCreated);
		transmittools->SetMember("EntityDestroyed", tt_EntityDestroyed);
		transmittools->SetMember("PlayerDestroyed", tt_PlayerDestroyed);
		transmittools->SetMember("AddValue", tt_AddValue);

		transmittools->SetMember("VariableUpdated", tt_VariableUpdated);
		transmittools->SetMember("EntityVariableUpdated", tt_EntityVariableUpdated);

		transmittools->SetMember("InvalidatePlayerCrashed", tt_InvalidatePlayerCrashed);
		transmittools->SetMember("PlayerTimeout", tt_PlayerTimeout);

		transmittools->SetMember("NWTick", NWTickAll);

		transmittools->SetMember("NWTest", NWUmsgTest);
	gLua->SetGlobal("transmittools", transmittools);
	transmittools->UnReference();

	return 0;
}

int Close(lua_State *L)
{
	ILuaInterface *gLua = Lua();

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	DetourDetach(&(PVOID&)CDetour::CheckTrampoline,
		(PVOID)(&(PVOID&)CDetour::CheckTransmit));

	DetourTransactionCommit();

	return 0;
}