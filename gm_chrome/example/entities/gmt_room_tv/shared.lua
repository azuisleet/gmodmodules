ENT.Base		= "browser_base"
ENT.Type		= "anim"
ENT.PrintName		= "Television"
ENT.Contact		= ""
ENT.Purpose		= "For GMod Tower"
ENT.Instructions	= ""
ENT.Spawnable		= false
ENT.AdminSpawnable	= false

ENT.Model		= "models/gmod_tower/suitetv.mdl"

GtowerPrecacheModel( ENT.Model )

function ENT:SharedInit()
	RegisterNWTable(self, { 
		{"Power", false, NWTYPE_BOOLEAN, REPL_EVERYONE, self.Powered }, 
	})
end