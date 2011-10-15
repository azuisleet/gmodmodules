
#ifndef FURRYFINDER_H_
#define FURRYFINDER_H_

#ifdef _WIN32
#pragma once
#endif

#include "common/GMLuaModule.h"

#define VERSION_SAFE_STEAM_API_INTERFACES
#include "steam/steam_gameserver.h"

class CFurryFinder
{

public:
	CFurryFinder( lua_State *state ) :
		m_CallbackGroupStatus( this, &CFurryFinder::Steam_OnGSGroupStatus ),
		m_CallbackSteamConnected( this, &CFurryFinder::Steam_OnSteamConnected ),
		m_CallbackSteamDisconnected( this, &CFurryFinder::Steam_OnSteamDisconnected ),

		L( state ),
		bInited( false ) {}

	  ~CFurryFinder();

	  bool RequestGroupStatus( CSteamID steamUser, CSteamID steamGroup );

	  void Init();

	  ISteamUtils *GetUtils();

	  ILuaInterface *GetLua() { return Lua(); }

private:
	STEAM_GAMESERVER_CALLBACK( CFurryFinder, Steam_OnSteamConnected, SteamServersConnected_t, m_CallbackSteamConnected );
	STEAM_GAMESERVER_CALLBACK( CFurryFinder, Steam_OnSteamDisconnected, SteamServersDisconnected_t, m_CallbackSteamDisconnected );

	STEAM_GAMESERVER_CALLBACK( CFurryFinder, Steam_OnGSGroupStatus, GSClientGroupStatus_t, m_CallbackGroupStatus );

	lua_State *L;

	bool bInited;

	CSteamGameServerAPIContext gsContext;

};

extern CFurryFinder *g_pFurryFinder;


inline bool CSteamGameServerAPIContext::InitDetailed( CFurryFinder *pFurryFinder )
{
	ILuaInterface *lua = pFurryFinder->GetLua();

	if ( !g_pSteamClientGameServer )
	{
		lua->Msg( "FurryFinder: g_pSteamClientGameServer is NULL!\n" );
		return false;
	}

	HSteamUser hSteamUser = SteamGameServer_GetHSteamUser();
	HSteamPipe hSteamPipe = SteamGameServer_GetHSteamPipe();

	lua->Msg( "FurryFinder: User %d | Pipe %d\n", hSteamUser, hSteamPipe );


	m_pSteamGameServer = g_pSteamClientGameServer->GetISteamGameServer( hSteamUser, hSteamPipe, STEAMGAMESERVER_INTERFACE_VERSION );
	if ( !m_pSteamGameServer )
	{
		lua->Msg( "FurryFinder: m_pSteamGameServer is NULL!\n" );
		return false;
	}

	m_pSteamGameServerUtils = g_pSteamClientGameServer->GetISteamUtils( hSteamPipe, STEAMUTILS_INTERFACE_VERSION );
	if ( !m_pSteamGameServerUtils )
	{
		lua->Msg( "FurryFinder: m_pSteamGameServerUtils is NULL!\n" );
		return false;
	}

	return true;
}

#endif
