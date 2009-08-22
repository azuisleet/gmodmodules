typedef std::bitset<MAX_GMOD_PLAYERS> PlayerVector;

struct ValueInfo {
	PlayerVector finalTransmit; // final transmit, only players it knows
	PlayerVector currentTransmit; // currently transmitted
	NWTypes type; // value type
	NWRepl repl; // value repl
	int tableoffset; // offset of said table
	int valueentindex; // special case for nwtype_entity so we don't have to call into lua
};

typedef	std::vector<ValueInfo> ValueVector;

struct EntInfo {
	bool complete; // whether or not the currentTransmits match finalTransmits

	PlayerVector ReplTransmit[NUM_NWREPLS]; // reference for the final transmits

	PlayerVector ultimateTransmit; // and of all the repls want used by the values
	std::bitset<NUM_NWREPLS> replsUsed; // these are the repls being used

	ValueVector values; // array of values

	int entindex; // ent index
	int entityvaluestable; // reference to table holding values
};

typedef std::tr1::unordered_map<int, EntInfo *> netents;
typedef std::vector<int> actpl;
typedef std::vector<EntInfo *> entinfovec;
typedef std::tr1::unordered_multimap<int, EntInfo *> nwdepend;

extern netents NetworkedEntities;
extern actpl ActivePlayers;
extern entinfovec IncompleteEntities;
extern nwdepend NWDependencies;

// created a networked entity
void NetworkedEntityCreated(int entindex, int tableref);
void NetworkedEntityCreatedFinish(int entindex);
// call before NetworkedEntityCreated
void PlayerCreated(int player);

// destroyed a player
void PlayerDestroyed(int player);
// _any_ entity is destroyed
void EntityDestroyed(int entindex);

// CheckTransmit calls this, handles repl and values
void EntityTransmittedToPlayer(int entindex, int playerindex);

// crash
void InvalidatePlayerCrashed(int playerindex);

// Add a value to the table
void AddValueToEntityValues(int entindex, int offset, int type, int repl);

// Called when a variable is updated
void InvalidateEntityVariableUpdate(int entindex, unsigned int offset);
// Special call for nwtype_entity, to set the valueentindex of a value
void EntitySetValueEntIndex(int entindex, unsigned int offset, int newentindex);