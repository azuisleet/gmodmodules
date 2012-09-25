#include "gm_transmittools.h"

entinfovec IncompleteEntities;
netents NetworkedEntities;
actpl ActivePlayers;

nwdepend NWDependencies; // an entindex that points to a nwentity that should be updated!

void UpdateReplTransmitsForPlayer(EntInfo *ent, int playerindex);
void UpdateEntityValuesTransmitsForPlayer(EntInfo *ent, int playerindex, bool remove=false);
void InvalidatePlayerForAllNetworkedGone(int playerindex);
void InvalidatePlayerForAllNetworked(int playerindex);
void InvalidateEntityForAllPlayers(int entindex);

void UpdateCompleteState(EntInfo *ent, bool newcomplete)
{
	//printf("Entity %d complete state set to %d\n", ent->entindex, newcomplete);

	if(ent->complete && !newcomplete)
	{
		IncompleteEntities.push_back(ent);
	} else if(!ent->complete && newcomplete) {
		for(entinfovec::const_iterator it = IncompleteEntities.begin(); it != IncompleteEntities.end(); ++it)
		{
			if( *it == ent )
			{
				it = IncompleteEntities.erase(it);
				break;
			}
		}
	}

	ent->complete = newcomplete;
}

void NetworkedEntityCreated(int entindex, int tableref)
{
	//printf("Entity created %d\n", entindex);
	EntInfo *ent = new EntInfo();
	ent->complete = true;
	ent->entindex = entindex;
	ent->entityvaluestable = tableref;

	entityParity[entindex].flip();

	netents::const_iterator iter = NetworkedEntities.find(entindex);
	if(iter != NetworkedEntities.end())
	{
		return;
	}

	NetworkedEntities.insert(netents::value_type(entindex, ent));	
}

// add values after created then finish
void NetworkedEntityCreatedFinish(int entindex)
{
	InvalidateEntityForAllPlayers(entindex);
}

// call before NetworkedEntityCreated
void PlayerCreated(int player)
{
	//printf("Player created %d\n", player);
	ActivePlayers.push_back(player);
	InvalidatePlayerForAllNetworked(player);
}

// call before EntityDestroyed
void PlayerDestroyed(int player)
{
	//printf("Player destroyed %d\n", player);
	sentEnts[player-1].reset();

	for(actpl::const_iterator it = ActivePlayers.begin(); it != ActivePlayers.end(); ++it)
	{
		if( *it == player )
		{
			it = ActivePlayers.erase(it);
			break;
		}
	}

	InvalidatePlayerForAllNetworkedGone(player);
}

int EntityDestroyed(int entindex)
{
	if(entindex == 0)
		return -1;

	//printf("Entity destroyed %d\n", entindex);
	NWDependencies.erase(entindex);

	for(actpl::const_iterator iter = ActivePlayers.begin(); iter != ActivePlayers.end(); ++iter)
	{
		int player = *iter;
		sentEnts[player-1].reset(entindex);
	}

	netents::const_iterator iter = NetworkedEntities.find(entindex);

	// this entity isn't networked, we're done
	if(iter == NetworkedEntities.end())
		return -1;

	EntInfo *ent = iter->second;

	//printf("Networked entity destroyed %d\n", entindex);

	// clear any remaining dependencies based on bound values
	for(ValueVector::const_iterator iter = ent->values.begin(); iter != ent->values.end(); ++iter)
	{
		if(iter->type != NWTYPE_ENTITY || iter->valueentindex < 0)
			continue;

		std::pair<nwdepend::const_iterator, nwdepend::const_iterator> result;
		result = NWDependencies.equal_range(iter->valueentindex);
		for(nwdepend::const_iterator it = result.first; it != result.second; ++it)
		{
			if( it->second == ent )
			{
				NWDependencies.erase(it);
				break;
			}
		}
	}

	for(entinfovec::const_iterator it = IncompleteEntities.begin(); it != IncompleteEntities.end(); ++it)
	{
		if( *it == ent )
		{
			it = IncompleteEntities.erase(it);
			break;
		}
	}

	int reference = ent->entityvaluestable;

	//finally:
	NetworkedEntities.erase(iter);
	delete ent;

	return reference;
}

