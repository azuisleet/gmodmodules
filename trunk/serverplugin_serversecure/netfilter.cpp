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

time_t cache_timer = 0;
time_t cache_players_timer = 0;

byte *querybuffer = NULL;
int querylength = 0;

uint32 playerschallenge = 0;
byte *playerbuffer = NULL;
int playerlength = 0;

const uint32 OOB = 0xFFFFFFFF;
const char FOOTER[15] = "00000000000000";
byte CACHECHALLENGE[] = {0xFF, 0xFF, 0xFF, 0xFF, 'A', 0, 0, 0, 0};

// cvars
static ConVar cvar_cachetime("ss_query_cachetime", "5", 0, "How many seconds to cache a query");
static ConVar cvar_showoob("ss_show_oob", "0", 0, "Print out OOB packets");

int (*vcr_recvfrom) (int s, char *buf, int len, int flags, struct sockaddr *from, int *fromlen);
int (WINAPI *wsock_sendto) (SOCKET s, const char *buf, int len, int flags, const struct sockaddr *to, int tolen);

bool ClassifyPacket(int s, char *buf, sockaddr *from, int fromlength, int retlen)
{
	// ignore 0 length packets
	if(retlen == 0)
		return false;

	// pass through packets (-1 and < 5 bytes)
	if(retlen < (int)sizeof(GamePacket))
		return true;

	GamePacket *packet = (GamePacket *)buf;
	sockaddr_in *sin = (sockaddr_in *)from;

	// don't bother filtering game packets
	if(packet->channel != OOB)
		return true;

	// cache a2s_info
	if(packet->type == 'T')
	{
		if(time(NULL) > cache_timer)
			return true;

		wsock_sendto(s, (const char *)querybuffer, querylength, 0, from, fromlength);
		return false;
	}
	// cache player requests
	else if(packet->type == 'U')
	{
		return true;
		/*if(retlen != (int)sizeof(GamePacketServerChallenge))
			return false;

		if(time(NULL) > cache_players_timer)
			return true;

		GamePacketServerChallenge *packetc = (GamePacketServerChallenge *)buf;

		if(packetc->challenge == -1)
			wsock_sendto(s, (const char *)CACHECHALLENGE, sizeof(CACHECHALLENGE), 0, from, fromlength);
		else if(packetc->challenge == playerschallenge)
			wsock_sendto(s, (const char *)playerbuffer, playerlength, 0, from, fromlength);

		return false;*/
	}

	// fast deny of packets, a2c_print, spam
	if((packet->type == 'l' || packet->type == 'i'))
		return false;

	if(cvar_showoob.GetBool())
		Msg("packet len: %d channel: %X type %c from: %s\n", retlen, packet->channel, packet->type, inet_ntoa(sin->sin_addr));


	// pass through master server specific packets
	if((packet->type == 's' || packet->type == 'X' || packet->type == 'N' || packet->type == 'O'))
		return true;

	// pass through some reasonable connectionless (lite challenge, rule info)
	if((packet->type == 'W' || packet->type == 'V'))
		return true;

	// check for connection challenges
	if(packet->type == 'q')
	{
		// let the game handle it (master server) clients shouldn't be able to get past
		if(retlen < (int)sizeof(GamePacketFooter))
			return true;

		GamePacketFooter *packetfooter = (GamePacketFooter *)buf;
		if(strncmp(packetfooter->footer, FOOTER, sizeof(packetfooter->footer)) != 0)
		{
			Msg("Bad connect challenge (footer didn't match): %s\n", inet_ntoa(sin->sin_addr));
			return false;
		}

		return true;
	}
	// check for auths 
	else if (packet->type == 'k')
	{
		// we only need to validate the challenge, this will hinder spoof attacks a little more
		// (they can only send the whitelisted connectionless types)

		if(retlen < (int)sizeof(GamePacketChallenge))
		{
			Msg("Client sent too short of a key auth: %s\n", inet_ntoa(sin->sin_addr));
			return false;
		}

		GamePacketChallenge *packetchallenge = (GamePacketChallenge *)buf;

		char *name = NULL, *password = NULL, *steamid = NULL;
		bool validated = ValidateKPacket((byte *)buf, retlen, sin->sin_addr, &name, &password, &steamid);

		Msg("Client \"%s\" [%s:%d] connecting. (Password: %s SteamID: %s)\n", name, inet_ntoa(sin->sin_addr), ntohs(sin->sin_port), password, steamid);

		if(!validated)
		{
			Msg("Auth ticket did not pass local validation: %s\n", inet_ntoa(sin->sin_addr));

			int len = 0;
			byte *buff = CreateRejection("Error validating Steam ticket.", &len);
			wsock_sendto(s, (const char *)buff, len, 0, from, fromlength);
			delete buff;

			return false;
		}

		return true;
	}

	// otherwise drop it (they aren't challenging and they aren't sending auth
	return false;
}

int SSRecvFrom(int s, char *buf, int len, int flags, struct sockaddr *from, int *fromlen)
{
	for(int i = 0; i < 2; ++i)
	{
		int retlen = vcr_recvfrom(s, buf, len, flags, from, fromlen);

		if(ClassifyPacket(s, buf, from, *fromlen, retlen))
		{
			return retlen;
		}
	}

	WSASetLastError(WSAEWOULDBLOCK); 
	return SOCKET_ERROR;

}

int WINAPI SSSendTo(SOCKET s, const char *buf, int len, int flags, const struct sockaddr *to, int tolen)
{
	if(len >= (int)sizeof(GamePacket))
	{
		GamePacket *packet = (GamePacket *)buf;

		if(packet->channel == OOB)
		{
			// cache this reply
			if(packet->type == 'I')
			{
				if(querybuffer != NULL)
					delete querybuffer;

				//someday eh
				//*(short *)(buf + len - 2) = 27018;

				querylength = len;
				querybuffer = new byte[len];
				memcpy(querybuffer, buf, len);

				cache_timer = time(NULL) + cvar_cachetime.GetInt();
			}
			/*else if(packet->type == 'D')
			{
				if(playerbuffer != NULL)
					delete playerbuffer;

				playerlength = len;
				playerbuffer = new byte[len];
				memcpy(playerbuffer, buf, len);

				cache_players_timer = time(NULL) + cvar_cachetime.GetInt();
			}*/
		}
	}

	return wsock_sendto(s, buf, len, flags, to, tolen);
}


void NetFilter_Load()
{
	srand(time(NULL));
	playerschallenge = rand();
	*(uint32 *)(CACHECHALLENGE + 5) = playerschallenge;

	vcr_recvfrom = g_pVCR->Hook_recvfrom;
	g_pVCR->Hook_recvfrom = &SSRecvFrom;

	wsock_sendto = sendto;

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
		DetourAttach(&(PVOID&)wsock_sendto, SSSendTo);
	DetourTransactionCommit();
}

void NetFilter_Unload()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
		DetourDetach(&(PVOID&)wsock_sendto, SSSendTo);
	DetourTransactionCommit();

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