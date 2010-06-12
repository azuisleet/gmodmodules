// GateKeeper V4
// ComWalk 5/11/10

#ifdef WIN32
	#define VTABLE_OFFSET 0

	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include "sigscan.h"
#else
	#define VTABLE_OFFSET 1

	#include <dlfcn.h>
	#include <sys/mman.h>
	#include <stdlib.h>
	#include <unistd.h>
#endif

#include "vfnhook.h"

#include <steam/steamclientpublic.h>
#include <interface.h>
#include <netadr.h>
#include <iclient.h>
#include <iserver.h>
#include <inetchannel.h>

#include "common/GMLuaModule.h"

GMOD_MODULE(Load, Unload)

typedef void unknown_ret;

// None of these are used vOv
class CBaseClient;
class CClientFrame;
class CFrameSnapshot;
class bf_write;

class CBaseServer : public IServer {
public:
	virtual unknown_ret GetCPUUsage() = 0;
	virtual unknown_ret BroadcastPrintf( char const*, ... ) = 0;
	virtual unknown_ret SetMaxClients( int ) = 0;
	virtual unknown_ret WriteDeltaEntities( CBaseClient*, CClientFrame*, CClientFrame*, bf_write& ) = 0;
	virtual unknown_ret WriteTempEntities( CBaseClient*, CFrameSnapshot*, CFrameSnapshot*, bf_write&, int ) = 0;
	virtual unknown_ret Init( bool ) = 0;
	virtual unknown_ret Clear() = 0;
	virtual unknown_ret Shutdown() = 0;
	virtual unknown_ret CreateFakeClient( char const* ) = 0;
	virtual unknown_ret RemoveClientFromGame( CBaseClient* ) = 0;
	virtual unknown_ret SendClientMessages( bool ) = 0;
	virtual unknown_ret FillServerInfo( SVC_ServerInfo& ) = 0;
	virtual unknown_ret UserInfoChanged( int ) = 0;
	virtual unknown_ret RejectConnection( netadr_s const&, char*, ... ) = 0;
	virtual bool CheckIPRestrictions( netadr_s const&, int ) = 0;
	virtual void* ConnectClient( netadr_s&, int, int, int, char const*, char const*, char const*, int ) = 0;
	virtual unknown_ret GetFreeClient( netadr_s& ) = 0;
	virtual unknown_ret CreateNewClient( int ) = 0;
	virtual unknown_ret FinishCertificateCheck( netadr_s&, int, char const* ) = 0;
	virtual int GetChallengeNr( netadr_s& ) = 0;
	virtual int GetChallengeType( netadr_s& ) = 0;
	virtual bool CheckProtocol( netadr_s&, int ) = 0;
	virtual bool CheckChallengeNr( netadr_s&, int ) = 0;
	virtual bool CheckChallengeType( CBaseClient*, int, netadr_s&, int, char const*, int ) = 0;
	virtual bool CheckPassword( netadr_s&, char const*, char const* ) = 0;
	virtual bool CheckIPConnectionReuse( netadr_s& ) = 0;
	virtual unknown_ret ReplyChallenge( netadr_s& ) = 0;
	virtual unknown_ret ReplyServerChallenge( netadr_s& ) = 0;
	virtual unknown_ret CalculateCPUUsage() = 0;
	virtual bool ShouldUpdateMasterServer() = 0;
};

struct SteamCert
{
	char Unknown1[16];
	CSteamID id;
};

CBaseServer* pServer = NULL;
ILuaInterface* gLua = NULL;
SteamCert* steamCert = NULL;

DEFVFUNC_(origConnectClient, void, (CBaseServer* srv,
		netadr_t& netinfo, int unk1, int unk2, int unk3, char const* user, char const* pass, char const* cert, int unk4));
void VFUNC newConnectClient(CBaseServer* srv,
		netadr_t& netinfo, int unk1, int unk2, int unk3, char const* user, char const* pass, char const* cert, int unk4)
{
	steamCert = (SteamCert*) cert;

	return origConnectClient(srv, netinfo, unk1, unk2, unk3, user, pass, cert, unk4);
}

