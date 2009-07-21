AddCSLuaFile("shared.lua")

ENT.Base = "base_anim"
ENT.Type = "anim"

ENT.RenderGroup = RENDERGROUP_BOTH

local activebrowser = nil
MatInUse = {}

function SetupBrowserMat(ENT, mat, width, height, scale)
	if SERVER then return end
	require("chrome")

	ENT.Mat = mat
	ENT.BrowserMat = Material(mat)
	ENT.BrowserTex = ENT.BrowserMat:GetMaterialTexture("$basetexture")
	ENT.TexId = surface.GetTextureID(mat)

	if !ENT.BrowserTex then
		Error("Invalid mat " .. tostring(mat))
	end

	ENT.TexWidth = ENT.BrowserTex:GetActualWidth()
	ENT.TexHeight = ENT.BrowserTex:GetActualHeight()
	ENT.Width = width
	ENT.Height = height
	ENT.Scale = scale
	MatInUse[mat] = false
end

local function RayQuadIntersect(vOrigin, vDirection, vPlane, vX, vY)
	local vp = vDirection:Cross(vY)

	local d = vX:DotProduct(vp)

	if (d <= 0.0) then return end

	local vt = vOrigin - vPlane
	local u = vt:DotProduct(vp)
	if (u < 0.0 or u > d) then return end

	local v = vDirection:DotProduct(vt:Cross(vX))
	if (v < 0.0 or v > d) then return end

	return Vector(u / d, v / d, 0)
end

local function BuildFace(vMins, vMaxs)
	local p3 = Vector(0, vMaxs.y, vMins.z)
	local p4 = Vector(0, vMins.y, vMins.z)
	local p7 = Vector(0, vMaxs.y, vMaxs.z)
	local p8 = Vector(0, vMins.y, vMaxs.z)

	return
	{
		Vertex( p8, 0, 0 ),
		Vertex( p7, 1, 0 ),
		Vertex( p4, 0, 1 ),
		Vertex( p7, 1, 0 ),
		Vertex( p3, 1, 1 ),
		Vertex( p4, 0, 1 ),
	}
end

function ENT:SetLargeBounds()
	local w = self.Width / 2
	local min = Vector(-w * self.Scale, -w * self.Scale, 0)
	local max = Vector(w * self.Scale, w * self.Scale, self.Height*self.Scale)

	self:SetRenderBounds(min, max)
end

function ENT:InitBrowser()
	if !self.Mat then return end

	if MatInUse[self.Mat] then
		MatInUse[self.Mat]:RemoveBrowser()
	end
	
	self.Browser = chrome.NewBrowser(self.Width, self.Height, self.BrowserTex, self:GetTable())
	MatInUse[self.Mat] = self

	self.mX, self.mY = 0, 0
	self.mActive = false
end

function ENT:RemoveBrowser()
	if self.Browser then
		self.Browser:Free()
		self.Browser = false
		MatInUse[self.Mat] = false

		self:CloseInputPanel()
		if self.OnLeaveBrowser then
			self:OnLeaveBrowser()
		end
	end
end

function ENT:OnRemove()
	self:RemoveBrowser()
end

function ENT:GetPosBrowser()
	return self:GetPos()
end

function BrowserScroll(ply, bind, pressed)
	if !IsValid(activebrowser) || !activebrowser.Browser || !activebrowser.mActive then return end

	if bind == "invnext" then
		activebrowser.Browser:MouseScroll(-30)
		return true
	elseif bind == "invprev" then
		activebrowser.Browser:MouseScroll(30)
		return true
	end
end

hook.Add("PlayerBindPress", "BrowserScroll", BrowserScroll)

function ENT:MouseRayInteresct()
	local up, right = self:GetUp(), self:GetRight()
	local plane = self:GetPosBrowser() + ( (up * self.Height * self.Scale) + ( right * (-self.Width/2) * self.Scale ) )

	local x = (right * (self.Width/2) * self.Scale) - (right * (-self.Width/2) * self.Scale)
	local y = (up * -self.Height * self.Scale)

	return RayQuadIntersect(LocalPlayer():EyePos(), LocalPlayer():GetCursorAimVector(), plane, x, y)
end

local mousedownlast = false

