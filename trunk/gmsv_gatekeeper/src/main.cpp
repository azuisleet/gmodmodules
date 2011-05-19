// GateKeeper V4.2
// ComWalk, VoiDeD, chrisaster 6/29/10

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

	#include "sourcemod/memutils.h"
#endif

#include "vfnhook.h"

#include <interface.h>
#include <netadr.h>
#include <inetmsghandler.h>
#include <inetchannel.h>
#include <steamclientpublic.h>

#include "tmpserver.h"
#include "tmpclient.h"

#include "common/GMLuaModule.h"

GMOD_MODULE(Load, Unload);

CBaseServer* pServer = NULL;
ILuaInterface* gLua = NULL;

static uint64 rawSteamID = 0;
static int clientChallenge = 0;

DEFVFUNC_(origConnectClient, void, (CBaseServer* srv,
		netadr_t &netinfo, int netProt, int chal, int authProt, int challenge, const char* user, const char *pass, const char* cert, int certLen));
void VFUNC newConnectClient(CBaseServer* srv,
		netadr_t &netinfo, int netProt, int chal, int clientchal, int authProt, const char* user, const char *pass, const char* cert, int certLen)
{
	clientChallenge = clientchal;

	if ( netProt == 15 || netProt == 16 )
	{
		uint32 headerLen = *((uint32*)cert);

		if ( headerLen <= 20 )
			rawSteamID = *(uint64*)(cert + 4 + headerLen + 12);
		else
			rawSteamID = 0;
	}
	else if ( netProt == 14 )
		rawSteamID = *(uint64*)(cert + 16);
	else
		rawSteamID = 0;

	return origConnectClient(srv, netinfo, netProt, chal, clientchal, authProt, user, pass, cert, certLen);
}

DEFVFUNC_(origCheckPassword, bool, (CBaseServer* srv, netadr_s& adr, char const* pass, char const* user));
bool VFUNC newCheckPassword(CBaseServer* srv, netadr_t& netinfo, const char* pass, const char* user)
{
	const char* steamid = "STEAM_ID_UNKNOWN";
	CSteamID steam(rawSteamID);

	// This should never be NULL, but if it is it means it was unable
	// to find the call to CBaseServer::ConnectClient on the stack.
	if ( steam.BIndividualAccount() )
		steamid = steam.Render();

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
		srv->RejectConnection( netinfo, clientChallenge, ret->GetString() );
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
						srv->RejectConnection( netinfo, clientChallenge, reason->GetString() );
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

	for (int i=0; i < pServer->GetClientCount(); i++)
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

#ifndef WIN32
	unsigned char runFrameOrig[10];
	unsigned char* runFrame;

	void tempRunFrame(CBaseServer* srv)
	{
		// Restore member function back to how it should be
		memcpy(runFrame, runFrameOrig, sizeof(runFrameOrig));
		ReProtect( runFrame );

		// All that trouble for this pointer.
		pServer = srv;
 
		// Apply the hooks now that we have the pointer.
		HOOKVFUNC(pServer, (58 + VTABLE_OFFSET), origCheckPassword, newCheckPassword);
		HOOKVFUNC(pServer, (49 + VTABLE_OFFSET), origConnectClient, newConnectClient);

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

	HOOKVFUNC(pServer, (58 + VTABLE_OFFSET), origCheckPassword, newCheckPassword);
	HOOKVFUNC(pServer, (49 + VTABLE_OFFSET), origConnectClient, newConnectClient);
#else
	// So, linux. Here we are. There's a right way and a wrong way to do this.
	// This is the wrong way. (It still works).
	// The problem we have is that while acquiring a pointer to CBaseServer* is
	// relatively easy on windows, because of the code that gcc tends to generate
	// it's very hard with linux. So, rather than acquiring a pointer to CBaseServer*,
	// I've elected to detour a member function, grab the 'this' pointer from it, and
	// then restore the function back to normal and continue operation just as it would
	// with the windows binaries.

	void *hEngine = dlopen("engine.so", RTLD_NOW);
	
	if ( !hEngine )
		gLua->Error("Gatekeeper: dlopen() failed!");

	runFrame = ResolveSymbol(hEngine, "_ZN11CBaseServer8RunFrameEv");

	dlclose(hEngine);

	if ( !runFrame )
		gLua->Error("Gatekeeper: CBaseServer::RunFrame signature failed!");

	// The address needs to be aligned to a memory page. This is ugly. Oh well.
	if ( DeProtect( runFrame ) )
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
		UNHOOKVFUNC(pServer, (58 + VTABLE_OFFSET), origCheckPassword);
		UNHOOKVFUNC(pServer, (49 + VTABLE_OFFSET), origConnectClient);
	}

	return 0;
}
