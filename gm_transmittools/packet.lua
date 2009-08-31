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

	local ent = um:ReadShort()

	while ent != 0 do
		if ent == -1 then ent = 0 end
		local entity = Entity(ent)

		if IsValid(entity) || entity == GetWorldEntity() then

			local nwtable = entity.__nwtable
			if !nwtable then return end

			local items = um:ReadChar()

			for i=1, items do
				local index = um:ReadChar()
				local tbl = nwtable[index]
				if !tbl then
					ErrorNoHalt("Missing table for entity " .. tostring(entity) .. " at index " .. tostring(index))
					return
				end

				if NWDEBUG then
					print(entity, tbl.type, Readers[tbl.type])
				end
				local value = um[Readers[tbl.type]](um)

				if tbl.proxy then
					pcall(tbl.proxy, entity, tbl.name, entity[tbl.name], value)
				end

				if NWDEBUG then
					print(ent, entity, tbl.name, value)
				end

				entity[tbl.name] = value
			end

		else
			if NWDEBUG then
				print("Invalid entity ", entity, ent)
			end
			return
		end

		ent = um:ReadShort()
	end
end

usermessage.Hook("N", ParsePacket)