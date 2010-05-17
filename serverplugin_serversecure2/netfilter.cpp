#include "plugin.h"

#pragma pack(push, 1)
struct GamePacket
{
	uint32 channel;
	uint8 type;
};

struct GamePacketFooter
{
	uint32 channel;
	uint8 type;
	char footer[15];
};

struct GamePacketServerChallenge
{
	uint32 channel;
	uint8 type;
	uint32 challenge;
};

struct GamePacketChallenge
{
	uint32 channel;
	uint8 type;
	uint32 protocolver;
	uint32 authprotover;
	uint32 challenge;
};
#pragma pack(pop)

enum PacketClassification
{
	PACKET_TYPE_BAD,
	PACKET_TYPE_GAME,
	PACKET_TYPE_OOB,
};

const uint32 OOB = 0xFFFFFFFF;
const char FOOTER[15] = "00000000000000";

// drop-inject queue
struct PacketStruct
{
	char* buffer;
	int len;
	char* from;
	int fromlen;
};

CUtlStack<PacketStruct*> oobQueue;

time_t injectWindow;
int injected;
bool injectedLastCall;

// cvars
static ConVar cvar_showoob("ss_show_oob", "0", 0, "Print out OOB packets");

int (*vcr_recvfrom) (int s, char *buf, int len, int flags, struct sockaddr *from, int *fromlen);
int (WINAPI *wsock_sendto) (SOCKET s, const char *buf, int len, int flags, const struct sockaddr *to, int tolen);

PacketClassification ClassifyPacket(int s, char *buf, sockaddr *from, int fromlength, int retlen)
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
		packet->type != 'T'
		&& packet->type != 'U'
		&& packet->type != 'N'
		&& packet->type != 'X'
		&& packet->type != 'W'
		&& packet->type != 'V'
		&& packet->type != 'O'

		&& packet->type != 's'
		&& packet->type != 'q'
		&& packet->type != 'k'
		)
	{
		if( cvar_showoob.GetBool() )
			Msg("[BAD OOB] packet len: %d channel: %X type %c from: %s\n", retlen, packet->channel, packet->type, inet_ntoa(sin->sin_addr));

		return PACKET_TYPE_BAD;
	}
	if( cvar_showoob.GetBool() )
		Msg("packet len: %d channel: %X type %c from: %s\n", retlen, packet->channel, packet->type, inet_ntoa(sin->sin_addr));

	return PACKET_TYPE_OOB;
}

void DropOOB(int s, char *buf, int len, sockaddr *from, int fromlength)
{
	char* buffer = new char[len];
	memcpy( buffer, buf, len );

	char* sfrom = new char[fromlength];
	memcpy( sfrom, from, fromlength );

	PacketStruct* packet = new PacketStruct;
	packet->buffer = buffer;
	packet->len = len;
	packet->from = sfrom;
	packet->fromlen = fromlength;

	oobQueue.Push( packet );
}

bool InjectOOB( char* buf, int* len, sockaddr* from, int* fromlen )
{
	if ( oobQueue.Count() <= 0 )
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

	PacketStruct* packet;
	oobQueue.Pop( packet );

	memcpy( buf, packet->buffer, packet->len );
	*len = packet->len;
	memcpy( from, packet->from, packet->fromlen );

	delete packet->buffer;
	delete packet->from;

	delete packet;

	injectedLastCall = true;
	return true;
}

int SSRecvFrom(int s, char *buf, int len, int flags, struct sockaddr *from, int *fromlen)
{
	if ( InjectOOB( buf, &len, from, fromlen) )
	{
		return len;
	}

	for(int i = 0; i < 2; ++i)
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

	WSASetLastError(WSAEWOULDBLOCK); 
	return SOCKET_ERROR;

}

void NetFilter_Load()
{
	injectWindow = time( NULL );
	injected = 0;
	injectedLastCall = false;

	vcr_recvfrom = g_pVCR->Hook_recvfrom;
	g_pVCR->Hook_recvfrom = &SSRecvFrom;
}

void NetFilter_Unload()
{
	g_pVCR->Hook_recvfrom = vcr_recvfrom;
}

void NetFilter_LevelShutdown()
{
}

void NetFilter_ClientConnect(INetChannel *channel, edict_t *pEntity)
{
}

void NetFilter_ClientDisconnect(INetChannel *channel)
{
}

void NetFilter_Tick(const time_t& now)
{
}