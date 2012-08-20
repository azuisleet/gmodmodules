#include "gm_transmittools.h"

#undef GetObject

// blah, don't know the proper way to hash this
struct PacketHash : public std::unary_function<PlayerVector, size_t> {
    size_t operator()(const PlayerVector& f) const {
		return std::tr1::hash<int>()(f._Getword(0));
    }
};

struct ItemCount
{
	unsigned char count;
	int offset;
};

typedef std::tr1::unordered_map<EntInfo *, ItemCount> entitemcount;
struct Packet
{
	bf_write write;
	entitemcount itemcount;
};

typedef std::tr1::unordered_map<PlayerVector, Packet, PacketHash> packetmap;
packetmap Packets;

class CRecipientFromBitset : public IRecipientFilter
{
public:
	CRecipientFromBitset(const PlayerVector &bit) : sendbits(bit), count(bit.count()) {};
	virtual ~CRecipientFromBitset() {};

	virtual bool	IsReliable( void ) const { return true; }
	virtual bool	IsInitMessage( void ) const	{ return false;	}

	virtual int		GetRecipientCount( void ) const { return count; }
	virtual int		GetRecipientIndex( int slot ) const
	{
		if ( slot < 0 || slot >= GetRecipientCount() )
			return -1;

		for(unsigned int i = 0; i < sendbits.size(); ++i)
		{
			if(sendbits[i] == true && slot-- == 0)
			{
				return i+1;
			}
		}

		return -1;
	}

private:
	const PlayerVector &sendbits;
	int count;
};

#define NWVAR_BYTES_PER_TICK 64

bool NWTryPack(EntInfo *ent, const ValueInfo &value, char &bytes, Packet &packet)
{
	std::pair<entitemcount::iterator, bool> countiter = packet.itemcount.insert(entitemcount::value_type(ent, ItemCount()));

	ItemCount &count = countiter.first->second;

	// added this entity to the packet best case is 4 ent + 1 num + 2 best case (7)
	if(countiter.second == true)
	{
		// we can't fit the ent and a best case
		if(bytes < 7)
		{
			packet.itemcount.erase(countiter.first);
			return false;
		}

		packet.write.WriteLong(ResolveEHandleForEntity(ent->entindex));

		count.offset = packet.write.m_iCurBit;
		packet.write.WriteChar(0);
		bytes -= 5;
	}

	int size = 0;

	switch(value.type)
	{
	case NWTYPE_CHAR:
	case NWTYPE_BOOL:
		size = 1;
		break;
	case NWTYPE_SHORT:
		size = 2;
		break;
	case NWTYPE_FLOAT:
	case NWTYPE_NUMBER:
	case NWTYPE_ENTITY:
		size = 4;
		break;
	case NWTYPE_VECTOR:
	case NWTYPE_ANGLE:
		size = 12; // careful..
		break;
	}

	if(size > bytes)
		return false;

	g_pLua->PushReference(ent->entityvaluestable);
	ILuaObject *valuestable = g_pLua->GetObject();
	ILuaObject *luavalue = valuestable->GetMember(value.tableoffset);

	if(value.type == NWTYPE_STRING && luavalue != NULL && luavalue->isString() && luavalue->GetString() != NULL)
	{
		size = strlen(luavalue->GetString()) + 1;
		if(size > bytes)
			return false;
	}

	size++;
	bytes -= size;
	count.count++;

	// update the count
	//printf("new count %d\n", count.count);

	int curpos = packet.write.m_iCurBit;
	packet.write.SeekToBit(count.offset);
	packet.write.WriteChar(count.count);
	packet.write.SeekToBit(curpos);

	packet.write.WriteChar(value.tableoffset);
	switch(value.type)
	{
	case NWTYPE_CHAR:
		if(luavalue != NULL && luavalue->isNumber())
			packet.write.WriteChar(luavalue->GetInt());
		else
			packet.write.WriteChar(0);
		break;
	case NWTYPE_BOOL:
		if(luavalue != NULL)
			packet.write.WriteOneBit(luavalue->GetBool());
		else
			packet.write.WriteOneBit(false);
		break;
	case NWTYPE_SHORT:
		if(luavalue != NULL && luavalue->isNumber())
			packet.write.WriteShort(luavalue->GetInt());
		else
			packet.write.WriteShort(0);
		break;
	case NWTYPE_FLOAT:
		if(luavalue != NULL)
			packet.write.WriteFloat(luavalue->GetFloat());
		else
			packet.write.WriteFloat(0);
		break;
	case NWTYPE_NUMBER:
		if(luavalue != NULL && luavalue->isNumber())
			packet.write.WriteLong(luavalue->GetInt());
		else
			packet.write.WriteLong(0);
		break;
	case NWTYPE_ENTITY:
		// ehandle
		packet.write.WriteLong(ResolveEHandleForEntity(luavalue));
		break;
	case NWTYPE_VECTOR:
		if(luavalue != NULL)
		{
			Vector *vec = (Vector *)luavalue->GetUserData();
			if(vec != NULL)
			{
				packet.write.WriteBitVec3Coord(*vec);
				break;
			}
		}
		packet.write.WriteBitVec3Coord(Vector(0,0,0));
		break;
	case NWTYPE_ANGLE:
		// angle
		packet.write.WriteBitAngles(QAngle(1,1,1));
		break;
	case NWTYPE_STRING:
		packet.write.WriteOneBit(0); // fuck pooling
		if(luavalue != NULL && luavalue->isString() && luavalue->GetString() != NULL)
			packet.write.WriteString(luavalue->GetString());
		else
			packet.write.WriteString("");
		break;
	}

	return true;
}

