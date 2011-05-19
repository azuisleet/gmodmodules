#ifndef TMPSERVER_H
#define TMPSERVER_H

class bf_write;
class CBaseClient;
class CClientFrame;
class CFrameSnapshot;
class INetMessage;
class IRecipientFilter;
class IClient;

typedef struct player_info_s player_info_t;

typedef void unknown_ret;

abstract_class IServer : public IConnectionlessPacketHandler
{
public:
	virtual	~IServer() {}

	virtual int		GetNumClients( void ) const = 0; // returns current number of clients
	virtual int		GetNumProxies( void ) const = 0; // returns number of attached HLTV proxies
	virtual int		GetNumFakeClients() const = 0; // returns number of fake clients/bots
	virtual int		GetMaxClients( void ) const = 0; // returns current client limit
	virtual IClient	*GetClient( int index ) = 0; // returns interface to client 
	virtual int		GetClientCount() const = 0; // returns number of clients slots (used & unused)
	virtual int		GetUDPPort( void ) const = 0; // returns current used UDP port
	virtual float	GetTime( void ) const = 0;	// returns game world time
	virtual int		GetTick( void ) const = 0;	// returns game world tick
	virtual float	GetTickInterval( void ) const = 0; // tick interval in seconds
	virtual const char *GetName( void ) const = 0;	// public server name
	virtual const char *GetMapName( void ) const = 0; // current map name (BSP)
	virtual int		GetSpawnCount( void ) const = 0;	
	virtual int		GetNumClasses( void ) const = 0;
	virtual int		GetClassBits( void ) const = 0;
	virtual void	GetNetStats( float &avgIn, float &avgOut ) = 0; // total net in/out in bytes/sec
	virtual int		GetNumPlayers() = 0;
	virtual	bool	GetPlayerInfo( int nClientIndex, player_info_t *pinfo ) = 0;

	virtual bool	IsActive( void ) const = 0;	
	virtual bool	IsLoading( void ) const = 0;
	virtual bool	IsDedicated( void ) const = 0;
	virtual bool	IsPaused( void ) const = 0;
	virtual bool	IsMultiplayer( void ) const = 0;
	virtual bool	IsPausable() const = 0;
	virtual bool	IsHLTV() const = 0;
	virtual bool	IsReplay() const = 0;

	virtual const char * GetPassword() const = 0;	// returns the password or NULL if none set	

	virtual void	SetPaused(bool paused) = 0;
	virtual void	SetPassword(const char *password) = 0; // set password (NULL to disable)

	virtual void	BroadcastMessage( INetMessage &msg, bool onlyActive = false, bool reliable = false) = 0;
	virtual void	BroadcastMessage( INetMessage &msg, IRecipientFilter &filter ) = 0;

	virtual void	DisconnectClient( IClient *client, const char *reason ) = 0;
};

class CBaseServer : public IServer {
public:
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
	virtual unknown_ret RejectConnection( netadr_s const&, int, const char* ) = 0;
	virtual bool CheckIPRestrictions( netadr_s const&, int ) = 0;
	virtual void* ConnectClient( netadr_s&, int, int, int, int, char const*, char const*, char const*, int ) = 0;
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

#endif // TMPSERVER_H