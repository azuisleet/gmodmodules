byte *CreateRejection( const char *reason, int *size );
bool ValidateKPacket( byte *buffer, int length, in_addr from, char **out_name, char **out_password );