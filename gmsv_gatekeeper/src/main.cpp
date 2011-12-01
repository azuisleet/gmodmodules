// GateKeeper V4.2
// ComWalk, VoiDeD, Chrisaster

#ifdef WIN32
	#define VTABLE_OFFSET 0
	#define ENGINE_LIB "engine.dll"

	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>

	#include "sigscan.h"
#else if defined _LINUX
	#define VTABLE_OFFSET 1
	#define ENGINE_LIB "engine.so"

	#include <dlfcn.h>
	#include <sys/mman.h>
	#include <stdlib.h>
	#include <unistd.h>

	#include "sourcemod/memutils.h"
#endif

#include "vfnhook.h"

#include <tier1/tier1.h>
#include <tier1/convar.h>

#include <interface.h>
#include <netadr.h>
#include <inetmsghandler.h>
#include <inetchannel.h>
#include <steamclientpublic.h>
#include <vstdlib/cvar.h>

#include "tmpserver.h"
#include "tmpclient.h"

#include "common/GMLuaModule.h"

GMOD_MODULE( Load, Unload );

static ConVar gk_force_protocol_enable( "gk_force_protocol_enable", "0", FCVAR_NONE, "Enable or disable gatekeeper handling of a specific network protocol." );
static ConVar gk_force_protocol( "gk_force_protocol", "0", FCVAR_NONE, "Force gatekeeper to handle a specific protocol." );
static ConVar gk_debug( "gk_debug", "0", FCVAR_NONE, "Enables additional debug output." );

static uint64 rawSteamID = 0;
static int clientChallenge = 0;

CBaseServer* pServer = NULL;
ILuaInterface* gLua = NULL;

DEFVFUNC_( origConnectClient, void, ( CBaseServer *srv, netadr_t &netinfo, int netProt, int chal, int authProt, int challenge, const char *user, const char *pass, const char *cert, int certLen ) );
void VFUNC newConnectClient( CBaseServer *srv, netadr_t &netinfo, int netProt, int chal, int clientchal, int authProt, const char *user, const char *pass, const char *cert, int certLen )
{
	clientChallenge = clientchal;

	bool debugOutputEnabled = gk_debug.GetBool();

	if ( debugOutputEnabled )
	{
		// print network and authentication protocol versions
		gLua->Msg( "[GateKeeper] ConnectClient( netProt: %d, authProt: %d )\n", netProt, authProt );

		// print client certificate
		gLua->Msg( "[GateKeeper] Printing client (%s) certificate..\n", user );
		for ( int i = 0; i < certLen; i++ )
			gLua->Msg( "%02X ", (unsigned char)cert[i] );
		gLua->Msg( "\n" );
	}

	int origNetProt = netProt;

	if ( gk_force_protocol_enable.GetBool() )
	{
		// force a specific auth pathway
		netProt = gk_force_protocol.GetInt();

		if ( debugOutputEnabled )
			gLua->Msg( "[GateKeeper] Forcing network protocol to %d!\n", netProt );
	}

	switch ( netProt )
	{
		case 14:
		{
			rawSteamID = *(uint64*)( cert + 16 );

			break;
		}
		case 15:
		case 16:
		{
			uint32 headerLen = *((uint32*)cert);

			if ( headerLen <= 20 )
				rawSteamID = *(uint64*)(cert + 4 + headerLen + 12);
			else
				rawSteamID = 0;

			break;
		}
		case 17:
		case 18:
		case 19:
		case 20:
		case 21:
		{
			// steamid is always at the beginning, and is reliable
			rawSteamID = *(uint64 *)cert;
			
			break;
		}
		default:
		{

			rawSteamID = 0;

			break;
		}
	}

	if ( debugOutputEnabled )
		gLua->Msg( "[GateKeeper] Certificate SteamID: %llu\n", rawSteamID );

	return origConnectClient( srv, netinfo, origNetProt, chal, clientchal, authProt, user, pass, cert, certLen );
}

