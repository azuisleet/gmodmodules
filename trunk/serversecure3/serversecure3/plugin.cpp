
#include "cbase.h"
#include "tier1.h"

#include "netfilter.h"
#include "filecheck.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


// Interfaces from the engine
IVEngineServer	*engine = NULL; 
IPlayerInfoManager *playerinfomanager = NULL; 
IServerPluginHelpers *helpers = NULL;
CGlobalVars *gpGlobals = NULL;


// plugin

class CSSServerPlugin: public IServerPluginCallbacks
{
public:
	CSSServerPlugin();
	~CSSServerPlugin();

	// IServerPluginCallbacks methods
	virtual bool			Load(	CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory );
	virtual void			Unload( void );
	virtual void			Pause( void );
	virtual void			UnPause( void );
	virtual const char     *GetPluginDescription( void );      
	virtual void			LevelInit( char const *pMapName );
	virtual void			ServerActivate( edict_t *pEdictList, int edictCount, int clientMax );
	virtual void			GameFrame( bool simulating );
	virtual void			LevelShutdown( void );
	virtual void			ClientActive( edict_t *pEntity );
	virtual void			ClientDisconnect( edict_t *pEntity );
	virtual void			ClientPutInServer( edict_t *pEntity, char const *playername );
	virtual void			SetCommandClient( int index );
	virtual void			ClientSettingsChanged( edict_t *pEdict );
	virtual PLUGIN_RESULT	ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen );
	virtual PLUGIN_RESULT	ClientCommand( edict_t *pEntity, const CCommand &args );
	virtual PLUGIN_RESULT	NetworkIDValidated( const char *pszUserName, const char *pszNetworkID );
	virtual void			OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue );
	virtual void			OnEdictAllocated( edict_t *edict );
	virtual void			OnEdictFreed( const edict_t *edict  );
};


CSSServerPlugin g_ServerSecureServerPlugin;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CSSServerPlugin, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_ServerSecureServerPlugin );

CSSServerPlugin::CSSServerPlugin()
{
}

CSSServerPlugin::~CSSServerPlugin()
{
}

bool CSSServerPlugin::Load(	CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory )
{
	Msg( "[SS3] Loading...\n" );
	ConnectTier1Libraries( &interfaceFactory, 1 );

	engine = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL);
	helpers = (IServerPluginHelpers*)interfaceFactory(INTERFACEVERSION_ISERVERPLUGINHELPERS, NULL);
	playerinfomanager = (IPlayerInfoManager *)gameServerFactory(INTERFACEVERSION_PLAYERINFOMANAGER,NULL);

	if(	! ( engine && helpers && playerinfomanager ) )
		return false;

	gpGlobals = playerinfomanager->GetGlobalVars();

	MathLib_Init( 2.2f, 2.2f, 0.0f, 2 );
	ConVar_Register( 0 );

	NetFilter_Load();
	//FileCheck_Load();

	return true;
}

void CSSServerPlugin::Unload( void )
{
	Msg( "[SS3] Unloading...\n" );

	//FileCheck_Unload();
	NetFilter_Unload();

	ConVar_Unregister( );
	DisconnectTier1Libraries( );
}

void CSSServerPlugin::Pause( void )
{
}

void CSSServerPlugin::UnPause( void )
{
}

const char *CSSServerPlugin::GetPluginDescription( void )
{
	return " \"Server Secure 3\" ";
}

void CSSServerPlugin::LevelInit( char const *pMapName )
{
}

void CSSServerPlugin::ServerActivate( edict_t *pEdictList, int edictCount, int clientMax )
{
}

void CSSServerPlugin::GameFrame( bool simulating )
{
}

void CSSServerPlugin::LevelShutdown( void ) 
{
}

void CSSServerPlugin::ClientActive( edict_t *pEntity )
{
}

void CSSServerPlugin::ClientDisconnect( edict_t *pEntity )
{
}

void CSSServerPlugin::ClientPutInServer( edict_t *pEntity, char const *playername )
{
}

void CSSServerPlugin::SetCommandClient( int index )
{
}

void CSSServerPlugin::ClientSettingsChanged( edict_t *pEdict )
{
}

PLUGIN_RESULT CSSServerPlugin::ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{
	return PLUGIN_CONTINUE;
}

PLUGIN_RESULT CSSServerPlugin::ClientCommand( edict_t *pEntity, const CCommand &args )
{
	return PLUGIN_CONTINUE;
}

PLUGIN_RESULT CSSServerPlugin::NetworkIDValidated( const char *pszUserName, const char *pszNetworkID )
{
	return PLUGIN_CONTINUE;
}

void CSSServerPlugin::OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue )
{
}

void CSSServerPlugin::OnEdictAllocated( edict_t *edict )
{
}
void CSSServerPlugin::OnEdictFreed( const edict_t *edict )
{
}