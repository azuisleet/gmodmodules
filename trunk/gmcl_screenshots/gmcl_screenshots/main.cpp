
#include "GMLuaModule.h"

#include "materialsystem/imaterialsystem.h"
#include "tier1/iconvar.h"
#include "tier1/tier1.h"
#include "vstdlib/cvar.h"

#include "Steamworks.h"


GMOD_MODULE( Start, Close );


IMaterialSystem *pMatSys = NULL;
IMatRenderContext *pRender = NULL;

IClientEngine *pClientEngine = NULL;
IClientRemoteStorage *pRemoteStorage = NULL;


static ConVar cvar_debug( "screenshots_debug", "0", 0, "Show debug messages." );


LUA_FUNCTION( Screenshot )
{
	Assert( pRender );
	Assert( pRemoteStorage );

	int x, y, width, height;
	pRender->GetViewport( x, y, width, height );

	if ( cvar_debug.GetBool() )
	{
		Lua()->Msg( "[Screenshots] Viewport: Origin: ( %d, %d ) Size: ( %d, %d )\n", x, y, width, height );
	}

	unsigned char *pData = new( std::nothrow ) unsigned char[ width * height * 3 ];
	if ( !pData )
	{
		Lua()->Error( "[Screenshots] Unable to allocate data for image.\n" );
		Lua()->Push( false );
		return 1;
	}

	pRender->ReadPixels( x, y, width, height, pData, IMAGE_FORMAT_RGB888 );

	// todo: IClient interfaces are inherently unsafe, should sigscan for this function in the near future
	bool bWrote = pRemoteStorage->ScreenshotWrite( CGameID( 4000 /* gmode */ ), pData, width * height * 3, width, height );

	if ( cvar_debug.GetBool() )
	{
		Lua()->Msg( "[Screenshots] IClientRemoteStorage::ScreenshotWrite = %d\n", bWrote );
	}

	delete [] pData;

	Lua()->Push( bWrote );
	return 1;
}


LUA_FUNCTION( Start )
{
	if ( !Lua()->IsClient() )
	{
		Lua()->Error( "[Screenshots] This module can only be loaded on the client.\n" );
		return 0;
	}


	// hookup icvar
	CreateInterfaceFn tier1Factory = VStdLib_GetICVarFactory();

	if ( !tier1Factory ) 
	{
		Lua()->Msg( "[Screenshots] Unable to get ICVar factory, convars disabled.\n" );
	}
	else
	{
		ConnectTier1Libraries( &tier1Factory, 1 );
		g_pCVar->Connect( tier1Factory );
	}

	ConVar_Register();


	// grab our required materialsystem interfaces
	CreateInterfaceFn materialFactory = Sys_GetFactory( "materialsystem" );
	if ( !materialFactory )
	{
		Lua()->Error( "[Screenshots] Unable to load materialsystem library.\n" );
		return 0;
	}

	pMatSys = ( IMaterialSystem *)materialFactory( MATERIAL_SYSTEM_INTERFACE_VERSION, NULL );
	if ( !pMatSys )
	{
		Lua()->Error( "[Screenshots] Unable to get IMaterialSystem.\n" );
		return 0;
	}

	pRender = pMatSys->GetRenderContext();
	if ( !pRender )
	{
		Lua()->Error( "[Screenshots] Unable to get render context.\n" );
		return 0;
	}


	// setup steamapi
	// it seems when the module is loaded the api is already setup, so we just need the pipe and user
	HSteamPipe hPipe = SteamAPI_GetHSteamPipe();
	HSteamUser hUser = SteamAPI_GetHSteamUser();

#if DEBUG
	Lua()->Msg( "[Screenshots] HSteamPipe = %d, HSteamUser = %d\n" );
#endif

	CreateInterfaceFn steamFactory = Sys_GetFactory( "steamclient" );
	if ( !steamFactory )
	{
		Lua()->Error( "[Screenshots] Unable to load steamclient library.\n" );
		return 0;
	}

	pClientEngine = ( IClientEngine *)steamFactory( CLIENTENGINE_INTERFACE_VERSION, NULL );
	if ( !pClientEngine )
	{
		Lua()->Error( "[Screenshots] Unable to get IClientEngine.\n" );
		return 0;
	}

	pRemoteStorage = pClientEngine->GetIClientRemoteStorage( hUser, hPipe, CLIENTREMOTESTORAGE_INTERFACE_VERSION );
	if ( !pRemoteStorage )
	{
		Lua()->Error( "[Screenshots] Unable to get IClientRemoteStorage.\n" );
		return 0;
	}


	ILuaObject *table = Lua()->GetNewTable();
		table->SetMember( "Take", Screenshot );
	Lua()->SetGlobal( "screenshots", table );

	table->UnReference();

	Lua()->Msg( "[Screenshots] Loaded!\n" );

	return 0;

}

LUA_FUNCTION( Close )
{
	ConVar_Unregister();

	if ( g_pCVar )
	{
		g_pCVar->Disconnect();
		DisconnectTier1Libraries();
	}

	Lua()->Msg( "[Screenshots] Unloaded!\n" );

	return 0;
}