DEFVFUNC_(origCheckPassword, bool, (CBaseServer* srv, netadr_s& adr, char const* pass, char const* user));
bool VFUNC newCheckPassword(CBaseServer* srv, netadr_t& netinfo, const char* pass, const char* user)
{
	char* steamid = "STEAM_ID_UNKNOWN";

	// This should never be NULL, but if it is it means it was unable
	// to find the call to CBaseServer::ConnectClient on the stack.
	if ( steamCert != NULL )
		steamid = steamCert->id.Render();

	gLua->Push(gLua->GetGlobal("hook")->GetMember("Call"));
		gLua->Push("PlayerPasswordAuth");
		gLua->PushNil(); // Gamemode (Unnecessary, always nil)
		gLua->Push(user);
		gLua->Push(pass);
		gLua->Push(steamid);
		gLua->Push(netinfo.ToString());
	gLua->Call(6, 1);

	ILuaObject* ret = gLua->GetReturn(0);

	if ( ret->GetType() == GLua::TYPE_BOOL )
	{
		return ret->GetBool();
	}
	else if ( ret->GetType() == GLua::TYPE_STRING )
	{
		srv->RejectConnection(netinfo, "%s", ret->GetString());
		return false;
	}
	else if ( ret->GetType() == GLua::TYPE_TABLE )
	{
		ILuaObject* allow = ret->GetMember(1);

		if ( allow && allow->GetType() == GLua::TYPE_BOOL )
		{
			if ( allow->GetBool() == true )
			{
				return true;
			}
			else
			{				
				ILuaObject* reason = ret->GetMember(2);

				if ( reason != NULL )
				{
					if ( reason->GetType() == GLua::TYPE_STRING )
					{
						srv->RejectConnection(netinfo, "%s", reason->GetString());
					}
					else if ( !reason->isNil() )
					{
						gLua->ErrorNoHalt("Second return value of PlayerPasswordAuth must be nil or a string!\n");
					}
				}

				return false;
			}
		}
	}

	if ( !ret->isNil() )
		gLua->ErrorNoHalt("PlayerPasswordAuth hook must return a boolean, string, or table value!\n");

	return origCheckPassword(srv, netinfo, pass, user);
}

LUA_FUNCTION(GetUserByAddress)
{
	if ( !pServer )
		gLua->Error("Gatekeeper: pServer is NULL!");

	gLua->CheckType(1, GLua::TYPE_STRING);
	const char *addr = gLua->GetString(1);

	for (int i=0; i < pServer->GetClientCount(); i++)
	{
		IClient* client = pServer->GetClient(i);
		if(client->IsConnected() && strcmp(addr, client->GetNetChannel()->GetRemoteAddress().ToString()) == 0)
		{	
			gLua->Push((float) client->GetUserID());
			return 1;
		}
	}

	return 0;
}

LUA_FUNCTION(DropAllPlayers)
{
	if ( !pServer )
		gLua->Error("Gatekeeper: pServer is NULL!");

	gLua->CheckType(1, GLua::TYPE_STRING);

	for (int i=0; i < pServer->GetClientCount(); i++)
	{
		IClient* client = pServer->GetClient(i);

		if(client->IsConnected())
			client->Disconnect(gLua->GetString(1));
	}

	return 0;
}

LUA_FUNCTION(DropPlayer)
{
	if ( !pServer )
		gLua->Error("Gatekeeper: pServer is NULL!");

	gLua->CheckType(1, GLua::TYPE_NUMBER);
	gLua->CheckType(2, GLua::TYPE_STRING);

	int DropID = gLua->GetNumber(1);

	for (int i=0; i < pServer->GetClientCount(); i++)
	{
		IClient* client = pServer->GetClient(i);
		
		if ( client->GetUserID() == DropID )
		{
			client->Disconnect(gLua->GetString(2));
			gLua->Push(true);
			return 1;
		}
	}

	gLua->Push(false);
	return 1;
}

LUA_FUNCTION(GetNumClients)
{
	if ( !pServer )
		gLua->Error("Gatekeeper: pServer is NULL!");

	int spawning = 0;
	int active = 0;
	int total = 0;

	for (int i=0; i < pServer->GetNumClients(); i++)
	{
		IClient* client = pServer->GetClient(i);
		
		if ( client->IsConnected() )
		{
			total++;

			if ( client->IsActive() )
				active++;
			else
				spawning++;
		}
	}

	ILuaObject* ret = gLua->GetNewTable();
		ret->SetMember("active", (float) active);
		ret->SetMember("spawning", (float) spawning);
		ret->SetMember("total", (float) total);
	gLua->Push(ret);

	ret->UnReference();

	return 1;
}

char* CSteamID::Render() const
{
	static char szSteamID[64];
	
	// Pirated clients (or legit clients with bad steamid tickets) seem to report
	// values for account type and universe that are totally invalid and an account
	// instance that is not equal to one. This should catch all of that.

	// Add in event of further errors: || this->GetEAccountType() >= k_EAccountTypeMax || this->GetEUniverse() >= k_EUniverseMax 
	if ( !this->IsValid() || this->GetUnAccountInstance() != 1)
		Q_snprintf(szSteamID, sizeof(szSteamID), "STEAM_ID_UNKNOWN");
	else if ( this->GetEAccountType() == k_EAccountTypePending )
		Q_snprintf(szSteamID, sizeof(szSteamID), "STEAM_ID_PENDING");
	else
		Q_snprintf(szSteamID, sizeof(szSteamID), "STEAM_0:%u:%u", (m_unAccountID % 2) ? 1 : 0, m_unAccountID/2);

	return szSteamID;
}