void EntityTransmittedToPlayer(int entindex, int playerindex)
{
	//printf("Entity %d transmitted to play %d\n", entindex, playerindex);

	// find dependencies
	std::pair<nwdepend::const_iterator, nwdepend::const_iterator> result;
	result = NWDependencies.equal_range(entindex);

	for(nwdepend::const_iterator iter = result.first; iter != result.second; ++iter)
	{
		//printf("Entity %d depends on %d being transmitted, updating for player %d\n", iter->second->entindex, entindex, playerindex);
		UpdateEntityValuesTransmitsForPlayer(iter->second, playerindex);
	}

	netents::const_iterator iter = NetworkedEntities.find(entindex);

	// this entity isn't networked
	if(iter == NetworkedEntities.end())
		return;

	EntInfo *ent = iter->second;

	// none of the values want the player
	if( !ent->ultimateTransmit[playerindex-1] )
		return;

	//printf("Entity %d is being updated for player %d\n", ent->entindex, playerindex);
	// at this point the entity wants to know about this player
	// finalTransmit will only know about players it's sent to, and currentTransmit only cares about who knows what
	UpdateReplTransmitsForPlayer(ent, playerindex);
	UpdateEntityValuesTransmitsForPlayer(ent, playerindex);
}

void UpdateReplTransmitsForPlayer(EntInfo *ent, int playerindex)
{
	PlayerVector &ultimate = ent->ultimateTransmit;

	for(int i=NWREPL_EVERYONE; i < NUM_NWREPLS; i++)
	{
		if(ent->replsUsed[i] && ShouldReplTransmit(i, ent, playerindex))
		{
			//printf("Adding player %d to the ultimate list for entity %d\n", playerindex, ent->entindex);
			// if we're transmitting, set them in the ultimate, but only set them in the repl transmit if we've seen them
			ultimate[playerindex-1] = true;

			if(sentEnts[playerindex-1][ent->entindex])
			{
				//printf("Adding player %d to the repl %d list for entity %d\n", playerindex, i, ent->entindex);
				PlayerVector &vec = ent->ReplTransmit[i];
				vec[playerindex-1] = true;
				//printf("Repl %i looks like %s\n", i, vec.to_string().c_str());
			}
		}
	}
}

void UpdateEntityValuesTransmitsForPlayer(EntInfo *ent, int playerindex, bool remove)
{
	ValueVector::iterator iter;
	bool complete = true;

	for(iter = ent->values.begin(); iter != ent->values.end(); ++iter)
	{
		ValueInfo &value = *iter;
		PlayerVector &repl = ent->ReplTransmit[value.repl];

		// this way we can simply call UpdateEntitiyValuesTransmit on a dependent to add them
		if(value.type == NWTYPE_ENTITY)
		{
			// make sure this player is someone we want, and that they have seen it, otherwise we'll be notified later
			if(repl[playerindex-1] && (value.valueentindex < 0 || sentEnts[playerindex-1][value.valueentindex]))
				value.finalTransmit[playerindex-1] = true;
		} else {
			value.finalTransmit = repl;
		}

		// don't let there be any currentTransmit bits that aren't in the finalTransmit
		value.currentTransmit &= value.finalTransmit;

		// remove this player from the currenttransmit, crash case
		if(remove)
			value.currentTransmit[playerindex-1] = false;

		//printf("ValueInfo index %d looks like %s out of %s\n", value.tableoffset, value.currentTransmit.to_string().c_str(), value.finalTransmit.to_string().c_str());

		if(value.currentTransmit != value.finalTransmit)
			complete = false;
	}

	UpdateCompleteState(ent, complete);
}

// the player is gone, remove them!
void HandleRemovePlayerFromRepls(EntInfo *ent, int playerindex)
{
	PlayerVector &ultimate = ent->ultimateTransmit;
	ultimate[playerindex-1] = false;

	for(int i=NWREPL_EVERYONE; i < NUM_NWREPLS; i++)
	{
		if(ent->replsUsed[i])
		{
				PlayerVector &vec = ent->ReplTransmit[i];
				vec[playerindex-1] = false;
		}
	}

	bool complete = true;
	for(ValueVector::iterator iter = ent->values.begin(); iter != ent->values.end(); ++iter)
	{
		ValueInfo &value = *iter;

		value.finalTransmit[playerindex-1] = false; // don't destroy the finalTransmit, especially for nwtype_entity
		value.currentTransmit[playerindex-1] = false; // clear bit

		if(value.currentTransmit != value.finalTransmit)
			complete = false;
	}

	UpdateCompleteState(ent, complete);
}

// a player leaves, remove them from the ultimate and final masks of all entities
void InvalidatePlayerForAllNetworkedGone(int playerindex)
{
	netents::const_iterator iter;
	for(iter = NetworkedEntities.begin(); iter != NetworkedEntities.end(); ++iter)
	{
		EntInfo *ent = iter->second;
		HandleRemovePlayerFromRepls(ent, playerindex);
	}
}

// only need to update repls, transmit will update values
// Called when a player joins, to build repls, don't need to build values
void InvalidatePlayerForAllNetworked(int playerindex)
{
	netents::const_iterator iter;
	for(iter = NetworkedEntities.begin(); iter != NetworkedEntities.end(); ++iter)
	{
		EntInfo *ent = iter->second;
		UpdateReplTransmitsForPlayer(ent, playerindex);
	}
}

