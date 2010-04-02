bool Validation_Load();

byte *CreateRejection(const char *reason, int *size);
bool ValidateKPacket(byte *buffer, int length, in_addr from);