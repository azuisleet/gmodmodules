require("bass")

include('shared.lua')

SetupBrowserMat(ENT, "chrome/radio", 720, 480, 0.075)

local zerovec = Vector(0,0,0)
local MaxNameLength = 32

hook.Add("CanMousePress", "DisableRadio", function()
	
	for _, v in pairs( ents.FindByClass( "gmt_radio" ) ) do
		if v.Browser && v.MouseRayInteresct && v:MouseRayInteresct() then
			return false
		end
	end
	
end )

usermessage.Hook("ShowRadioSelect", function(umsg)
	local ent = umsg:ReadEntity()
	
	if !BASS || !chrome then
		GtowerMessages:AddNewItem( GetTranslation("PleaseFullGtowerPack") )
		return
	end

	if ent.Browser then
		ent.DrawTranslucent = ent.BrowserDraw
		return
	end

	ent:InitBrowser(ent:GetTable())
	ent.Browser:LoadURL("http://www.gmodtower.org/gmtonline/radio/")
	ent.DrawTranslucent = ent.BrowserDraw
	ent.CursorOut = 0
end)

local GreenBox	= Color(0, 255, 0, 50)
local RedBox	= Color(255, 0, 0, 50)

ENT.RenderGroup = RENDERGROUP_BOTH

function ENT:OpeningURL(url)
	self:ForcePlaySelect()
	print("opening", url)

		return		
	end

	self:RemoveBrowser()
	self.DrawTranslucent = self.MessageDraw
end

function ENT:FinishLoading()
	self:ForcePlaySelect()
end

function ENT:OnEnterBrowser()
	self.CursorOut = 0
end

function ENT:OnLeaveBrowser()
	self.CursorOut = CurTime() + 2
end

function ENT:Initialize()
	self.BoxColor = RedBox
	self.DrawTranslucent = self.MessageDraw

	if BASS == nil then
		self.DrawMessage = GetTranslation("PleaseFullGtowerPack")
		return
	end

	self.DrawMessage = GetTranslation("RadioTurnedOff")
	self.CurStation = ""
	self.radiostream = nil
	
	self.NextCalcName = 0
	self.TextWidth, self.TextHeight = 150, 20
	self.CurName = ""
	self.ShouldDrawName = false
	
	self.TextOffset = 0
	self.TargetTextOffset = 0
	
	self:SetNetworkedVarProxy("CurChan", self.ChannelChanged )

	self:SetLargeBounds()
	self.CursorOut = 0
end

function ENT:StopRadio()
	if self.radiostream then
		self.radiostream:stop()
	end

	self.BoxColor = RedBox
	self.DrawMessage = GetTranslation("RadioTurnedOff")

	if !self.Browser then
		self.DrawTranslucent = self.MessageDraw
	end

	self.ShouldDrawName = false
	self.radiostream = nil
end

function ENT:CalculateEarShot()
	if !GTowerLocation then
		self.InEarShot = true
		return
	end

	local radioloc = GTowerLocation:FindPlacePos(self:GetPos())
	local plyloc = GTowerLocation:FindPlacePos(LocalPlayer():GetPos())
	
	self.InEarShot = radioloc != nil && plyloc != nil && (plyloc == radioloc)
end

function ENT:StartRadio( stream )
	if !stream || #stream == 0 then return end

	self.BoxColor = GreenBox
	self.DrawTranslucent = self.RadioDraw
	
	self:RemoveBrowser()

	BASS.StreamFileURL(stream, 0, function(basschannel, error)
		if self.DrawTranslucent != self.RadioDraw then
			return // this means we stopped it between the startradio and this callback
		end

		self:CalculateEarShot()

		print("Ready to play", self.InEarShot)
		if !self.InEarShot then
			self:StopRadio()
			return
		end

		if !basschannel then
			print("Error streaming file", error)
			if error == 40 || error == 2 then
				self.DrawMessage = GetTranslation("RadioTimeout")
			elseif error == 41 then
				self.DrawMessage = GetTranslation("RadioUnsupportedFormat")
			elseif error == 8 then
				self.DrawMessage = GetTranslation("RadioBASSInitError")
			else
				self.DrawMessage = GetTranslation("RadioUnknown") .. " " .. tostring(error)
			end
			self.BoxColor = RedBox
			self.DrawTranslucent = self.MessageDraw

			return
		end

		self.radiostream = basschannel
		self.radiostream:set3dposition(self:GetPos(), zerovec, zerovec)

		self.radiostream:play()

		self.NextCalcName = 0
	end)
end

function ENT:ChannelChanged( name, oldval, nwc )
	
	if nwc == self.CurStation then return end
	
	self.CurStation = nwc
	
	if #nwc == 0 && self.StopRadio then
		self:StopRadio()
		return
	end
	
	if !self.StartRadio then return end

	print("station", self.Stream)

	self:CalculateEarShot()
	if !self.InEarShot then return end

	self:StartRadio(self.Stream)
end

function ENT:OnRemove( )
	self:RemoveBrowser()
	self:StopRadio()
end

