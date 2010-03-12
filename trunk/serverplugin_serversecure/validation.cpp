#include "plugin.h"
#include "tier1/bitbuf.h"

bool ValidateKPacket(byte *buffer, int length)
{
	Msg("Validating app ticket..\n");
	if(length <= 17)
	{
		Msg("Impossible K packet length\n");
		return false;
	}

	bf_read buf;
	buf.StartReading(buffer, length);

	int32 channel = buf.ReadLong();
	byte type = buf.ReadByte();
	int32 protover = buf.ReadLong();
	int32 authprotover = buf.ReadLong();
	int32 challenge = buf.ReadLong();

	if(buf.GetNumBytesLeft() == 0)
		return false;

	char name[128];
	if(!buf.ReadString(name, sizeof(name)))
		return false;

	if(buf.GetNumBytesLeft() == 0)
		return false;

	char password[32];
	if(!buf.ReadString(password, sizeof(password)))
		return false;

	if(buf.GetNumBytesLeft() < 10 )
		return false;

	int16 appticketlength = buf.ReadShort();

	if(appticketlength < 1)
	{
		Msg("Impossible app ticket length\n");
		return false;
	}

	if(protover == 14)
	{
		buf.ReadLong();
	}

	int32 apptickerheaderlength = buf.ReadLong();

	if(apptickerheaderlength != 4 && apptickerheaderlength != 20)
	{
		Msg("Unexpected app ticket header length: %d\n", apptickerheaderlength);
		return false;
	}

	return true;
}