function ENT:MouseThink()
	local uv = self:MouseRayInteresct()

	if uv then
		if !self.mActive then
			if self.Focus then
				self:BringUpInputPanel()
			end
			if self.OnEnterBrowser then
				self:OnEnterBrowser()
			end
		end

		// there can only be one active browser, this is for scrolling
		activebrowser = self

		self.mActive = true
		self.mX, self.mY = (1-uv.x) * self.Width, uv.y * self.Height

		self.Browser:MouseMove(self.mX, self.mY)

		local down = input.IsMouseDown(MOUSE_LEFT)

		if down && !mousedownlast then
			self.Browser:MouseUpDown(true, 0)
		elseif !down && mousedownlast then
			self.Browser:MouseUpDown(false, 0)
		end

		mousedownlast = down

		self.mX = math.Clamp(self.mX, 1, self.Width - 1)
		self.mY = math.Clamp(self.mY, 1, self.Height - 1)
	elseif self.mActive then
		self.mActive = false
		self:CloseInputPanel()

		if self.OnLeaveBrowser then
			self:OnLeaveBrowser()
		end
	end
end

function ENT:DrawBrowser()
	surface.SetDrawColor(255, 255, 255, 255)
	surface.SetTexture(self.TexId)
	surface.DrawTexturedRect(0,0, self.TexWidth, self.TexHeight)
end

function ENT:DrawCursor()
	if !self.mActive then return end

	surface.SetDrawColor(255, 0, 0, 255)
	surface.DrawRect(self.mX - 1, self.mY - 1, 2, 2)
end

function ENT:BaseBrowserDraw()
	self.Browser:Update()

	local pos, ang = self:GetPosBrowser(), self:GetAngles()
	local up, right = self:GetUp(), self:GetRight()

	pos = pos + (up * self.Height * self.Scale) + (right * (self.Width/2) * self.Scale)

	ang:RotateAroundAxis(ang:Up(), 90)
	ang:RotateAroundAxis(ang:Forward(), 90)

	cam.Start3D2D(pos,ang,self.Scale)
		self:DrawBrowser()
		self:DrawCursor()
	cam.End3D2D()
end

function ENT:BringUpInputPanel()
	if self.InputPanel then return end

	print("bring up input")
	self.InputPanel = vgui.Create("DTextEntry")
	self.InputPanel:NoClipping(true)
	self.InputPanel:MakePopup()
	self.InputPanel:SetMouseInputEnabled(false)
	self.InputPanel.DownKey = {}

	self.InputPanel.AllowInput = function(panel, strValue)
		self.Browser:KeyEvent(string.byte(strValue), true)
		return false
	end

	self.InputPanel.Think = function(panel)
		// this is so we can "hold down" a key

		for key, down in pairs(panel.DownKey) do
			if !input.IsKeyDown(key) then
				self.Browser:KeyEvent(key, false, true)
				panel.DownKey[key] = nil
			end
		end
	end

	self.InputPanel.OnKeyCodePressed = function(panel, code)
		self.Browser:KeyEvent(code, true, true)
		panel.DownKey[code] = true
		panel.RepeatKey = RealTime()

		if code == KEY_TAB || code == KEY_ENTER then
			self:CloseInputPanel()
		end
	end

	self.InputPanel.Paint = function(panel)
		draw.SimpleText("Hit tab to regain control", "ScoreboardText", ScrW() / 2, 0, Color(255,255,255,255), TEXT_ALIGN_CENTER, TEXT_ALIGN_TOP)
	end
end

function ENT:CloseInputPanel()
	if !self.InputPanel then return end

	print("close input")
	self.InputPanel:Remove()
	self.InputPanel = nil
end

function ENT:onBeginNavigation(url)
	if self.OpeningURL then
		self:OpeningURL(url)
	end
end

function ENT:onBeginLoading(url, status)
	if self.LoadingURL then
		self:LoadingURL(url)
	end
end

function ENT:onFinishLoading()
	if self.FinishLoading then
		self:FinishLoading()
	end
end

function ENT:onChangeFocus(focus)
	print("focus", focus, self.InputPanel)
	if focus then
		self:BringUpInputPanel()
	else
		self:CloseInputPanel()
	end
	self.Focus = focus
end