DEFVFUNC_( origCheckPassword, bool, ( CBaseServer *srv, netadr_s &adr, const char *pass, const char *user ) );
bool VFUNC newCheckPassword( CBaseServer *srv, netadr_t &netinfo, const char *pass, const char *user )
{
	const char *steamid = "STEAM_ID_UNKNOWN";
	
	CSteamID steam( rawSteamID );

	if ( steam.BIndividualAccount() )
		steamid = steam.Render();
	
	ILuaObject *hookTable = gLua->GetGlobal( "hook" );
	ILuaObject *hookCallFunc = hookTable->GetMember( "Call" );
	
	// hook.Call
	gLua->Push( hookCallFunc );
	// hook name
	gLua->Push( "PlayerPasswordAuth" );
	// gamemode
	gLua->PushNil();
	// hook arguments
	gLua->Push( user );
	gLua->Push( pass );
	gLua->Push( steamid );
	gLua->Push( netinfo.ToString() );
	// call hook
	gLua->Call( 6, 1 );
	// cleanup
	hookTable->UnReference();
	hookCallFunc->UnReference();

	ILuaObject *ret = gLua->GetReturn(0);

	if ( ret->GetType() == GLua::TYPE_BOOL )
	{
		bool b = ret->GetBool();
		
		ret->UnReference();

		return b;
	}
	else if ( ret->GetType() == GLua::TYPE_STRING )
	{
		srv->RejectConnection( netinfo, clientChallenge, ret->GetString() );
		
		ret->UnReference();

		return false;
	}
	else if ( ret->GetType() == GLua::TYPE_TABLE )
	{
		ILuaObject *allow = ret->GetMember( 1 );
		ILuaObject *reason = ret->GetMember( 2 );
		
		ret->UnReference();

		if ( allow && allow->GetType() == GLua::TYPE_BOOL )
		{
			if ( allow->GetBool() == true )
			{
				return true;
			}
			else
			{				
				if ( reason != NULL )
				{
					if ( reason->GetType() == GLua::TYPE_STRING )
					{
						srv->RejectConnection( netinfo, clientChallenge, reason->GetString() );
					}
					else if ( !reason->isNil() )
					{
						gLua->ErrorNoHalt( "Second return value of PlayerPasswordAuth must be nil or a string!\n" );
					}
				}

				return false;
			}
		}
	}
	else
	{
		if ( !ret->isNil() )
			gLua->ErrorNoHalt( "PlayerPasswordAuth hook must return a boolean, string, or table value!\n" );
		
		ret->UnReference();
	}

	return origCheckPassword( srv, netinfo, pass, user );
}

LUA_FUNCTION( ForceProtocol )
{
	if ( gLua->GetType( 1 ) == GLua::TYPE_NIL )
	{
		gk_force_protocol_enable.SetValue( false );
		gk_force_protocol.Revert();

		return 0;
	}

	gLua->CheckType( 1, GLua::TYPE_NUMBER );

	int forceProt = gLua->GetNumber( 1 );

	gk_force_protocol_enable.SetValue( true );
	gk_force_protocol.SetValue( forceProt );

	return 0;
}


LUA_FUNCTION( GetUserByAddress )
{
	gLua->CheckType( 1, GLua::TYPE_STRING );
	
	const char *pszAddress = gLua->GetString( 1 );

	for (int i=0; i < pServer->GetClientCount(); i++)
	{
		IClient *client = pServer->GetClient( i );

		if( client->IsConnected() && strcmp( pszAddress, client->GetNetChannel()->GetRemoteAddress().ToString() ) == 0 )
		{	
			gLua->Push( (float)client->GetUserID() );
	
			return 1;
		}
	}

	return 0;
}


LUA_FUNCTION( DropAllPlayers )
{
	gLua->CheckType( 1, GLua::TYPE_STRING );

	const char *pszReason = gLua->GetString( 1 );

	for (int i=0; i < pServer->GetClientCount(); i++)
	{
		IClient *client = pServer->GetClient( i );

		if( client->IsConnected() )
			client->Disconnect( pszReason );
	}

	return 0;
}

