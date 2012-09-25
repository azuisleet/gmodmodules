typedef std::bitset<MAX_GMOD_PLAYERS> PlayerVector;

struct ValueInfo {
	PlayerVector finalTransmit; // final transmit, only players it knows
	PlayerVector currentTransmit; // currently transmitted
	NWTypes type; // value type
	NWRepl repl; // value repl
	int tableoffset; // offset of said table
	int valueentindex; // special case for nwtype_entity so we don't have to call into lua

	ValueInfo() : finalTransmit(), currentTransmit(), type(NUM_NWTYPES), repl(NUM_NWREPLS), tableoffset(0), valueentindex(0) {}
	ValueInfo(const ValueInfo& other) : finalTransmit(other.finalTransmit), currentTransmit(other.currentTransmit), type(other.type), repl(other.repl), tableoffset(other.tableoffset), valueentindex(other.valueentindex) {}
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

	EntInfo() : complete(false), ReplTransmit(), ultimateTransmit(), replsUsed(), values(), entindex(), entityvaluestable() {}
	EntInfo(const EntInfo& other) : complete(other.complete), ReplTransmit(), ultimateTransmit(other.ultimateTransmit), 
		replsUsed(other.replsUsed), values(other.values), entindex(other.entindex), entityvaluestable(other.entityvaluestable)
	{
		for(int i=0; i < NUM_NWREPLS; i++)
		{
			ReplTransmit[i] = other.ReplTransmit[i];
		}
	}
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
int EntityDestroyed(int entindex);

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