// player crashed, only clear them from entities they've seen and the currentTransmit
void InvalidatePlayerCrashed(int playerindex)
{
	netents::const_iterator iter;
	for(iter = NetworkedEntities.begin(); iter != NetworkedEntities.end(); ++iter)
	{
		EntInfo *ent = iter->second;
		if(sentEnts[playerindex-1][ent->entindex] && ent->ultimateTransmit[playerindex-1])
			UpdateEntityValuesTransmitsForPlayer(ent, playerindex, true);
	}
}


// Called when an entity is created, to build repls for all players for this entity
void InvalidateEntityForAllPlayers(int entindex)
{
	netents::const_iterator entiter = NetworkedEntities.find(entindex);
	if(entiter == NetworkedEntities.end())
	{
		//printf("Tried to invalidate non-networked entityforallplayers %d\n", entindex);
		return;
	}
	EntInfo *ent = entiter->second;

	actpl::const_iterator iter;
	for(iter = ActivePlayers.begin(); iter != ActivePlayers.end(); ++iter)
	{
		UpdateReplTransmitsForPlayer(ent, *iter);
	}
}

void AddValueToEntityValues(int entindex, int offset, int type, int repl)
{
	netents::const_iterator entiter = NetworkedEntities.find(entindex);
	if(entiter == NetworkedEntities.end())
	{
		//printf("Tried to add value to non-networked entityvariableupdate %d\n", entindex);
		return;
	}
	EntInfo *ent = entiter->second;

	ent->replsUsed[repl] = true;

	ValueInfo value;
	value.currentTransmit.reset();
	value.finalTransmit.reset();
	value.type = (NWTypes)type;
	value.repl = (NWRepl)repl;
	value.tableoffset = offset;
	value.valueentindex = -1;
	ent->values.push_back(value);

	//printf("Added value offset %d type %d repl %d to entity %d\n", offset, type, repl, entindex);
}

// if a nwtype_entity was changed we need to update var for all players because they may know about the entity, this should be called after the valueentindex is set
void InvalidateEntityVariableUpdate(int entindex, unsigned int offset)
{
	netents::const_iterator entiter = NetworkedEntities.find(entindex);
	if(entiter == NetworkedEntities.end())
	{
		//printf("Tried to invalidate non-networked entityvariableupdate %d\n", entindex);
		return;
	}
	EntInfo *ent = entiter->second;

	if(offset >= ent->values.size())
	{
		//printf("Offset %d is too large for entity %d\n", offset, entindex);
		return;
	}

	ValueInfo &value = ent->values[offset];

	value.currentTransmit.reset();

	// the only time we have to do something special for nwtype_entity is EntityTransmit and when we set a new entity
	if(value.type == NWTYPE_ENTITY)
	{
		PlayerVector &repl = ent->ReplTransmit[value.repl];

		actpl::const_iterator iter;
		for(iter = ActivePlayers.begin(); iter != ActivePlayers.end(); ++iter)
		{
			int player = *iter;
			if(repl[player-1] && (value.valueentindex < 0 || sentEnts[player-1][value.valueentindex]))
				value.finalTransmit[player-1] = true;
		}
	}

	if(ent->complete && value.currentTransmit != value.finalTransmit)
	{
		//printf("Invalidated value %d of entity %d, added to incomplete\n", offset, entindex);
		ent->complete = false;
		IncompleteEntities.push_back(ent);
	}
}

// special valueentindex for faster nwtype_entity checking
void EntitySetValueEntIndex(int entindex, unsigned int offset, int newentindex)
{
	netents::const_iterator entiter = NetworkedEntities.find(entindex);
	if(entiter == NetworkedEntities.end())
	{
		//printf("Tried setvalueentindex for non-networked entity %d\n", entindex);
		return;
	}
	EntInfo *ent = entiter->second;

	if(offset >= ent->values.size())
	{
		//printf("Offset %d is too large for entity %d\n", offset, entindex);
		return;
	}

	ValueInfo &value = ent->values[offset];

	int oldentindex = value.valueentindex;

	std::pair<nwdepend::const_iterator, nwdepend::const_iterator> result;
	result = NWDependencies.equal_range(oldentindex);
	for(nwdepend::const_iterator it = result.first; it != result.second; ++it)
	{
		if( it->second == ent )
		{
			NWDependencies.erase(it);
			break;
		}
	}

	value.valueentindex = newentindex;
	value.finalTransmit.reset(); // the finalTransmit isn't valid now, it will be rebuilt in invalidate

	if(newentindex >= 0)
	{
		NWDependencies.insert(nwdepend::value_type(newentindex, ent));
		//printf("Added entity dependency. %d depends on %d being transmitted.\n", ent->entindex, newentindex);
	}
}