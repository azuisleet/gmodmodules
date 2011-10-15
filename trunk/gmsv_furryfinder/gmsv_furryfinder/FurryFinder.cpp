
#include "FurryFinder.h"

CFurryFinder::~CFurryFinder()
{
	gsContext.Clear();
}

bool CFurryFinder::RequestGroupStatus( CSteamID steamUser, CSteamID steamGroup )
{
	if ( !bInited )
	{
		Lua()->Error( "FurryFinder: RequestGroupStatus failed. GS context not initialized.\n" );
		return false;
	}

	return gsContext.SteamGameServer()->RequestUserGroupStatus( steamUser, steamGroup );
}


void CFurryFinder::Init()
{
	if ( bInited )
		return;

	bInited = gsContext.InitDetailed( this );
}

ISteamUtils *CFurryFinder::GetUtils()
{
	if ( !bInited )
		return NULL;

	return gsContext.SteamGameServerUtils();
}

void CFurryFinder::Steam_OnGSGroupStatus( GSClientGroupStatus_t *pParam )
{
	Lua()->Push( Lua()->GetGlobal( "hook" )->GetMember( "Call" ) );

	Lua()->Push( "GSGroupStatus" );
	Lua()->PushNil(); // gamemode
	Lua()->Push( pParam->m_SteamIDUser.Render() );
	Lua()->Push( pParam->m_SteamIDGroup.Render() );
	Lua()->Push( pParam->m_bMember );
	Lua()->Push( pParam->m_bOfficer );

	Lua()->Call( 6, 0 );
}

void CFurryFinder::Steam_OnSteamConnected( SteamServersConnected_t *pParam )
{
	this->Init();
}

void CFurryFinder::Steam_OnSteamDisconnected( SteamServersDisconnected_t *pParam )
{
	// sometimes steam3 will be down when the plugin is loaded, but we should try to init anyway
	this->Init();
}

CFurryFinder *g_pFurryFinder;
