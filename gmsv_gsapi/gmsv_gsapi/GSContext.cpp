
#include "GSContext.h"

#include "netadr.h"


#define HANDLE_CALLRESULT( type, apiCall ) \
	case type##_t::k_iCallback: \
	{ \
		type##_t type##param; \
		bool type##bFailed; \
		\
		if ( !steamUtils->GetAPICallResult( apiCall, &type##param, sizeof( type##_t ), type##_t::k_iCallback, &type##bFailed ) ) \
		{ \
			Lua()->Msg( "GSAPI: Unable to get API call result!\n" ); \
			return; \
		} \
		\
		if ( type##bFailed ) \
		{ \
			ESteamAPICallFailure callFailure = steamUtils->GetAPICallFailureReason( apiCall ); \
			Lua()->Msg( "GSAPI: API call failed: %d\n", callFailure ); \
			return; \
		} \
		this->Handle##type( & type##param ); \
	} \
	break;


CGSContext::~CGSContext()
{
	m_gsContext.Clear();
}


void CGSContext::Init()
{
	if ( m_bInited )
		return;

	m_bInited = m_gsContext.Init();
}

bool CGSContext::IsLoggedOn()
{
	if ( !m_bInited )
	{
		Lua()->Error( "GSAPI: GS context isn't initialized.\n" );
		return false;
	}

	return m_gsContext.SteamGameServer()->BLoggedOn();
}

bool CGSContext::IsSecure()
{
	if ( !m_bInited )
	{
		Lua()->Error( "GSAPI: GS context isn't initialized.\n" );
		return false;
	}

	return m_gsContext.SteamGameServer()->BSecure();
}

CSteamID CGSContext::GetSteamID()
{
	if ( !m_bInited )
	{
		Lua()->Error( "GSAPI: GS context isn't initialized.\n" );
		return k_steamIDNil;
	}

	return m_gsContext.SteamGameServer()->GetSteamID();
}

uint32 CGSContext::GetPublicIP()
{
	if ( !m_bInited )
	{
		Lua()->Error( "GSAPI: GS context isn't initialized.\n" );
		return 0;
	}

	return m_gsContext.SteamGameServer()->GetPublicIP();
}

bool CGSContext::UpdateUserData( CSteamID steamID, const char *pchPlayerName, uint32 uScore )
{
	if ( !m_bInited )
	{
		Lua()->Error( "GSAPI: GS context isn't initialized.\n" );
		return false;
	}

	return m_gsContext.SteamGameServer()->BUpdateUserData( steamID, pchPlayerName, uScore );
}

bool CGSContext::SetServerType( uint32 unServerFlags, uint32 unGameIP, uint16 unGamePort, uint16 unSpectatorPort, uint16 usQueryPort, const char *pchGameDir, const char *pchVersion, bool bLANMode )
{
	if ( !m_bInited )
	{
		Lua()->Error( "GSAPI: GS context isn't initialized.\n" );
		return false;
	}

	return m_gsContext.SteamGameServer()->BSetServerType( unServerFlags, unGameIP, unGamePort, unSpectatorPort, usQueryPort, pchGameDir, pchVersion, bLANMode );
}

void CGSContext::UpdateServerStatus( int cPlayers, int cPlayersMax, int cBotPlayers, const char *pchServerName, const char *pSpectatorServerName, const char *pchMapName )
{
	if ( !m_bInited )
	{
		Lua()->Error( "GSAPI: GS context isn't initialized.\n" );
		return;
	}

	m_gsContext.SteamGameServer()->UpdateServerStatus( cPlayers, cPlayersMax, cBotPlayers, pchServerName, pSpectatorServerName, pchMapName );
}

void CGSContext::SetGameTags( const char *pchGameTags )
{
	if ( !m_bInited )
	{
		Lua()->Error( "GSAPI: GS context isn't initialized.\n" );
		return;
	}

	m_gsContext.SteamGameServer()->SetGameTags( pchGameTags );
}

bool CGSContext::GetServerRep()
{
	if ( !m_bInited )
	{
		Lua()->Error( "GSAPI: GS context isn't initialized.\n" );
		return false;
	}

	SteamAPICall_t apiCall = m_gsContext.SteamGameServer()->GetServerReputation();

	if ( apiCall == k_uAPICallInvalid )
		return false;

	CallResult_t callResult;
	callResult.m_iCallback = GSReputation_t::k_iCallback;

	m_mapCallResults.Insert( apiCall, callResult );

	return true;
}

void CGSContext::GetGameStats()
{
	if ( !m_bInited )
	{
		Lua()->Error( "GSAPI: GS context isn't initialized.\n" );
		return;
	}

	m_gsContext.SteamGameServer()->GetGameplayStats();
}

// gameserverstats functions
bool CGSContext::RequestUserStats( CSteamID steamID )
{
	if ( !m_bInited )
	{
		Lua()->Error( "GSAPI: GS context isn't initialized.\n" );
		return false;
	}

	SteamAPICall_t apiCall = m_gsContext.SteamGameServerStats()->RequestUserStats( steamID );

	if ( apiCall == k_uAPICallInvalid )
		return false;

	CallResult_t callResult;
	callResult.m_iCallback = GSStatsReceived_t::k_iCallback;

	m_mapCallResults.Insert( apiCall, callResult );

	return true;
}

bool CGSContext::GetUserStat( CSteamID steamID, const char *pchName, int32 *pData )
{
	if ( !m_bInited )
	{
		Lua()->Error( "GSAPI: GS context isn't initialized.\n" );
		return false;
	}

	return m_gsContext.SteamGameServerStats()->GetUserStat( steamID, pchName, pData );
}

bool CGSContext::GetUserStat( CSteamID steamID, const char *pchName, float *pData )
{
	if ( !m_bInited )
	{
		Lua()->Error( "GSAPI: GS context isn't initialized.\n" );
		return false;
	}

	return m_gsContext.SteamGameServerStats()->GetUserStat( steamID, pchName, pData );
}

bool CGSContext::GetUserAchievement( CSteamID steamID, const char *pchName, bool *pbAchieved )
{
	if ( !m_bInited )
	{
		Lua()->Error( "GSAPI: GS context isn't initialized.\n" );
		return false;
	}

	return m_gsContext.SteamGameServerStats()->GetUserAchievement( steamID, pchName, pbAchieved );
}



ILuaObject *CGSContext::TableFromCGameID( CGameID gameId )
{
	ILuaObject *gameTable = Lua()->GetNewTable();

	gameTable->SetMember( "AppID", (float)gameId.AppID() );
	gameTable->SetMember( "ModID", (float)gameId.ModID() );
	gameTable->SetMember( "IsMod", gameId.IsMod() );
	gameTable->SetMember( "IsSteamApp", gameId.IsSteamApp() );
	gameTable->SetMember( "IsShortcut", gameId.IsShortcut() );
	gameTable->SetMember( "IsP2PFile", gameId.IsP2PFile() );
	gameTable->SetMember( "IsValid", gameId.IsValid() );

	return gameTable;
}



void CGSContext::HandleGSReputation( GSReputation_t *pParam )
{
	Lua()->Push( Lua()->GetGlobal( "hook" )->GetMember( "Call" ) );

	Lua()->Push( "GSReputation" );
	Lua()->PushNil(); // gamemode

	Lua()->Push( (float)pParam->m_eResult );
	Lua()->Push( (float)pParam->m_unReputationScore );
	Lua()->Push( pParam->m_bBanned );

	netadr_t netAddr( pParam->m_unBannedIP, pParam->m_usBannedPort );
	ILuaObject *gameTable = this->TableFromCGameID( CGameID( pParam->m_ulBannedGameID ) );

	ILuaObject *bannedTable = Lua()->GetNewTable();
	
	bannedTable->SetMember( "BannedAddress", netAddr.ToString() );
	bannedTable->SetMember( "BannedGame", gameTable );
	bannedTable->SetMember( "BanExpires", (float)pParam->m_unBanExpires );

	Lua()->Push( bannedTable );

	gameTable->UnReference();
	bannedTable->UnReference();

	Lua()->Call( 6, 0 );
}

			


void CGSContext::Steam_OnCallCompleted( SteamAPICallCompleted_t *pParam )
{
	if ( !m_bInited )
		return;
	
	SteamAPICall_t apiCall = pParam->m_hAsyncCall;

	ISteamUtils *steamUtils = m_gsContext.SteamGameServerUtils();

	if ( apiCall == k_uAPICallInvalid )
	{
		Lua()->Msg( "GSAPI: Got API call completion invalid call handle.\n" );
		return;
	}

	CallResultIndex index = m_mapCallResults.Find( apiCall );

	if ( index == m_mapCallResults.InvalidIndex() )
	{
		Lua()->Msg( "GSAPI: Got API call completion for unexpected call handle.\n" );
		return;
	}

	CallResult_t callResult = m_mapCallResults[ index ];

	m_mapCallResults.RemoveAt( index );

	switch ( callResult.m_iCallback )
	{
		HANDLE_CALLRESULT( GSReputation, apiCall );
		HANDLE_CALLRESULT( GSStatsReceived, apiCall );
		HANDLE_CALLRESULT( GSStatsUnloaded, apiCall );
	}

}

void CGSContext::Steam_OnGameStats( GSGameplayStats_t *pParam )
{
	Lua()->Push( Lua()->GetGlobal( "hook" )->GetMember( "Call" ) );

	Lua()->Push( "GSGameStats" );
	Lua()->PushNil(); // gamemode

	Lua()->Push( (float)pParam->m_eResult );
	Lua()->Push( (float)pParam->m_nRank );
	Lua()->Push( (float)pParam->m_unTotalConnects );
	Lua()->Push( (float)pParam->m_unTotalMinutesPlayed );

	Lua()->Call( 6, 0 );
}

void CGSContext::HandleGSStatsReceived( GSStatsReceived_t *pParam )
{
	Lua()->Push( Lua()->GetGlobal( "hook" )->GetMember( "Call" ) );

	Lua()->Push( "GSStatsReceived" );
	Lua()->PushNil(); // gamemode

	Lua()->Push( (float)pParam->m_eResult );
	Lua()->Push( pParam->m_steamIDUser.Render() );

	Lua()->Call( 4, 0 );
}

void CGSContext::HandleGSStatsUnloaded( GSStatsUnloaded_t *pParam )
{
	Lua()->Push( Lua()->GetGlobal( "hook" )->GetMember( "Call" ) );

	Lua()->Push( "GSStatsUnloaded" );
	Lua()->PushNil();

	Lua()->Push( pParam->m_steamIDUser.Render() );

	Lua()->Call( 3, 0 );
}


void CGSContext::Steam_OnStatsUnloaded( GSStatsUnloaded_t *pParam )
{
	Lua()->Push( Lua()->GetGlobal( "hook" )->GetMember( "Call" ) );

	Lua()->Push( "GSStatsUnloaded" );
	Lua()->PushNil(); // gamemode

	Lua()->Push( pParam->m_steamIDUser.Render() );

	Lua()->Call( 3, 0 );
}


void CGSContext::Steam_OnConnect( SteamServersConnected_t *pParam )
{
	this->Init();
}

void CGSContext::Steam_OnDisconnect( SteamServersDisconnected_t *pParam )
{
	this->Init();
}

CGSContext *g_pGSContext = NULL;


LUA_FUNCTION( IsLoggedOn )
{
	if ( !g_pGSContext )
	{
		Lua()->Error( "GSAPI: g_pGSContext is NULL!" );
		return 0;
	}

	Lua()->Push( g_pGSContext->IsLoggedOn() );
	return 1;
}

LUA_FUNCTION( IsSecure )
{
	if ( !g_pGSContext )
	{
		Lua()->Error( "GSAPI: g_pGSContext is NULL!" );
		return 0;
	}

	Lua()->Push( g_pGSContext->IsSecure() );
	return 1;
}

LUA_FUNCTION( GetSteamID )
{
	if ( !g_pGSContext )
	{
		Lua()->Error( "GSAPI: g_pGSContext is NULL!" );
		return 0;
	}

	Lua()->Push( g_pGSContext->GetSteamID().Render() );
	return 1;
}

LUA_FUNCTION( GetPublicIP )
{
	if ( !g_pGSContext )
	{
		Lua()->Error( "GSAPI: g_pGSContext is NULL!" );
		return 0;
	}

	netadr_t netAddr( g_pGSContext->GetPublicIP(), 0 );

	Lua()->Push( netAddr.ToString( true ) );
	return 1;
}

LUA_FUNCTION( UpdateUserData )
{
	if ( !g_pGSContext )
	{
		Lua()->Error( "GSAPI: g_pGSContext is NULL!" );
		return 0;
	}

	Lua()->CheckType( 1, GLua::TYPE_STRING );
	Lua()->CheckType( 2, GLua::TYPE_STRING );
	Lua()->CheckType( 3, GLua::TYPE_NUMBER );

	const char *pchSteamID = Lua()->GetString( 1 );
	CSteamID steamID( pchSteamID, k_EUniversePublic );

	const char *pchName = Lua()->GetString( 2 );
	uint32 uScore = Lua()->GetInteger( 3 );

	bool bRet = g_pGSContext->UpdateUserData( steamID, pchName, uScore );

	Lua()->Push( bRet );
	return 1;
}

LUA_FUNCTION( SetServerType )
{
	if ( !g_pGSContext )
	{
		Lua()->Error( "GSAPI: g_pGSContext is NULL!" );
		return 0;
	}

	Lua()->CheckType( 1, GLua::TYPE_NUMBER );
	Lua()->CheckType( 2, GLua::TYPE_NUMBER );
	Lua()->CheckType( 3, GLua::TYPE_NUMBER );
	Lua()->CheckType( 4, GLua::TYPE_NUMBER );
	Lua()->CheckType( 5, GLua::TYPE_NUMBER );
	Lua()->CheckType( 6, GLua::TYPE_STRING );
	Lua()->CheckType( 7, GLua::TYPE_STRING );
	Lua()->CheckType( 8, GLua::TYPE_BOOL );

	uint32 unServerFlags = Lua()->GetInteger( 1 );
	uint32 unGameIP = Lua()->GetInteger( 2 );
	uint16 unGamePort = (uint16)Lua()->GetInteger( 3 );
	uint16 unSpectatorPort = (uint16)Lua()->GetInteger( 4 );
	uint16 unQueryPort = (uint16)Lua()->GetInteger( 5 );

	const char *pchGameDir = Lua()->GetString( 6 );
	const char *pchVersion = Lua()->GetString( 7 );

	bool bLANMode = Lua()->GetBool( 8 );

	bool bRet = g_pGSContext->SetServerType( unServerFlags, unGameIP, unGamePort, unSpectatorPort, unQueryPort, pchGameDir, pchVersion, bLANMode );

	Lua()->Push( bRet );
	return 1;
}

LUA_FUNCTION( UpdateServerStatus )
{
	if ( !g_pGSContext )
	{
		Lua()->Error( "GSAPI: g_pGSContext is NULL!" );
		return 0;
	}

	Lua()->CheckType( 1, GLua::TYPE_NUMBER );
	Lua()->CheckType( 2, GLua::TYPE_NUMBER );
	Lua()->CheckType( 3, GLua::TYPE_NUMBER );
	Lua()->CheckType( 4, GLua::TYPE_STRING );
	Lua()->CheckType( 5, GLua::TYPE_STRING );
	Lua()->CheckType( 6, GLua::TYPE_STRING );

	int cPlayers = Lua()->GetInteger( 1 );
	int cPlayersMax = Lua()->GetInteger( 2 );
	int cBotPlayers = Lua()->GetInteger( 3 );

	const char *pchServerName = Lua()->GetString( 4 );
	const char *pchSpectatorServerName = Lua()->GetString( 5 );
	const char *pchMapName = Lua()->GetString( 6 );

	g_pGSContext->UpdateServerStatus( cPlayers, cPlayersMax, cBotPlayers, pchServerName, pchSpectatorServerName, pchMapName );
	return 0;
}

LUA_FUNCTION( SetGameTags )
{
	if ( !g_pGSContext )
	{
		Lua()->Error( "GSAPI: g_pGSContext is NULL!" );
		return 0;
	}

	Lua()->CheckType( 1, GLua::TYPE_STRING );

	const char *pchGameTags = Lua()->GetString( 1 );

	g_pGSContext->SetGameTags( pchGameTags );
	return 0;
}

LUA_FUNCTION( GetServerRep )
{
	if ( !g_pGSContext )
	{
		Lua()->Error( "GSAPI: g_pGSContext is NULL!" );
		return 0;
	}

	Lua()->Push( g_pGSContext->GetServerRep() );
	return 1;
}

LUA_FUNCTION( GetGameStats )
{
	if ( !g_pGSContext )
	{
		Lua()->Error( "GSAPI: g_pGSContext is NULL!" );
		return 0;
	}

	g_pGSContext->GetGameStats();
	return 0;
}


// gameserverstats funcs
LUA_FUNCTION( RequestUserStats )
{
	if ( !g_pGSContext )
	{
		Lua()->Error( "GSAPI: g_pGSContext is NULL!" );
		return 0;
	}

	Lua()->CheckType( 1, GLua::TYPE_STRING );

	const char *pchSteamID = Lua()->GetString( 1 );
	CSteamID steamID( pchSteamID, k_EUniversePublic );

	if ( !( steamID.IsValid() && steamID.BIndividualAccount() ) )
	{
		Lua()->Error( "GSAPI: Invalid SteamID!" );
		return 0;
	}

	Lua()->Push( g_pGSContext->RequestUserStats( steamID ) );
	return 1;
}

LUA_FUNCTION( GetUserStatInt )
{
	if ( !g_pGSContext )
	{
		Lua()->Error( "GSAPI: g_pGSContext is NULL!" );
		return 0;
	}

	Lua()->CheckType( 1, GLua::TYPE_STRING );
	Lua()->CheckType( 2, GLua::TYPE_STRING );

	const char *pchSteamID = Lua()->GetString( 1 );
	CSteamID steamID( pchSteamID, k_EUniversePublic );

	if ( !( steamID.IsValid() && steamID.BIndividualAccount() ) )
	{
		Lua()->Error( "GSAPI: Invalid SteamID!" );
		return 0;
	}

	const char *pchName = Lua()->GetString( 2 );

	int32 iData = 0;
	bool bRet = g_pGSContext->GetUserStat( steamID, pchName, &iData );

	Lua()->Push( bRet );

	if ( !bRet )
		return 1;

	Lua()->Push( (float)iData );
	return 2;

}

LUA_FUNCTION( GetUserStatFloat )
{
	if ( !g_pGSContext )
	{
		Lua()->Error( "GSAPI: g_pGSContext is NULL!" );
		return 0;
	}

	Lua()->CheckType( 1, GLua::TYPE_STRING );
	Lua()->CheckType( 2, GLua::TYPE_STRING );

	const char *pchSteamID = Lua()->GetString( 1 );
	CSteamID steamID( pchSteamID, k_EUniversePublic );

	if ( !( steamID.IsValid() && steamID.BIndividualAccount() ) )
	{
		Lua()->Error( "GSAPI: Invalid SteamID!" );
		return 0;
	}

	const char *pchName = Lua()->GetString( 2 );

	float fData = 0.0f;
	bool bRet = g_pGSContext->GetUserStat( steamID, pchName, &fData );

	Lua()->Push( bRet );

	if ( !bRet )
		return 1;

	Lua()->Push( fData );
	return 2;

}

LUA_FUNCTION( GetUserAchievement )
{
	if ( !g_pGSContext )
	{
		Lua()->Error( "GSAPI: g_pGSContext is NULL!" );
		return 0;
	}

	Lua()->CheckType( 1, GLua::TYPE_STRING );
	Lua()->CheckType( 2, GLua::TYPE_STRING );

	const char *pchSteamID = Lua()->GetString( 1 );
	CSteamID steamID( pchSteamID, k_EUniversePublic );

	if ( !( steamID.IsValid() && steamID.BIndividualAccount() ) )
	{
		Lua()->Error( "GSAPI: Invalid SteamID!" );
		return 0;
	}

	const char *pchName = Lua()->GetString( 2 );

	bool bAchieved = false;
	bool bRet = g_pGSContext->GetUserAchievement( steamID, pchName, &bAchieved );

	Lua()->Push( bRet );

	if ( !bRet )
		return 1;

	Lua()->Push( bAchieved );
	return 2;

}