// entity tick, returns if the entity is complete
bool NWTickEntity(EntInfo *ent, char &bytes)
{
	bool complete = true;

	for(ValueVector::iterator iter = ent->values.begin(); iter != ent->values.end(); ++iter)
	{
		ValueInfo &value = *iter;

		// this value is complete, move along
		if(value.currentTransmit == value.finalTransmit)
			continue;

		// xor the final and current to get the bits that need to be set in currentTransmit, which corresponds to the players
		PlayerVector sendvec = value.finalTransmit ^ value.currentTransmit;

		//printf("Ent %d value %d count %d\n", ent->entindex, value.tableoffset, sendvec.count());

		std::pair<packetmap::iterator, bool> packetiter = Packets.insert(packetmap::value_type(sendvec, Packet()));

		// a new packet is created, but we have < (2 umsg + 2 ent + 1 num + 2 best case) 7
		if(packetiter.second == true)
		{
			// we can't fit a new packet
			if(bytes < 7)
			{
				Packets.erase(packetiter.first);
				complete = false;
				continue;
			}

			Packet &packet = packetiter.first->second;
			packet.write.StartWriting(new char[NWVAR_BYTES_PER_TICK], NWVAR_BYTES_PER_TICK);

			bytes -= 2; // a umsg is created
		}

		if(! NWTryPack(ent, value, bytes, packetiter.first->second))
		{
			complete = false;
			continue;
		}

		value.currentTransmit = value.finalTransmit;
	}

	ent->complete = complete;
	return complete;
}

// the actual function to send the packets
void DispatchPackets()
{
	packetmap::iterator iter = Packets.begin();
	while(iter != Packets.end())
	{
		const PlayerVector &vec = iter->first;
		Packet &packet = iter->second;

		CRecipientFromBitset filter(vec);

		bf_write *bf_write = engine->UserMessageBegin(&filter, 34); // LuaUserMessage

		bf_write->WriteShort(umsgStringTableOffset);
		bf_write->WriteOneBit(0); // important bit!

		unsigned char *packetdata = packet.write.GetBasePointer(); 
		//printf("packet bytes written: %d\n", packet.write.GetNumBytesWritten());

		bf_write->WriteBits(packet.write.GetBasePointer(), packet.write.GetNumBitsWritten());
		engine->MessageEnd();

		delete packetdata;
		iter = Packets.erase(iter);
	}
}

// main processing loop that goes through IncompleteEntities
int NWTickAll(lua_State *)
{
	// double time?
	for(int i = 0; i < 2; i++) 
	{
		if(IncompleteEntities.size() == 0)
			return 0;

		//printf("Tick with %d incomplete\n", IncompleteEntities.size());
		char bytes = NWVAR_BYTES_PER_TICK;

		entinfovec::iterator iter = IncompleteEntities.begin();
		while(iter != IncompleteEntities.end())
		{
			if(NWTickEntity(*iter, bytes))
			{
				//printf("Tick shows end %d completed and transmitted\n", (*iter)->entindex);
				*iter = IncompleteEntities.back();
				IncompleteEntities.pop_back();
			} else {
				++iter;
			}

			// don't bother if we can't fit the best case
			if(bytes < 2)
				break;
		}

		DispatchPackets();
		//printf("Final bytes: %d -- %d\n", bytes, NWVAR_BYTES_PER_TICK - bytes);
	}
	return 0;
}


int NWUmsgTest(lua_State *)
{
	PlayerVector vec;
	vec[0] = true;

	CRecipientFromBitset filter(vec);

	for(int i = 25; i < 40; i++)
	{
		Msg( " %d \n ", i );

		bf_write *bf_write = engine->UserMessageBegin(&filter, i); // LuaUserMessage
		bf_write->WriteShort(umsgStringTableOffset); //umsgStringTableOffset);
		bf_write->WriteOneBit(0); // important bit!

		int pos = bf_write->m_iCurBit;
		bf_write->WriteChar(64);
		int opos = bf_write->m_iCurBit;
		bf_write->SeekToBit(pos);
		bf_write->WriteChar(63);
		bf_write->SeekToBit(opos);
	
		engine->MessageEnd();
	}

	return 0;
}