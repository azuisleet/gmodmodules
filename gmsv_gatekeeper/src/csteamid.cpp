#include <steamclientpublic.h>

#include <stdio.h>

char * CSteamID::Render() const
{
	static char szSteamID[64];
	switch(m_EAccountType)
	{
	case k_EAccountTypeInvalid:
	case k_EAccountTypeIndividual:
		sprintf_s(szSteamID, sizeof(szSteamID), "STEAM_0:%u:%u", (m_unAccountID % 2) ? 1 : 0, (int32)m_unAccountID/2);
		break;
	default:
		sprintf_s(szSteamID, sizeof(szSteamID), "%llu", ConvertToUint64());
	}
	return szSteamID;
}