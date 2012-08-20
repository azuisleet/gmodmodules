local Readers = {
	[NWTYPE_CHAR  ] = "ReadChar",
	[NWTYPE_BOOL  ] = "ReadBool",
	[NWTYPE_SHORT ] = "ReadShort",
	[NWTYPE_ENTITY] = "ReadEntity",
	[NWTYPE_STRING] = "ReadString",
	[NWTYPE_NUMBER] = "ReadLong",
	[NWTYPE_FLOAT ] = "ReadFloat",
	[NWTYPE_VECTOR] = "ReadVector"
}

function ParsePacket(um)
	if NWDEBUG then
		print("parsing packet")
	end

	local entity = um:ReadEntity()
	
	while entity:IsValid() do
		local nwtable = entity.__nwtable
		if !nwtable then return end

		local ent_index = entity:EntIndex()
		
		if not NW_ENTITY_DATA[ent_index].__entity:IsValid() then
			if NWDEBUG then print("packet: we need to re-create ", entity) end
			NW_ENTITY_DATA[ent_index] = {}
			NW_ENTITY_DATA[ent_index].__entity = entity
		end
		
		local items = um:ReadChar()

		for i=1, items do
			local index = um:ReadChar()
			local tbl = nwtable[index]
			if !tbl then
				// can this happen? do we need to defer?
				ErrorNoHalt("Missing table for entity " .. tostring(entity) .. " at index " .. tostring(index))
				return
			end

			if NWDEBUG then
				print(entity, tbl.type, Readers[tbl.type])
			end
			local value = um[Readers[tbl.type]](um)

			if tbl.proxy then
				local b, err = pcall(tbl.proxy, entity, tbl.name, NW_ENTITY_DATA[ent_index][tbl.name], value)
				
				if !b then
					ErrorNoHalt( err )
				end
			end

			if NWDEBUG then
				print(entity, tbl.name, value)
			end

			NW_ENTITY_DATA[ent_index][tbl.name] = value
		end

		entity = um:ReadEntity()
	end
end

usermessage.Hook("N", ParsePacket)