LUA_FUNCTION( DropPlayer )
{
	gLua->CheckType( 1, GLua::TYPE_NUMBER );
	gLua->CheckType( 2, GLua::TYPE_STRING );

	int iDropID = gLua->GetNumber( 1 );

	for (int i=0; i < pServer->GetClientCount(); i++)
	{
		IClient *client = pServer->GetClient(i);
	
		if ( client->GetUserID() == iDropID )
		{
			client->Disconnect( gLua->GetString( 2 ) );
			
			gLua->Push( true );

			return 1;
		}
	}

	gLua->Push( false );

	return 1;
}

LUA_FUNCTION( GetNumClients )
{
	int spawning = 0;
	int active = 0;
	int total = 0;

	for (int i=0; i < pServer->GetClientCount(); i++)
	{
		IClient *client = pServer->GetClient( i );
		
		if ( client->IsConnected() )
		{
			total++;

			if ( client->IsActive() )
				active++;
			else
				spawning++;
		}
	}

	// create table
	ILuaObject* ret = gLua->GetNewTable();
	// set members
	ret->SetMember( "active", (float)active );
	ret->SetMember( "spawning", (float)spawning );
	ret->SetMember( "total", (float)total );
	// push to stack
	gLua->Push( ret );
	// cleanup
	ret->UnReference();

	return 1;
}

int Load( lua_State *L )
{
	gLua = Lua();

#ifdef WIN32
	CSigScan::sigscan_dllfunc = Sys_GetFactory( ENGINE_LIB );
	
	if ( CSigScan::GetDllMemInfo() )
	{
		CSigScan sigBaseServer;
		sigBaseServer.Init( (unsigned char *)
			"\x00\x00\x00\x00\xE8\x2C\xFA\xFF\xFF\x5E"
			"\xC3\x8B\x0D\x00\x00\x00\x00\x51\xB9",
			"????x????xxxx????xx", 19 );

		if ( sigBaseServer.is_set )
			pServer = *(CBaseServer **)sigBaseServer.sig_addr;
	}
#else
	void *hEngine = dlopen( ENGINE_LIB, RTLD_LAZY );

	if ( hEngine )
	{
		pServer = (CBaseServer *)ResolveSymbol( hEngine, "sv" );

		dlclose( hEngine );
	}
#endif

	if ( !pServer )
	{
		gLua->Error( "Gatekeeper: failed to initialize (pServer is NULL)" );
		
		return 0;
	}

	HOOKVFUNC( pServer, (58 + VTABLE_OFFSET), origCheckPassword, newCheckPassword );
	HOOKVFUNC( pServer, (49 + VTABLE_OFFSET), origConnectClient, newConnectClient );

	// create table
	ILuaObject *gatekeeper = gLua->GetNewTable();
	// set members
	gatekeeper->SetMember( "Drop", DropPlayer );
	gatekeeper->SetMember( "GetNumClients", GetNumClients );
	gatekeeper->SetMember( "DropAllClients", DropAllPlayers );
	gatekeeper->SetMember( "GetUserByAddress", GetUserByAddress );
	gatekeeper->SetMember( "ForceProtocol", ForceProtocol );
	// register as global
	gLua->SetGlobal( "gatekeeper", gatekeeper );
	// cleanup
	gatekeeper->UnReference();

	CreateInterfaceFn tier1Factory = VStdLib_GetICVarFactory();

	g_pCVar = (ICvar *)tier1Factory( CVAR_INTERFACE_VERSION, NULL );

	ConVar_Register();

	return 0;
}

int Unload( lua_State *L )
{
	if ( !pServer )
		return 0;

	ConVar_Unregister();

	UNHOOKVFUNC( pServer, (58 + VTABLE_OFFSET), origCheckPassword );
	UNHOOKVFUNC( pServer, (49 + VTABLE_OFFSET), origConnectClient );

	return 0;
}