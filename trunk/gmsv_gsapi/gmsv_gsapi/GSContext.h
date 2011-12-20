
#ifndef GSCONTEXT_H_
#define GSCONTEXT_H_

#ifdef _WIN32
#pragma once
#endif


#include "common/GMLuaModule.h"

#define VERSION_SAFE_STEAM_API_INTERFACES
#include "steam/steam_gameserver.h"

#include "tier1/utlvector.h"
#include "tier1/utlmap.h"


#define STEAM_GAMESERVER_CALLRESULT( class, func, param, var ) \
	CCallResult<class, param> var; void func( param *pParam, bool bIOFailure );

#define STEAM_GAMESERVER_MULTI_CALLRESULT( class, func, param, var ) \
	CUtlVector<CCallResult<class, param> *> var; void func( param *pParam, bool bIOFailure );


struct CallResult_t
{
	uint32 m_iCallback;
};

typedef CUtlMap<SteamAPICall_t, CallResult_t> CallResultMap;
typedef CallResultMap::IndexType_t CallResultIndex;


class CGSContext
{

public:
	CGSContext( lua_State *luaState ) :

		m_CallbackConnected( this, &CGSContext::Steam_OnConnect ),
		m_CallbackDisconnected( this, &CGSContext::Steam_OnDisconnect ),

		m_CallbackGameStats( this, &CGSContext::Steam_OnGameStats ),
		m_CallbackCallCompleted( this, &CGSContext::Steam_OnCallCompleted ),
		m_CallbackStatsUnloaded( this, &CGSContext::Steam_OnStatsUnloaded ),

		L( luaState ),
		m_bInited( false ),
		
		m_mapCallResults( DefLessFunc( uint64 ) ) {};

	~CGSContext();


	void Init();


	// gameserver functions
	bool IsLoggedOn();
	bool IsSecure();

	CSteamID GetSteamID();
	uint32 GetPublicIP();

	bool UpdateUserData( CSteamID steamID, const char *pchPlayerName, uint32 uScore );

	bool SetServerType( uint32 unServerFlags, uint32 unGameIP, uint16 unGamePort, uint16 unSpectatorPort,
						uint16 usQueryPort, const char *pchGameDir,  const char *pchVersion, bool bLANMode );

	void UpdateServerStatus( int cPlayers, int cPlayersMax, int cBotPlayers, const char *pchServerName,
							 const char *pSpectatorServerName, const char *pchMapName );

	void SetGameTags( const char *pchGameTags );

	bool GetServerRep();
	void GetGameStats();


	// gameserverstats functions
	bool RequestUserStats( CSteamID steamID );

	bool GetUserStat( CSteamID steamID, const char *pchName, int32 *pData );
	bool GetUserStat( CSteamID steamID, const char *pchName, float *pData );
	bool GetUserAchievement( CSteamID steamID, const char *pchName, bool *pbAchieved );



private:
	ILuaObject *TableFromCGameID( CGameID gameId );

private:
	// callbacks
	STEAM_GAMESERVER_CALLBACK( CGSContext, Steam_OnConnect, SteamServersConnected_t, m_CallbackConnected );
	STEAM_GAMESERVER_CALLBACK( CGSContext, Steam_OnDisconnect, SteamServersDisconnected_t, m_CallbackDisconnected );

	STEAM_GAMESERVER_CALLBACK( CGSContext, Steam_OnGameStats, GSGameplayStats_t, m_CallbackGameStats );
	STEAM_GAMESERVER_CALLBACK( CGSContext, Steam_OnCallCompleted, SteamAPICallCompleted_t, m_CallbackCallCompleted );
	STEAM_GAMESERVER_CALLBACK( CGSContext, Steam_OnStatsUnloaded, GSStatsUnloaded_t, m_CallbackStatsUnloaded );

	// for api calls
	CallResultMap m_mapCallResults;

private:
	void HandleGSReputation( GSReputation_t *pParam );
	void HandleGSStatsReceived( GSStatsReceived_t *pParam );
	void HandleGSStatsUnloaded( GSStatsUnloaded_t *pParam );


private:
	lua_State *L;

	CSteamGameServerAPIContext m_gsContext;
	bool m_bInited;

};

extern CGSContext *g_pGSContext;

// gameserver funcs
LUA_FUNCTION( IsLoggedOn );
LUA_FUNCTION( IsSecure );
LUA_FUNCTION( GetSteamID );
LUA_FUNCTION( GetPublicIP );
LUA_FUNCTION( UpdateUserData );
LUA_FUNCTION( SetServerType );
LUA_FUNCTION( UpdateServerStatus );
LUA_FUNCTION( SetGameTags );
LUA_FUNCTION( GetServerRep );
LUA_FUNCTION( GetGameStats );

// gameserverstats funcs
LUA_FUNCTION( RequestUserStats );
LUA_FUNCTION( GetUserStatInt );
LUA_FUNCTION( GetUserStatFloat );
LUA_FUNCTION( GetUserAchievement );

#endif
