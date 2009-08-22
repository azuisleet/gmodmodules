#include "gm_transmittools.h"

bool ShouldReplTransmit(int repl, EntInfo *ent, int player)
{
	switch(repl)
	{
	default:
	case NWREPL_EVERYONE:
		return true;
		break;
	case NWREPL_PLAYERONLY:
		return ent->entindex == player /*|| ResolveEntInfoOwner(ent) == player*/;
		break;
	}
}