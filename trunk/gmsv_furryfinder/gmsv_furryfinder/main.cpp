

#include "FurryFinder.h"



GMOD_MODULE( Load, Unload );




LUA_FUNCTION( ReqGroupStatus )
{
	if ( !g_pFurryFinder )
	{
		Lua()->Error( "FurryFinder: pFurryFinder is NULL!\n" );

		Lua()->PushNil();
		return 1;
	}

	Lua()->CheckType( 1, GLua::TYPE_STRING );
	Lua()->CheckType( 2, GLua::TYPE_STRING );

	const char *strSteamUser = Lua()->GetString( 1 );
	const char *strSteamGroup = Lua()->GetString( 2 );

	ISteamUtils *pUtils = g_pFurryFinder->GetUtils();

	CSteamID steamUser( strSteamUser, ( pUtils != NULL ? pUtils->GetConnectedUniverse() : k_EUniversePublic ) );

	uint64 steamGrp64 = _atoi64( strSteamGroup );

	if ( steamGrp64 == 0 )
	{
		Lua()->Error( "FurryFinder: invalid steam group.\n" );

		Lua()->PushNil();
		return 1;
	}

	CSteamID steamGroup( steamGrp64 );

	bool bRet = g_pFurryFinder->RequestGroupStatus( steamUser, steamGroup );

	Lua()->Push( bRet );

	return 1;
}

LUA_FUNCTION( Load )
{
	if ( Lua()->IsClient() )
	{
		Lua()->Error( "gmsv_furryfinder cannot be loaded on the client." );
		return 0;
	}

	if ( g_pFurryFinder )
		delete g_pFurryFinder;

	g_pFurryFinder = new CFurryFinder( L );

	// try a manual init for changelevels
	g_pFurryFinder->Init();

	ILuaObject *ffTable = Lua()->GetNewTable();
		ffTable->SetMember( "RequestGroupStatus", ReqGroupStatus );
	Lua()->SetGlobal( "furryfinder", ffTable );

	ffTable->UnReference();

	return 0;
}

LUA_FUNCTION( Unload )
{
	if ( g_pFurryFinder )
		delete g_pFurryFinder;

	return 0;
}

