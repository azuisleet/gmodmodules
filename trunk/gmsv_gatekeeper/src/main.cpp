// GateKeeper V4.2
// ComWalk, VoiDeD 6/29/10

#ifdef WIN32
	#define VTABLE_OFFSET 0

	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#else
	#define VTABLE_OFFSET 1

	#include <dlfcn.h>
	#include <sys/mman.h>
	#include <stdlib.h>
	#include <unistd.h>
#endif

#include "sigscan.h"
#include "vfnhook.h"

#define NO_STEAM
#include "osw/Steamworks.h"

#include <interface.h>
#include <netadr.h>

#include <inetmsghandler.h>

#include <iserver.h>
#include <inetchannel.h>

#include "common/GMLuaModule.h"

GMOD_MODULE(Load, Unload)

typedef int USERID_t;

// This changed. Once the 2009 sdk comes out I'll go back to using iclient.h
class IClient : public INetChannelHandler
{
public:
	virtual					~IClient() {}
	virtual void			padding() = 0;
	virtual void			Connect(const char * szName, int nUserID, INetChannel *pNetChannel, bool bFakePlayer) = 0;
	virtual void			Inactivate( void ) = 0;
	virtual	void			Reconnect( void ) = 0;
	virtual void			Disconnect( const char *reason, ... ) = 0;
	virtual int				GetPlayerSlot() const = 0;
	virtual int				GetUserID() const = 0;
	virtual const USERID_t	GetNetworkID() const = 0;
	virtual const char		*GetClientName() const = 0;
	virtual INetChannel		*GetNetChannel() = 0;
	virtual IServer			*GetServer() = 0;
	virtual const char		*GetUserSetting(const char *cvar) const = 0;
	virtual const char		*GetNetworkIDString() const = 0;
	virtual void			SetRate( int nRate, bool bForce ) = 0;
	virtual int				GetRate( void ) const = 0;
	virtual void			SetUpdateRate( int nUpdateRate, bool bForce ) = 0;
	virtual int				GetUpdateRate( void ) const = 0;	
	virtual void			Clear( void ) = 0;
	virtual int				GetMaxAckTickCount() const = 0;
	virtual bool			ExecuteStringCommand( const char *s ) = 0;
	virtual bool			SendNetMsg(INetMessage &msg, bool bForceReliable = false) = 0;
	virtual void			ClientPrintf (const char *fmt, ...) = 0;
	virtual bool			IsConnected( void ) const = 0;
	virtual bool			IsSpawned( void ) const = 0;
	virtual bool			IsActive( void ) const = 0;
	virtual bool			IsFakeClient( void ) const = 0;
	virtual bool			IsHLTV( void ) const = 0;
	virtual bool			IsHearingClient(int index) const = 0;
	virtual bool			IsProximityHearingClient(int index) const = 0;
	virtual void			SetMaxRoutablePayloadSize( int nMaxRoutablePayloadSize ) = 0;
};



// None of these are used vOv
class CBaseClient;
class CClientFrame;
class CFrameSnapshot;
class bf_write;

//typedef void unknown_ret;

class CBaseServer : public IServer {
public:
	virtual unknown_ret unknown01() = 0;
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


class GSCallbacks
{
public:
	GSCallbacks() : m_CallbackClientApprove( this, &GSCallbacks::Steam_OnClientApprove ),
		m_CallbackClientDeny( this, &GSCallbacks::Steam_OnClientDeny ),
		m_CallbackClientKick( this, &GSCallbacks::Steam_OnClientKick ),
		m_CallbackSteamConnected( this, &GSCallbacks::Steam_OnConnect ),
		m_CallbackSteamDisconnected( this, &GSCallbacks::Steam_OnDisconnect ) {};

	STEAM_GAMESERVER_CALLBACK( GSCallbacks, Steam_OnClientDeny, GSClientDeny_t, m_CallbackClientDeny );
	STEAM_GAMESERVER_CALLBACK( GSCallbacks, Steam_OnClientApprove, GSClientApprove_t, m_CallbackClientApprove );
	STEAM_GAMESERVER_CALLBACK( GSCallbacks, Steam_OnClientKick, GSClientKick_t, m_CallbackClientKick );

