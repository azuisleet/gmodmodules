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

	ItemCount() : count(0), offset(0) {}
	ItemCount(const ItemCount& other) : count(other.count), offset(other.offset) {}
};

typedef std::tr1::unordered_map<EntInfo *, ItemCount> entitemcount;
struct Packet
{
	bf_write write;
	entitemcount itemcount;

	Packet() : write(), itemcount() {}
	Packet(const Packet& other) : write(), itemcount(other.itemcount)
	{
		write.m_pData = other.write.m_pData;
		write.m_nDataBytes = other.write.m_nDataBytes;
		write.m_nDataBits = other.write.m_nDataBits;
		write.m_iCurBit = other.write.m_iCurBit;
	}
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

bool NWTryPack(EntInfo *ent, const ValueInfo &value, int &bits, Packet &packet, ItemCount &count)
{
	int size = 0;

	switch(value.type)
	{
	case NWTYPE_BOOL:
		size = 1;
		break;
	case NWTYPE_CHAR:
		size = 8;
		break;
	case NWTYPE_SHORT:
		size = 16;
		break;
	case NWTYPE_FLOAT:
	case NWTYPE_NUMBER:
		size = 32;
		break;
	case NWTYPE_ENTITY:
		size = 32; //MAX_EDICT_BITS;
		break;
	case NWTYPE_VECTOR:
	case NWTYPE_ANGLE:
		size = 69;
		break;
	}

	if((size+8) > bits)
		return false;

	g_pLua->PushReference(ent->entityvaluestable);
	ILuaObject *valuestable = g_pLua->GetObject();
	ILuaObject *luavalue = valuestable->GetMember(value.tableoffset);
	valuestable->UnReference();

	if(value.type == NWTYPE_STRING && luavalue != NULL && luavalue->isString() && luavalue->GetString() != NULL)
	{
		size = ((strlen(luavalue->GetString()) + 1) << 3) + 1;
		if((size+8) > bits)
		{
			luavalue->UnReference();
			return false;
		}
	}

	size += 8;
	bits -= size;
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

	if(luavalue != NULL)
		luavalue->UnReference();
	return true;
}

bool NWTryPack(EntInfo *ent, const ValueInfo &value, int &bits, Packet &packet)
{
	ItemCount count;
	entitemcount::iterator iter = packet.itemcount.find(ent);

	// added this entity to the packet best case is 16 ent + 8 num + 16 best case (40)
	if(iter == packet.itemcount.end())
	{
		// we can't fit the ent and a best case
		if(bits < 40)
		{
			return false;
		}

		if(ent->entindex == 0)
			packet.write.WriteShort(-1);
		else
			packet.write.WriteShort(ent->entindex);

		count.offset = packet.write.m_iCurBit;
		count.count = 0;
		packet.write.WriteChar(0);
		bits -= 24;

		bool success = NWTryPack(ent, value, bits, packet, count);
		packet.itemcount.insert(entitemcount::value_type(ent, count));
		return success;
	}
	else
	{
		return NWTryPack(ent, value, bits, packet, iter->second);
	}

	return false;
}

// entity tick, returns if the entity is complete
bool NWTickEntity(EntInfo *ent, int &bits, ValueInfo &value, Packet &packet)
{
	if(! NWTryPack(ent, value, bits, packet))
	{
		return false;
	}

	value.currentTransmit = value.finalTransmit;
	return true;
}

bool NWTickEntity(EntInfo *ent, int &bits)
{
	bool complete = true;

	for(ValueVector::iterator iter = ent->values.begin(); iter != ent->values.end(); ++iter)
	{
		Packet packet;
		ValueInfo &value = *iter;

		// this value is complete, move along
		if(value.currentTransmit == value.finalTransmit)
			continue;

		// xor the final and current to get the bits that need to be set in currentTransmit, which corresponds to the players
		PlayerVector sendvec = value.finalTransmit ^ value.currentTransmit;
		packetmap::iterator packetiter = Packets.find(sendvec);

		//best case is 16 ent + 8 num + 16 best case (40)
		// a new packet is created, but we have < (16 umsg + 16 ent + 8 num + 16 best case) 56
		if(packetiter == Packets.end())
		{
			// we can't fit a new packet
			if(bits < 56)
			{
				complete = false;
				continue;
			}

			packet.write.StartWriting(new char[NWVAR_BYTES_PER_TICK], NWVAR_BYTES_PER_TICK);
			bits -= 16; // a umsg is created

			if(!NWTickEntity(ent, bits, value, packet))
				complete = false;

			Packets.insert(packetmap::value_type(sendvec, packet));
		}
		else
		{
			if(!NWTickEntity(ent, bits, value, packetiter->second))
				complete = false;
		}
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

#ifdef GMOD13
		bf_write *bf_write = engine->UserMessageBegin(&filter, 39); // LuaUserMessage
		bf_write->WriteString("N");
#else
		bf_write *bf_write = engine->UserMessageBegin(&filter, 34); // LuaUserMessage

		bf_write->WriteShort(umsgStringTableOffset);
		bf_write->WriteOneBit(0); // important bit!
#endif

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
		int bits = NWVAR_BYTES_PER_TICK << 3;

		entinfovec::const_iterator iter = IncompleteEntities.begin();
		while(iter != IncompleteEntities.end())
		{
			if(NWTickEntity(*iter, bits))
			{
				//printf("Tick shows end %d completed and transmitted\n", (*iter)->entindex);
				iter = IncompleteEntities.erase(iter);
			} else {
				++iter;
			}

			// don't bother if we can't fit the best case
			if(bits < 16)
				break;
		}

		DispatchPackets();
		//printf("Final bits: %d -- %d\n", bits, (NWVAR_BYTES_PER_TICK << 3) - bits);
	}
	return 0;
}


int NWUmsgTest(lua_State *)
{
	PlayerVector vec;
	vec[0] = true;

	CRecipientFromBitset filter(vec);

	for(int i = 25; i < 60; i++)
	{
		Msg( " %d \n ", i );

		bf_write *bf_write = engine->UserMessageBegin(&filter, 39); // LuaUserMessage
		bf_write->WriteString("N"); //umsgStringTableOffset);

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