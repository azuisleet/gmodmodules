#include "plugin.h"
#include "tier1/bitbuf.h"

byte *CreateRejection( const char *reason, int *size )
{
	*size = strlen( reason ) + 1 + 4 + 1;
	byte *buff = new byte[ *size ];

	bf_write buf;
	buf.StartWriting( buff, *size );

	buf.WriteLong( -1 );
	buf.WriteByte( '9' );
	buf.WriteString( reason );

	return buff;
}

bool ValidateKPacket( byte *buffer, int length, in_addr from, char **out_name, char **out_password )
{
	Msg( "Validating app ticket..\n" );

	if ( length <= 17 )
	{
		Msg("Impossible K packet length\n");
		return false;
	}

	bf_read buf;
	buf.StartReading( buffer, length );

	int32 channel = buf.ReadLong();
	byte type = buf.ReadByte();
	int32 protover = buf.ReadLong();
	int32 authprotover = buf.ReadLong();
	int32 challenge = buf.ReadLong();

	if ( protover != 16 ||
			authprotover != 3 )
	{
		// 15 is current engine protocol
		// 3 is steam auth protocol
		Msg( "Protocol mismatch\n" );
		return false;
	}

	if( buf.GetNumBytesLeft() == 0 )
	{
		Msg("Packet length too short 1\n");
		return false;
	}

	char name[128];
	if( !buf.ReadString( name, sizeof( name ) ) )
	{
		Msg( "Could not read name\n" );
		return false;
	}

	*out_name = name;

	if( buf.GetNumBytesLeft() == 0 )
	{
		Msg( "Packet length too short 2\n" );
		return false;
	}

	char password[32];
	if( !buf.ReadString( password, sizeof( password ) ) )
	{
		Msg( "Could not read password\n" );
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

	return true;
}