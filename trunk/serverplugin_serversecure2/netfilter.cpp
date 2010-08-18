#include "plugin.h"

#pragma pack(push, 1)
struct GamePacket
{
	uint32 channel;
	uint8 type;
};

struct GamePacketChallenge
{
	uint32 channel;
	uint8 type;
	uint32 challenge;
};
#pragma pack(pop)

struct PendingPacket
{
	uint8 type;
	union 
	{
		uint32 challenge;
		byte* data;
	};
	int len;
	sockaddr_in addr;
};

enum PacketClassification
{
	PACKET_TYPE_BAD,
	PACKET_TYPE_GAME,
	PACKET_TYPE_OOB,
};

const uint32 OOB = 0xFFFFFFFF;
const char FOOTER[15] = "00000000000000";
const char QUERY[20] = "Source Engine Query";

CUtlStack< PendingPacket > oobPacketQueue;

time_t injectWindow;
int injected;
bool injectedLastCall;

// cvars
static ConVar cvar_showoob( "ss_oob_show", "0", 0, "Print out OOB packets" );
static ConVar cvar_oobcapacity( "ss_oob_capacity", "25", 0, "Default queue capacity" );
static ConVar cvar_conservative( "ss_oob_conservative", "0", 0, "Use CPU conservation" );

int (*vcr_recvfrom) ( int s, char *buf, int len, int flags, struct sockaddr *from, int *fromlen );

int IsReconstructable( char type )
{
	switch( type )
	{
		case 'T':
			return 25;
		case 'U':
		case 'V':
			return 9;
		case 'W':
			return 5;
		case 'q':
			return 20;
		default:
			return 0;
	}
}

PacketClassification ClassifyPacket( int s, char *buf, sockaddr *from, int fromlength, int retlen )
{
	// ignore 0 length packets
	if( retlen == 0 )
		return PACKET_TYPE_BAD;

	// pass through packets (-1 and < 5 bytes)
	if( retlen < (int)sizeof(GamePacket) )
		return PACKET_TYPE_GAME;

	GamePacket *packet = (GamePacket *)buf;
	sockaddr_in *sin = (sockaddr_in *)from;

	if( packet->channel != OOB )
		return PACKET_TYPE_GAME;

	if ( 
		packet->type != 'T'		// A2S_INFO				-	25
		&& packet->type != 'U'	// Player info request	-	9

		&& packet->type != 'N'	
		&& packet->type != 'X'

		&& packet->type != 'W'	// challenge request	-	5
		&& packet->type != 'V'	// rules request		-	9
		&& packet->type != 'O'

		&& packet->type != 's'	// master server challenge
		&& packet->type != 'q'	// connect challenge	-	19
		&& packet->type != 'k'	// steam key
		)
	{
		if( cvar_showoob.GetBool() )
			Msg( "[BAD OOB] packet len: %d channel: %X type %c from: %s\n", retlen, packet->channel, packet->type, inet_ntoa( sin->sin_addr ) );

		return PACKET_TYPE_BAD;
	}

	char *name = NULL, *password = NULL;

	if ( packet->type == 'k' && !ValidateKPacket( (byte *)buf, retlen, sin->sin_addr, &name, &password ) )
	{
		Msg( "Auth ticket did not pass local validation: %s\n", inet_ntoa( sin->sin_addr ) );

		int len = 0;
		byte *buff = CreateRejection( "Error validating Steam ticket.", &len );
		sendto( s, (const char *)buff, len, 0, from, fromlength );
		delete buff;

		return PACKET_TYPE_BAD;
	}

	int reconlen = IsReconstructable( packet->type );
	if ( reconlen > 0 && retlen != reconlen )
	{
		Msg( "Got known OOB packet outside of expected size %c: %d\n", packet->type, retlen );

		return PACKET_TYPE_BAD;
	}

	if( cvar_showoob.GetBool() )
		Msg( "packet len: %d channel: %X type %c from: %s length: %d\n", retlen, packet->channel, packet->type, inet_ntoa( sin->sin_addr ), retlen );


	return PACKET_TYPE_OOB;
}

