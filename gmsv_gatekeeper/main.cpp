// GateKeeper V3
// ComWalk 8/1/09

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "vfnhook.h"
#include "sigscan.h"

#include <steam/steamclientpublic.h>
#include <interface.h>
#include <netadr.h>
#include <iclient.h>
#include <iserver.h>

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

DEFVFUNC_(origCheckPassword, bool, (CBaseServer* srv, netadr_s& adr, char const* pass, char const* user));
bool VFUNC newCheckPassword(CBaseServer* srv, netadr_t& netinfo, const char* pass, const char* user)
{
	// Ugly hack to get the steamcert passed to ConnectClient. Deal with it.
	// ConnectClient args seem to have changed across HL2/EP1/OB engines,
	// but not the order they are passed in, so avoiding another hook
	// is a Good Thing. (With ugly execution)
	SteamCert* cert = NULL;
	char** StackPtr;

	_asm { mov StackPtr, esp };
	for (int i = 0; i < 32; i++)
	{
		if ( StackPtr[i] == user && StackPtr[i + 1] == pass)
		{
			cert = (SteamCert *) (StackPtr[i + 2]);
			break;
		}
	}

	char* steamid = "STEAM_ID_UNKNOWN";

	// This should never be NULL, but if it is it means it was unable
	// to find the call to CBaseServer::ConnectClient on the stack.
	if ( cert != NULL )
		steamid = cert->id.Render();

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

LUA_FUNCTION(DropPlayer)
{
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

int Load(lua_State* L)
{
	gLua = Lua();

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
	HOOKVFUNC(pServer, 57, origCheckPassword, newCheckPassword);
	
	ILuaObject* gatekeeper = gLua->GetNewTable();
		gatekeeper->SetMember("Drop", DropPlayer);
		gatekeeper->SetMember("GetNumClients", GetNumClients);
	gLua->SetGlobal("gatekeeper", gatekeeper);

	gatekeeper->UnReference();

	return 0;
}

int Unload(lua_State* L)
{
	if ( pServer )
		UNHOOKVFUNC(pServer, 57, origCheckPassword);

	return 0;
}