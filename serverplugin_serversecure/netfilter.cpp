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

enum ClientOperationLevel
{
	OPERATION_NONE,
	OPERATION_CONNECTING,
	OPERATION_CONNECTEDPARTIAL,
	OPERATION_CONNECTEDFULL,
};

char *levelnames[] = 
{
	"None",
	"Pending",
	"Partial",
	"Connected"
};

struct ClientInformation
{
	ClientOperationLevel level;
	uint32 challenge;

	time_t timeout;
	int userid;
};

namespace boost {
	size_t hash_value(in_addr const &t) {
		return t.S_un.S_addr;
	}
}

typedef boost::unordered_map<in_addr, ClientInformation *> ClientInfoMap;
ClientInfoMap clientMap;

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

// commands
CON_COMMAND(ss_showconnections, "Show all connections")
{
	Msg("Active Connections:\n");
	Msg("       IP       |   Stage\n");

	for(ClientInfoMap::const_iterator iter = clientMap.begin(); iter != clientMap.end(); ++iter)
	{
		Msg("%-17s %-11s\n", inet_ntoa(iter->first), levelnames[iter->second->level]);
	}
}

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

	// cache a2s_info
	if(packet->channel == OOB && packet->type == 'T')
	{
		if(time(NULL) > cache_timer)
			return true;

		wsock_sendto(s, (const char *)querybuffer, querylength, 0, from, fromlength);
		return false;
	}
	// cache player requests
	else if(packet->channel == OOB && packet->type == 'U')
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
	if(packet->channel == OOB && (packet->type == 'l' || packet->type == 'i'))
		return false;

	ClientInfoMap::const_iterator iter = clientMap.find(sin->sin_addr);
	ClientInformation *info;
	ClientOperationLevel oplevel = OPERATION_NONE;

	if(iter != clientMap.end())
	{
		info = iter->second;
		oplevel = info->level;
	} else {
		info = new ClientInformation;
		info->level = OPERATION_NONE;
		info->timeout = time(NULL) + 60;
		info->challenge = 0;
		info->userid = -1;

		clientMap.insert(ClientInfoMap::value_type(sin->sin_addr, info));
	}

	if(packet->channel == OOB)
	{
		if(cvar_showoob.GetBool())
			Msg("packet len: %d channel: %X type %c from: %s\n", retlen, packet->channel, packet->type, inet_ntoa(sin->sin_addr));
	}

	// pass through for players ingame
	if(oplevel >= OPERATION_CONNECTEDPARTIAL)
		return true;

	// pass through master server specific packets
	if(packet->channel == OOB && (packet->type == 's' || packet->type == 'X' || packet->type == 'N' || packet->type == 'O'))
		return true;

	// pass through some reasonable connectionless (lite challenge, rule info)
	if(packet->channel == OOB && (packet->type == 'W' || packet->type == 'V'))
		return true;

	// check for connection challenges
	if(packet->channel == OOB && packet->type == 'q')
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

		info->level = OPERATION_CONNECTING;
		return true;
	}
	// check for auths 
	else if (oplevel > OPERATION_NONE && (packet->channel == OOB && packet->type == 'k'))
	{
		// we only need to validate the challenge, this will hinder spoof attacks a little more
		// (they can only send the whitelisted connectionless types)

		if(retlen < (int)sizeof(GamePacketChallenge))
		{
			Msg("Client sent too short of a key auth: %s\n", inet_ntoa(sin->sin_addr));
			return false;
		}

		GamePacketChallenge *packetchallenge = (GamePacketChallenge *)buf;

		if(packetchallenge->challenge != info->challenge)
		{
			Msg("Client sent wrong challenge: %s\n", inet_ntoa(sin->sin_addr));
			return false;
		}

		if(!ValidateKPacket((byte *)buf, retlen, sin->sin_addr))
		{
			Msg("Auth ticket did not pass local validation: %s\n", inet_ntoa(sin->sin_addr));

			int len = 0;
			byte *buff = CreateRejection("Error validating Steam ticket.", &len);
			wsock_sendto(s, (const char *)buff, len, 0, from, fromlength);
			delete buff;

			return false;
		}

		info->level = OPERATION_CONNECTEDPARTIAL;
		info->timeout = time(NULL) + 60;
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
			else if(packet->type == 'A' && len >= (int)sizeof(GamePacketServerChallenge))
			{
				GamePacketServerChallenge *packetsc = (GamePacketServerChallenge *)buf;

				sockaddr_in *sin = (sockaddr_in *)to;
				ClientInfoMap::const_iterator iter = clientMap.find(sin->sin_addr);

				if(iter != clientMap.end())
				{
					iter->second->challenge = packetsc->challenge;
				}
			}
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

	ClientInfoMap::iterator iter = clientMap.begin();
	while( iter != clientMap.end() )
	{
		delete iter->second;
		iter = clientMap.erase(iter);
	}
}

void NetFilter_LevelShutdown()
{
	for(ClientInfoMap::const_iterator iter = clientMap.begin(); iter != clientMap.end(); ++iter)
	{
		if(iter->second->level == OPERATION_CONNECTEDFULL)
		{
			iter->second->level = OPERATION_CONNECTEDPARTIAL;
			iter->second->timeout = time(NULL) + 30;
		}
	}
}

void NetFilter_ClientConnect(INetChannel *channel, edict_t *pEntity)
{
	const netadr_t &addr = channel->GetRemoteAddress();

	in_addr temp;
	temp.S_un.S_addr = addr.GetIP();

	ClientInfoMap::iterator iter = clientMap.find(temp);

	if(iter == clientMap.end())
	{
		Msg("A user connected we didn't know about: %s\n", addr.ToString());
	}
	else
	{
		iter->second->userid = engine->GetPlayerUserId(pEntity);
		iter->second->level = OPERATION_CONNECTEDFULL;
	}
}

void NetFilter_ClientDisconnect(INetChannel *channel)
{
	const netadr_t &addr = channel->GetRemoteAddress();

	in_addr temp;
	temp.S_un.S_addr = addr.GetIP();

	ClientInfoMap::iterator iter = clientMap.find(temp);

	if(iter == clientMap.end())
	{
		Msg("A user disconnected we didn't know about: %s\n", addr.ToString());
	}
	else
	{
		if(iter->second->level < OPERATION_CONNECTEDFULL)
			return;

		delete iter->second;
		clientMap.erase(iter);
	}
}

void NetFilter_Tick(const time_t& now)
{
	ClientInfoMap::iterator iter = clientMap.begin();
	while( iter != clientMap.end() )
	{
		if(iter->second->level <= OPERATION_CONNECTEDPARTIAL && now >= iter->second->timeout)
		{
			delete iter->second;
			iter = clientMap.erase(iter);
		}
		else if(iter->second->level == OPERATION_CONNECTEDFULL && !lookup_userid(iter->second->userid))
		{
			delete iter->second;
			iter = clientMap.erase(iter);
		}
		else
			++iter;
	}
}