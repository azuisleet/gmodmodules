
#include "netfilter.h"

#include "cbase.h"

#include <winsock2.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


static ConVar showOOB( "ss_show_oob", "0", 0, "Display any OOB messages received" );
static ConVar cvar_conservative( "ss_oob_conservative", "0", 0, "Use CPU conservation" );

static ConVarRef sv_max_queries_sec_global( "sv_max_queries_sec_global" );
static ConVarRef sv_max_queries_window( "sv_max_queries_window" );



typedef int ( *RecvFromFn )( int s, char *buf, int len, int flags, struct sockaddr *from, int *fromlen );

enum EPacketType
{
	k_ePacketType_Bad, // traffic that should be dropped
	k_ePacketType_Good, // OOB or game traffic that should be allowed
};

#pragma pack( push, 1 )
struct Packet_t
{
	uint32 m_Channel;
	uint8 m_Type;
};
#pragma pack( pop )


EPacketType ClassifyData( char *data, int len, sockaddr_in *from )
{
	if ( len == 0 )
	{
		// drop empty packets, these cause (or caused) the server to send full updates to connected clients
		return k_ePacketType_Bad;
	}

	if ( len < 5 )
	{
		// we need at least the channel and packet type to classify
		// otherwise this is likely game traffic
		return k_ePacketType_Good;
	}

	Packet_t *pPacket = reinterpret_cast<Packet_t *>( data );

	if ( pPacket->m_Channel != -1 )
	{
		return k_ePacketType_Good; // game traffic
	}


	switch ( pPacket->m_Type )
	{
		case 'W': // server challenge request
		case 's': // master server challenge
			if ( len > 100 )
			{
				if ( showOOB.GetBool() )
				{
					Msg( "[SS3] Bad OOB! len: %d, channel: 0x%X, type: %c from %s\n", len, pPacket->m_Channel, pPacket->m_Type, inet_ntoa( from->sin_addr ) );
				}
				// we shouldn't ever get an 's' packet that's large, so we can do this first inexpensive check
				return k_ePacketType_Bad;
			}
			if ( len >= 18 )
			{
				if ( V_strncmp( data + 4, "statusResponse", 14 ) == 0 )
				{
					if ( showOOB.GetBool() )
					{
						Msg( "[SS3] Bad OOB! len: %d, channel: 0x%X, type: %c from %s\n", len, pPacket->m_Channel, pPacket->m_Type, inet_ntoa( from->sin_addr ) );
					}
					return k_ePacketType_Bad;
				}
			}
		case 'T': // server info request
		case 'U': // player info request
		case 'V': // rules request
		case 'q': // connection handshake init
		case 'k': // steam auth packet
		{
			if ( showOOB.GetBool() )
			{
				Msg( "[SS3] Good OOB! len: %d, channel: 0x%X, type: %c from %s\n", len, pPacket->m_Channel, pPacket->m_Type, inet_ntoa( from->sin_addr ) );
			}

			return k_ePacketType_Good;
		}
	}

	if ( showOOB.GetBool() )
	{
		Msg( "[SS3] Bad OOB! len: %d, channel: 0x%X, type: %c from %s\n", len, pPacket->m_Channel, pPacket->m_Type, inet_ntoa( from->sin_addr ) );
	}

	return k_ePacketType_Bad;

}

RecvFromFn vcr_recvfrom = NULL; // original vcr recvfrom func

int Hook_RecvFrom( int s, char *buf, int len, int flags, struct sockaddr *from, int *fromlen )
{
	for ( int i = 0 ; i < 2 || !cvar_conservative.GetBool() ; ++i )
	{
		int dataLen = vcr_recvfrom( s, buf, len, flags, from, fromlen );

		EPacketType ePackType = ClassifyData( buf, dataLen, reinterpret_cast<sockaddr_in *>( from ) );
	
		if ( ePackType == k_ePacketType_Good )
			return dataLen; // if the data is good, pass it along to the game
	}
	
	// otherwise we drop it
	WSASetLastError( WSAEWOULDBLOCK );
	return SOCKET_ERROR;
}




void NetFilter_Load()
{
	sv_max_queries_sec_global.Init( "sv_max_queries_sec_global", false );
	sv_max_queries_sec_global.SetValue( 99999999 ); // sick number

	sv_max_queries_window.Init( "sv_max_queries_window", false );
	sv_max_queries_window.SetValue( 1 );


	vcr_recvfrom = g_pVCR->Hook_recvfrom; // store off the original vcr hook
	g_pVCR->Hook_recvfrom = &Hook_RecvFrom; // set ours
}

void NetFilter_Unload()
{
	// reset the vcr hook
	g_pVCR->Hook_recvfrom = vcr_recvfrom;
}