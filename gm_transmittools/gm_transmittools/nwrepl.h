struct EntInfo;

enum NWRepl
{
	NWREPL_EVERYONE,
	NWREPL_PLAYERONLY,

	NUM_NWREPLS
};

bool ShouldReplTransmit(int repl, EntInfo *ent, int player);