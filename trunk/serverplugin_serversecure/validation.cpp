#define CRYPTOPP_DEFAULT_NO_DLL
#include "dll.h"

#include "plugin.h"
#include "tier1/bitbuf.h"

using namespace CryptoPP;

int *serverprivatekey_length = NULL;
byte *serverprivatekey = NULL;

RSA::PrivateKey privKey;

// cvars
static ConVar cvar_ticketip("ss_enforce_ticketip", "0", 0, "Enforce ticket IP validation");

void ParseKeysOutOfFunc()
{
	HMODULE steam = GetModuleHandleA("Steam.dll");
	void *keyfunc = GetProcAddress(steam, "SteamGetEncryptionKeyToSendToNewClient");

	byte *ptr = (byte *)keyfunc;
	int counter = 0;

	for(;;)
	{
		if(*ptr == 0xCC)
			break;

		if((*ptr == 0xA1) || (*ptr == 0x83 && *(++ptr) == 0x3D))
		{
			uint32 addr = *(uint32 *)(++ptr);
			counter++;

			if(counter == 3)
			{
				serverprivatekey = *(byte **)addr;
				serverprivatekey_length = (int *)(addr - 4);
				break;
			}
			
			ptr += 4;
			continue;
		}

		++ptr;
	}
}
bool Validation_Load()
{	
	ParseKeysOutOfFunc();
	
	if(serverprivatekey_length == NULL || *serverprivatekey_length == 0)
		return false;

	Msg("priv key length: %d\n", *serverprivatekey_length);

	StringSource privateKey( serverprivatekey, *serverprivatekey_length, true);

	try {
		privKey.BERDecode(privateKey);
	} catch(...)
	{
		std::cerr << "Private key error" << std::endl;
	}

	return true;
}

byte *CreateRejection(const char *reason, int *size)
{
	*size = strlen(reason) + 1 + 4 + 1;
	byte *buff = new byte[*size];

	bf_write buf;
	buf.StartWriting(buff, *size);

	buf.WriteLong(-1);
	buf.WriteByte('9');
	buf.WriteString(reason);

	return buff;
}

bool ValidateKPacket(byte *buffer, int length, in_addr from, char **out_name, char **out_password, char **out_steamid)
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
	{
		Msg("Packet length too short 1\n");
		return false;
	}

	char name[128];
	if(!buf.ReadString(name, sizeof(name)))
	{
		Msg("Could not read name\n");
		return false;
	}

	*out_name = name;

	if(buf.GetNumBytesLeft() == 0)
	{
		Msg("Packet length too short 2\n");
		return false;
	}

	char password[32];
	if(!buf.ReadString(password, sizeof(password)))
	{
		Msg("Could not read password\n");
		return false;
	}

	*out_password = password;

	if(buf.GetNumBytesLeft() < 10 )
	{
		Msg("Packet length too short 3\n");
		return false;
	}

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

	int32 appticketheaderlength = buf.ReadLong();

	if(appticketheaderlength != 4 && appticketheaderlength != 20)
	{
		Msg("Unexpected app ticket header length: %d\n", appticketheaderlength);
		return false;
	}

	if(buf.GetNumBytesLeft() <= appticketheaderlength)
	{
		Msg("Packet length too short for header length: %d\n", appticketheaderlength);
		return false;
	}

	// at this point I'm only doing IP validation for v14
	if(protover != 14)
		return true;

	byte scratch[20];
	buf.ReadBytes(&scratch, appticketheaderlength);

	if(buf.GetNumBytesLeft() <= 152)
	{
		Msg("Not enough bytes for valid v14 auth\n");
		return false;
	}

	int32 rsaident = buf.ReadLong();

	byte aeskey[128];
	buf.ReadBytes(&aeskey, sizeof(aeskey));

	byte aesiv[16];
	buf.ReadBytes(&aesiv, sizeof(aesiv));

	uint16 plaintextLen = ntohs(buf.ReadShort());
	uint16 ciphertextLen = ntohs(buf.ReadShort());

	if(buf.GetNumBytesLeft() < ciphertextLen || plaintextLen > ciphertextLen)
	{
		Msg("Bytes left: %d Cipher len: %d plaintext len: %d\n", buf.GetNumBytesLeft(), ciphertextLen, plaintextLen);
		return false;
	}

	// couldn't find private key, we'll have to pass it
	if(serverprivatekey_length == NULL || *serverprivatekey_length == 0)
		return true;

	byte *cipherbuffer = NULL, *plaintext = NULL;

	try
	{
		AutoSeededRandomPool rng;
		RSAES_OAEP_SHA_Decryptor d( privKey );
		SecByteBlock recovered( d.MaxPlaintextLength( sizeof(aeskey) ) );

		DecodingResult result = d.Decrypt( rng, aeskey, sizeof(aeskey), recovered );

		if(!result.isValidCoding || result.messageLength != 16)
		{
			Msg("AES key bad: %d %d\n", result.isValidCoding, result.messageLength);
			return false;
		}

		recovered.resize(result.messageLength);

		CBC_Mode<AES>::Decryption aesDecrypt(recovered, result.messageLength, aesiv );

		cipherbuffer = new byte[ciphertextLen];
		buf.ReadBytes(cipherbuffer, ciphertextLen);

		plaintext = new byte[plaintextLen];

		StreamTransformationFilter dec(aesDecrypt, new ArraySink(plaintext, plaintextLen));
		dec.Put(cipherbuffer, ciphertextLen);
		dec.MessageEnd();

		delete cipherbuffer;

		uint32 ticket1, ticket2, internalip;
		memcpy(&ticket1, plaintext, 4);
		memcpy(&ticket2, plaintext + 6, 4);
		memcpy(&internalip, plaintext + 12, 4);

		ticket1 = ntohl(ticket1);
		ticket2 = ntohl(ticket2);
		internalip = ntohl(internalip);

		uint16 temp;
		memcpy(&temp, plaintext + 175, 2);

		temp = ntohs(temp);

		if(temp >= (plaintextLen - 14))
		{
			Msg("Not enough room in ticket for SteamID\n");
			delete plaintext;
			return false;
		}

		// this is the authoritative ID
		uint32 id, server, externalip;
		memcpy(&id, plaintext + 177 + temp + 2, 4);
		memcpy(&server, plaintext + 177 + temp + 6, 4);
		memcpy(&externalip, plaintext + 177 + temp + 10, 4);

		delete plaintext;

		char steambuf[32];
		_snprintf(steambuf, sizeof(steambuf), "STEAM_0:%lu:%lu", server, id);

		*out_steamid = steambuf;

		in_addr tempaddr;
		tempaddr.S_un.S_addr = externalip;

		if(!(tempaddr == from))
		{
			Msg("Bad external IP %s\n", inet_ntoa(tempaddr));

			if(cvar_ticketip.GetBool())
				return false;
		}

		Msg("id: %d server: %d external: %s\n", id, server, inet_ntoa(tempaddr));
	} catch(...)
	{
		if(cipherbuffer != NULL)
			delete cipherbuffer;

		if(plaintext != NULL)
			delete plaintext;

		std::cerr << "Decryption error." << std::endl;
		return false;
	} 

	return true;
}