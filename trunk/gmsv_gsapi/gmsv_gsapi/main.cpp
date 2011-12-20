

#include "GSContext.h"

#include "netadr.h"


GMOD_MODULE( Load, Unload );




LUA_FUNCTION( Load )
{
	if ( Lua()->IsClient() )
	{
		Lua()->Error( "gmsv_gsapi cannot be loaded on the client." );
		return 0;
	}

	if ( g_pGSContext )
		delete g_pGSContext;

	g_pGSContext = new CGSContext( L );

	// try a manual init for changelevels
	g_pGSContext->Init();

	ILuaObject *gsTable = Lua()->GetNewTable();
		gsTable->SetMember( "IsLoggedOn", IsLoggedOn );
		gsTable->SetMember( "IsSecure", IsSecure );
		gsTable->SetMember( "GetSteamID", GetSteamID );
		gsTable->SetMember( "GetPublicIP", GetPublicIP );
		gsTable->SetMember( "UpdateUserData", UpdateUserData );
		gsTable->SetMember( "SetType", SetServerType );
		gsTable->SetMember( "UpdateStatus", UpdateServerStatus );
		gsTable->SetMember( "SetGameTags", SetGameTags );
		gsTable->SetMember( "GetReputation", GetServerRep );
		gsTable->SetMember( "GetGameplayStats", GetGameStats );
	Lua()->SetGlobal( "gameserver", gsTable );

	gsTable->UnReference();


	ILuaObject *gssTable = Lua()->GetNewTable();
		gssTable->SetMember( "RequestUserStats", RequestUserStats );
		gssTable->SetMember( "GetUserStatInt", GetUserStatInt );
		gssTable->SetMember( "GetUserStatFloat", GetUserStatFloat );
		gssTable->SetMember( "GetUserAchievement", GetUserAchievement );
	Lua()->SetGlobal( "gameserverstats", gssTable );

	gssTable->UnReference();


	return 0;
}

LUA_FUNCTION( Unload )
{
	if ( g_pGSContext )
		delete g_pGSContext;

	return 0;
}
