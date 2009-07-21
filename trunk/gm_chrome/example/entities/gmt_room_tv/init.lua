AddCSLuaFile( "cl_init.lua" )
AddCSLuaFile( "shared.lua" )
include( "shared.lua" )

function ENT:Initialize()

	self.Entity:SetModel( self.Model )

	self.Entity:PhysicsInit( SOLID_VPHYSICS )
	self.Entity:SetMoveType( MOVETYPE_NONE )
	self.Entity:SetCollisionGroup( 0 )
	self.Entity:DrawShadow( false )
	
	local phys = self:GetPhysicsObject()
	
	if IsValid( phys ) then
		phys:EnableMotion( false )
	end

	self:LoadRoom()

	self:SharedInit()

	self.NextUse = 0
	self.AchiNextThink = 0.0
end

function ENT:Think()
	if !self.Power then return end
	if self.AchiNextThink > CurTime() then return end
	
	self.AchiNextThink = CurTime() + 1.0
	

	local Room = GtowerRooms:GetRoom( self.RoomId )
	
	if !Room then return end
	
	local Players = GtowerRooms:PlayersInRoom( Room )
	
	for _, ply in pairs( Players ) do
		ply:AddAchivement( ACHIVEMENTS.SUITEYOUTUBE, 1/60 )
	end
	
end 

function ENT:LoadRoom()
	self.RoomId = GtowerRooms:ClosestRoom( self:GetPos() )
end

function ENT:Use( ply )
	//if !ply:IsPlayer() then return end
	//if self:GetRoomOwner() != ply then return end
	
	if CurTime() < self.NextUse then return end
	self.NextUse = CurTime() + 0.2

	self.Power = !self.Power
end

function ENT:RemoteClick()
	self.Power = !self.Power
end

function ENT:GetRoomOwner()
    return GtowerRooms:RoomOwner( self.RoomId )
end


function ENT:RoomUnload( room )

end