	STEAM_GAMESERVER_CALLBACK( GSCallbacks, Steam_OnConnect, SteamServersConnected_t, m_CallbackSteamConnected );
	STEAM_GAMESERVER_CALLBACK( GSCallbacks, Steam_OnDisconnect, SteamServersDisconnected_t, m_CallbackSteamDisconnected );
};

GSCallbacks callbacks;

CBaseServer* pServer = NULL;
ILuaInterface* gLua = NULL;

uint64 rawSteamID = 0;

void GSCallbacks::Steam_OnClientApprove(GSClientApprove_t *gsclient)
{
	gLua->Push(gLua->GetGlobal("hook")->GetMember("Call"));
		gLua->Push("GSClientApprove");
		gLua->PushNil();
		gLua->Push( gsclient->m_SteamID.Render() );
	gLua->Call(3, 0);
};

void GSCallbacks::Steam_OnClientDeny(GSClientDeny_t *gsclient)
{
	gLua->Push(gLua->GetGlobal("hook")->GetMember("Call"));
		gLua->Push("GSClientDeny");
		gLua->PushNil();
		gLua->Push( gsclient->m_SteamID.Render() );
		gLua->Push( (float)gsclient->m_eDenyReason );
		gLua->Push( gsclient->m_pchOptionalText );
	gLua->Call(5, 0);
};

void GSCallbacks::Steam_OnClientKick(GSClientKick_t *gsclient)
{
	gLua->Push(gLua->GetGlobal("hook")->GetMember("Call"));
		gLua->Push("GSClientKick");
		gLua->PushNil();
		gLua->Push( gsclient->m_SteamID.Render() );
		gLua->Push( (float)gsclient->m_eDenyReason );
	gLua->Call(4, 0);
};

void GSCallbacks::Steam_OnConnect( SteamServersConnected_t *pParam )
{
	gLua->Push(gLua->GetGlobal("hook")->GetMember("Call"));
		gLua->Push("GSSteamConnected");
		gLua->PushNil();
	gLua->Call(2, 0);
}

void GSCallbacks::Steam_OnDisconnect( SteamServersDisconnected_t *pParam )
{
	gLua->Push(gLua->GetGlobal("hook")->GetMember("Call"));
		gLua->Push("GSSteamDisconnected");
		gLua->PushNil();
		gLua->Push( (float) pParam->m_eResult );
	gLua->Call(3, 0);
}

DEFVFUNC_(origConnectClient, void, (CBaseServer* srv,
		netadr_t &netinfo, int netProt, int chal, int authProt, const char* user, const char *pass, const char* cert, int certLen));
void VFUNC newConnectClient(CBaseServer* srv,
		netadr_t &netinfo, int netProt, int chal, int authProt, const char* user, const char *pass, const char* cert, int certLen)
{

	if ( netProt == 15 )
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

	return origConnectClient(srv, netinfo, netProt, chal, authProt, user, pass, cert, certLen);
}

DEFVFUNC_(origCheckPassword, bool, (CBaseServer* srv, netadr_s& adr, char const* pass, char const* user));
bool VFUNC newCheckPassword(CBaseServer* srv, netadr_t& netinfo, const char* pass, const char* user)
{
	char* steamid = "STEAM_ID_UNKNOWN";

	CSteamID steam = CSteamID(rawSteamID);

	// This should never be NULL, but if it is it means it was unable
	// to find the call to CBaseServer::ConnectClient on the stack.
	if ( steam.BIndividualAccount() )
		steamid = const_cast<char*>( steam.Render() );

	gLua->Push(gLua->GetGlobal("hook")->GetMember("Call"));
		gLua->Push("GSPlayerAuth");
		gLua->PushNil(); // Gamemode (Unnecessary, always nil)
		gLua->Push(user);
		gLua->Push(pass);
		gLua->Push(steamid);
		gLua->Push(netinfo.ToString());
	gLua->Call(6, 0);

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

		long pagesize = sysconf(_SC_PAGESIZE);
		mprotect(runFrame - ((unsigned long) runFrame % pagesize), pagesize, PROT_READ | PROT_EXEC);

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

	CSigScan::sigscan_dllfunc = Sys_GetFactory("engine.so");
	
	if ( !CSigScan::GetDllMemInfo() )
		gLua->Error("Gatekeeper: CSigScan::GetDllMemInfo failed!");

	CSigScan sigRunFrame;
	sigRunFrame.Init((unsigned char *)
		"\x55\x89\xE5\x57\x56\x53\x83\xEC"
		"\x1C\xE8\x00\x00\x00\x00\x81\xC3"
		"\x90\x97\x22\x00\x8B\x83\x60\xFB",
		"xxxxxxxxxx????xxxxxxxxxx", 24);

	if ( !sigRunFrame.is_set )
		gLua->Error("Gatekeeper: CBaseServer::RunFrame signature failed!");

	runFrame = (unsigned char *) sigRunFrame.sig_addr;

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