function ENT:BaseDraw()
	local EntPos = self:GetPos() + ( self:GetForward() * 4 ) + self:GetUp() * -5
	local PlyDistance = EntPos:Distance( LocalPlayer():GetPos() )
	
	
	if PlyDistance > 350 then
		return
	end
	
	local ang = self:GetAngles()

	if (LocalPlayer():GetPos() - EntPos ):DotProduct( ang:Forward() ) < 0 then
		return
	end
	
	local AlphaRatio = 1.0
	
	if PlyDistance > 128 then
		AlphaRatio = 1 - (PlyDistance - 128) / 222
		
		self.BoxColor.a = AlphaRatio * 50
		
	else
		self.BoxColor.a = 50
	end
	
	ang:RotateAroundAxis(ang:Right(), 	-90 )
	ang:RotateAroundAxis(ang:Up(), 		90 )

	cam.Start3D2D( EntPos , ang, 0.1)
	
	return AlphaRatio
end

function ENT:MessageDraw()
	local AlphaRatio = self:BaseDraw()
	
	if AlphaRatio == nil then return end
	
	self:DrawMyText( self.DrawMessage, AlphaRatio )
	
	cam.End3D2D()
end


function ENT:Draw()
	self:DrawModel()
end

function ENT:GetPosBrowser()
	return self:GetPos() + (self:GetForward() * 4)
end

function ENT:DrawBrowser()
	surface.SetDrawColor(255, 255, 255, 255)
	surface.SetTexture(self.TexId)
	surface.DrawTexturedRect(0,0, self.TexWidth, self.TexHeight)

	if self.CursorOut > 0 then
		surface.SetDrawColor(100, 100, 100, 150)
		surface.DrawRect(0, 0, self.Width, self.Height)
	end
end

function ENT:BrowserDraw()
	if !self.Browser then
		self.DrawTranslucent = self.MessageDraw
		return
	end

	self:BaseBrowserDraw()
end

function ENT:RadioDraw()
	if self.radiostream && !self.radiostream:getplaying() then
		self:StopRadio()
		return
	end

	local AlphaRatio = self:BaseDraw()

	if AlphaRatio == nil then return end
	
	if self.radiostream && self.ShouldDrawName then
		self:DrawMyText( GetTranslation("RadioPlaying") .. ": " .. self.CurName, AlphaRatio )
	else
		self:DrawMyText( GetTranslation("RadioLoading"), AlphaRatio )
	end

	self:DrawSpectrumAnalyzer()

	cam.End3D2D()

end

function ENT:DrawMyText( text, AlphaRatio )

	surface.SetFont( "ChatFont" )

	local w,h = surface.GetTextSize( text ) 

	draw.RoundedBox(4, -100 , -145, w + 16, h + 8 , self.BoxColor )

	surface.SetTextColor( 255, 255, 255, 255 * AlphaRatio) 
	surface.SetTextPos( -100 + 8 , -145 + 4 ) 	
	surface.DrawText(text) 
	
end

local SPECHEIGHT= 64
local SPECWIDTH	= 300
local BANDS	= 28
local ox, oy	= -100, -45

function ENT:DrawSpectrumAnalyzer()
	local chan = self.radiostream
	if !chan then return end

	local fft = chan:fft2048()
	local b0 = 0

	surface.SetDrawColor(0, 0, 255, 255)

	for x = 0, BANDS-2 do
		local sum = 0
		local sc = 0
		local b1 = math.pow(2,x*10.0/(BANDS-1))

		if (b1>1023) then b1=1023 end
		if (b1<=b0) then b1=b0+1 end
		sc=10+b1-b0;
		while b0 < b1 do
			sum = sum + fft[2+b0]
			b0 = b0 + 1
		end
		y = (math.sqrt(sum/math.log10(sc))*1.7*SPECHEIGHT)-4
		y = math.Clamp(y, 0, SPECHEIGHT)

		surface.DrawRect(ox + (x*(SPECWIDTH/BANDS)), oy - y - 1, (SPECWIDTH/BANDS) - 2, y + 1)
	end
end

function ENT:Think()
	if self.Browser then
		self:MouseThink()
		if self.CursorOut > 0 && CurTime() > self.CursorOut then
			self:RemoveBrowser()
			self.DrawTranslucent = self.MessageDraw
		end
	end
	if self.radiostream then self:RadioThink() end
end

function ENT:RadioThink()
	local pos = self:GetPos()
	pos.z = -pos.z

	self.radiostream:set3dposition(pos, zerovec, zerovec)
end

hook.Add("Location", "PlayerLeaveRoomRadio", function(ply, location)
	if ply != LocalPlayer() || !BASS then return end
	
	for k, v in ipairs(ents.FindByClass("gmt_radio")) do
		if v.CalculateEarShot then
			v:CalculateEarShot()

			if !v.InEarShot && v.radiostream then
				v:StopRadio()
			elseif v.InEarShot && !v.radiostream then
				v:StartRadio(v.Stream)
			end
		end
	end
end)

local function BassThink()
	local ply = LocalPlayer()

	local eyepos = ply:EyePos()
	eyepos.z = -eyepos.z

	local vel = ply:GetVelocity()

	local eyeangles = ply:GetAimVector():Angle()

	// threshold, 89 exact is backwards accord to BASS
	eyeangles.p = math.Clamp(eyeangles.p, -89, 88.9)
 
	local forward = eyeangles:Forward()
	local up = eyeangles:Up() * -1

	BASS.SetPosition(eyepos, vel * 0.005, forward, up)
end

if BASS then
	hook.Add("Think", "UpdateBassPosition", BassThink )
end