#ifndef WIN32
	unsigned char runFrameOrig[10];
	unsigned char* runFrame;

	void tempRunFrame(CBaseServer* srv)
	{
		// Restore member function back to how it should be
		memcpy(runFrame, runFrameOrig, sizeof(runFrameOrig));

		long pagesize = sysconf(_SC_PAGESIZE);
		mprotect(runFrame - ((unsigned long) runFrame % pagesize), pagesize, PROT_READ | PROT_EXEC);

		// All that trouble for this pointer.
		pServer = srv;

		// Apply the hooks now that we have the pointer.
		HOOKVFUNC(pServer, (57 + VTABLE_OFFSET), origCheckPassword, newCheckPassword);
		HOOKVFUNC(pServer, (48 + VTABLE_OFFSET), origConnectClient, newConnectClient);

		// Now that we're done, call the original.
		// Now featuring ugly pointer voodoo.
		((void (*)(CBaseServer*))(runFrame))(srv);
	}
#endif

int Load(lua_State* L)
{
	gLua = Lua();

#ifdef WIN32
	CSigScan::sigscan_dllfunc = Sys_GetFactory("engine.dll");
	
	if ( !CSigScan::GetDllMemInfo() )
		gLua->Error("CSigScan::GetDllMemInfo failed!");

	CSigScan sigBaseServer;
	sigBaseServer.Init((unsigned char *)
		"\x00\x00\x00\x00\xE8\x2C\xFA\xFF\xFF\x5E"
		"\xC3\x8B\x0D\x00\x00\x00\x00\x51\xB9",
		"????xxxxxxxxx????xx", 10);

	if ( !sigBaseServer.is_set )
		gLua->Error("CBaseServer signature failed!");

	pServer = *(CBaseServer**) sigBaseServer.sig_addr;

	HOOKVFUNC(pServer, (57 + VTABLE_OFFSET), origCheckPassword, newCheckPassword);
	HOOKVFUNC(pServer, (48 + VTABLE_OFFSET), origConnectClient, newConnectClient);
#else
	// So, linux. Here we are. There's a right way and a wrong way to do this.
	// This is the wrong way. (It still works).
	// The problem we have is that while acquiring a pointer to CBaseServer* is
	// relatively easy on windows, because of the code that gcc tends to generate
	// it's very hard with linux. So, rather than acquiring a pointer to CBaseServer*,
	// I've elected to detour a member function, grab the 'this' pointer from it, and
	// then restore the function back to normal and continue operation just as it would
	// with the windows binaries.

	void* engine = dlopen("engine_i486.so", RTLD_LAZY | RTLD_NOLOAD);
	runFrame = dlsym(engine, "_ZN11CBaseServer8RunFrameEv");

	if ( !runFrame )
		gLua->Error("Gatekeeper: CBaseServer::RunFrame symbol not found!");

	// The address needs to be aligned to a memory page. This is ugly. Oh well.
	long pagesize = sysconf(_SC_PAGESIZE);
	if ( int err = mprotect(runFrame - ((unsigned long) runFrame % pagesize), pagesize, PROT_READ | PROT_WRITE | PROT_EXEC) )
		gLua->Error("Gatekeeper: Couldn't mprotect CBaseServer::RunFrame");

	// Back up the original bytes so we can restore them later
	memcpy(runFrameOrig, runFrame, sizeof(runFrameOrig));

	// Manual detour! (ugh)
	// mov eax, tempRunFrame;
	// jmp eax;
	runFrame[0] = 0xB8;
	*(unsigned char**)(runFrame + 1) = (unsigned char*) tempRunFrame;
	runFrame[5] = 0xFF;
	runFrame[6] = 0xE0;

	dlclose(engine);
#endif
	
	ILuaObject* gatekeeper = gLua->GetNewTable();
		gatekeeper->SetMember("Drop", DropPlayer);
		gatekeeper->SetMember("GetNumClients", GetNumClients);
		gatekeeper->SetMember("DropAllClients", DropAllPlayers);
		gatekeeper->SetMember("GetUserByAddress", GetUserByAddress);

	gLua->SetGlobal("gatekeeper", gatekeeper);
	gatekeeper->UnReference();

	return 0;
}

int Unload(lua_State* L)
{
	if ( pServer )
	{
		UNHOOKVFUNC(pServer, (57 + VTABLE_OFFSET), origCheckPassword);
		UNHOOKVFUNC(pServer, (48 + VTABLE_OFFSET), origConnectClient);
	}

	return 0;
}