void DropOOB( int s, char *buf, int len, sockaddr *from, int fromlength )
{
	GamePacket *packet = (GamePacket *)buf;

	if ( cvar_showoob.GetBool() )
	{
		Msg( "OOB queue size: %d\n", oobPacketQueue.Count() );
	}

	oobPacketQueue.Push();
	PendingPacket& pending = oobPacketQueue.Top();

	pending.type = packet->type;
	memcpy( &pending.addr, from, sizeof(pending.addr) );

	pending.len = len;

	int reconlen = IsReconstructable( packet->type );
	if ( reconlen > 0 )
	{
		if ( reconlen > 5 )
		{
			GamePacketChallenge *challenge = (GamePacketChallenge *)buf;

			pending.challenge = challenge->challenge;
		}
	}
	else
	{
		byte* buffer = new byte[ len ];
		memcpy( buffer, buf, len );

		pending.data = buffer;
	}
}




bool InjectOOB( char* buf, int* len, sockaddr* from, int* fromlen )
{
	if ( oobPacketQueue.Count() == 0 )
	{
		return false;
	}

	if ( injectedLastCall )
	{
		injectedLastCall = false;
		return false;
	}

	time_t now = time( NULL );
	if ( now > injectWindow )
	{
		injectWindow = now;
		injected = 0;
	}

	// has to be less than sv_max_queries_sec_global
	if ( injected > 50 )
	{
		return false;
	}

	PendingPacket& pending = oobPacketQueue.Top();

	*fromlen = sizeof( pending.addr );
	memcpy( from, &pending.addr, *fromlen );

	int reconlen = IsReconstructable( pending.type );
	if ( reconlen > 0 )
	{
		*len = reconlen;
		GamePacket* gamepacket = (GamePacket*) buf;

		gamepacket->channel = OOB;
		gamepacket->type = pending.type;

		switch( pending.type )
		{
			case 'T':
				memcpy( buf + sizeof(GamePacket), QUERY, sizeof(QUERY) );
				break;

			case 'U':
			case 'V':
				{
					GamePacketChallenge* gamepacketchallenge = (GamePacketChallenge*) buf;
					gamepacketchallenge->challenge = pending.challenge;
					break;
				}

			case 'q':
				memcpy( buf + sizeof(GamePacket), FOOTER, sizeof(FOOTER) );
				break;
		}
	}
	else
	{
		*len = pending.len;

		memcpy( buf, pending.data, pending.len );
		delete pending.data;
	}

	oobPacketQueue.Pop();

	injectedLastCall = true;
	return true;
}

int SSRecvFrom(int s, char *buf, int len, int flags, struct sockaddr *from, int *fromlen)
{
	if ( InjectOOB( buf, &len, from, fromlen) )
	{
		return len;
	}

	for(int i = 0; i < 2 || !cvar_conservative.GetBool(); ++i)
	{
		int retlen = vcr_recvfrom( s, buf, len, flags, from, fromlen );
		PacketClassification pclass = ClassifyPacket( s, buf, from, *fromlen, retlen );

		switch( pclass )
		{
			case PACKET_TYPE_GAME:
				return retlen;
			break;
			case PACKET_TYPE_OOB:
				DropOOB( s, buf, retlen, from, *fromlen );
			break;
		}
	}

#ifdef WIN32
	WSASetLastError(WSAEWOULDBLOCK); 
	return SOCKET_ERROR;
#else
	errno = EWOULDBLOCK;
	return -1;
#endif
}

void NetFilter_Load()
{
	oobPacketQueue.EnsureCapacity( cvar_oobcapacity.GetInt() );

	injectWindow = time( NULL );
	injected = 0;
	injectedLastCall = false;

	vcr_recvfrom = g_pVCR->Hook_recvfrom;
	g_pVCR->Hook_recvfrom = &SSRecvFrom;
}

void NetFilter_Unload()
{
	oobPacketQueue.Purge();

	g_pVCR->Hook_recvfrom = vcr_recvfrom;
}
