#include "gm_chrome.h"
#undef GetObject

GMOD_MODULE(Start, Close);

IMaterialSystem		*materialsystem = NULL;
IInputSystem		*inputsystem	= NULL;

Awesomium::WebCore	*webCore		= NULL;
ILuaInterface		*g_pLua			= NULL;

IDirect3DDevice9	*g_pDevice		= NULL;

#define META_BROWSER "ChromeBrowser"
#define TYPE_BROWSER 7002

struct Browser
{
	Awesomium::WebView *view;
	Awesomium::WebViewListener *listener;

	IDirect3DTexture9 *tex;
	IDirect3DVertexBuffer9 *vbuff;

	int width, height;

	int handler;

	bool free;
};

std::vector<Browser *> freeBrowsers;
std::vector<Browser *> allBrowsers;

bool deviceLost = false;
bool memoryLost = false;

#define GETMEMBERFROMREF(mem) 	g_pLua->PushReference(browser->handler); \
	ILuaObject *handler = g_pLua->GetObject(); \
	if(!handler) return; \
	ILuaObject *func = handler->GetMember(mem);

#define BROWSERFROMLUA() ILuaInterface *gLua = Lua(); \
	if (gLua->GetType(1) != TYPE_BROWSER) gLua->TypeError(META_BROWSER, 1); \
	Browser *browser = (Browser *)gLua->GetUserData(1); \
	if(browser->free) return 0; \
	Awesomium::WebView *view = browser->view; \
	if(!view) return 0;

void CreateTexture(ILuaInterface *gLua, IDirect3DTexture9 **tex, int *width, int *height)
{
	HRESULT result = D3DXCreateTexture(g_pDevice, *width, *height, 1, D3DUSAGE_DYNAMIC, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, tex);
	gLua->Msg("tex result: %d. w,h = %d, %d\n", result, *width, *height);

	if(FAILED(result))
	{
		*width = 0;
		return;
	}

	D3DSURFACE_DESC desc;
	(*tex)->GetLevelDesc(0, &desc);

	gLua->Msg("tex w,h = %d, %d - %d\n", desc.Width, desc.Height, desc.Format);

	*width = desc.Width, *height = desc.Height;
}

LUA_FUNCTION(chrome_newbrowser)
{
	ILuaInterface *gLua = Lua();

	gLua->CheckType(1, GLua::TYPE_NUMBER);
	gLua->CheckType(2, GLua::TYPE_NUMBER);

	int width = gLua->GetInteger(1), height = gLua->GetInteger(2);

	Browser *browser = NULL;

	if(!freeBrowsers.empty())
	{
		browser = freeBrowsers.back();

		if(browser->width != width || browser->height != height)
		{
			browser->tex->Release();
			browser->tex = NULL;

			IDirect3DTexture9 *tex = NULL;
			CreateTexture(gLua, &tex, &width, &height);

			if(width == 0)
				return 0;

			browser->tex = tex;
		}

		freeBrowsers.pop_back();
	} else {
		IDirect3DTexture9 *tex = NULL;

		CreateTexture(gLua, &tex, &width, &height);

		if(width == 0)
			return 0;

		Awesomium::WebView *view = webCore->createWebView(width, height, false, true);

		browser = new Browser;
		allBrowsers.push_back(browser);

		browser->tex = tex;

		browser->width = width, browser->height = height;

		browser->view = view;

		browser->listener = NULL; //new LuaWebViewListener(browser);
	}

	browser->free = false;

	browser->handler = gLua->GetReference(3);
	browser->view->setListener(browser->listener);

	ILuaObject* BrowserLua = gLua->GetMetaTable(META_BROWSER, TYPE_BROWSER);
	gLua->PushUserData(BrowserLua, (void *)browser);
	BrowserLua->UnReference();

	return 1;
}

LUA_FUNCTION(chrome_update)
{
	if(!webCore || !g_pDevice)
		return 0;

	ILuaInterface *gLua = Lua();

	HRESULT coop = g_pDevice->TestCooperativeLevel();
			
	if(coop == D3DERR_DEVICELOST)
	{
		deviceLost = true;
	} 
	else if(coop == D3DERR_DEVICENOTRESET && !memoryLost) 
	{
		deviceLost = false;
		memoryLost = true;
		gLua->Msg("Lost device.. clearing\n");

		for(std::vector<Browser *>::iterator iter = allBrowsers.begin(); iter != allBrowsers.end(); iter++)
		{
			Browser *browser = *iter;
			browser->tex->Release();
			browser->tex = NULL;
		}
	}
	else if(coop == D3D_OK && memoryLost)
	{
		gLua->Msg("Restoring device..\n");
		memoryLost = false;
		for(std::vector<Browser *>::iterator iter = allBrowsers.begin(); iter != allBrowsers.end(); iter++)
		{
			Browser *browser = *iter;
			int width = browser->width, height = browser->height;
			IDirect3DTexture9 *tex = NULL;
			CreateTexture(Lua(), &tex, &width, &height);

			if(width != 0)
			{
				browser->tex = tex;
				browser->width = width, browser->height = height;
			}
		}
	}

	webCore->update();

	return 0;
}

LUA_FUNCTION(browser_render)
{
	BROWSERFROMLUA()

	if(!browser->tex || deviceLost)
		return 0;

	bool force = gLua->GetBool(2);

	if(!view->isDirty() && !force)
		return 0;

	D3DLOCKED_RECT lockedRect;
	HRESULT lock = browser->tex->LockRect(0, &lockedRect, NULL, D3DLOCK_DISCARD | D3DLOCK_DONOTWAIT);
	if (SUCCEEDED(lock))
	{
		Awesomium::Rect rect;
		unsigned char* destBuffer = static_cast<unsigned char*>(lockedRect.pBits);
		browser->view->render(destBuffer, lockedRect.Pitch, (int)4, &rect);
		browser->tex->UnlockRect(0);

		//D3DXSaveTextureToFileA("test.jpg", D3DXIFF_JPG, browser->tex, NULL);
	}

	return 0;
}

/*
lua_run_cl require("chrome") x,y = chrome.NewBrowser(400, 300) x:LoadHTML("<h1>hi</h1>") print(x)

lua_run_cl hook.Add("RenderScreenspaceEffects", "blah", function() x:Render() x:Draw() end)

lua_run_cl hook.Add("RenderScreenspaceEffects", "blah", function() surface.SetTexture(tex) surface.SetDrawColor(255, 255, 255, 255) surface.DrawTexturedRect(0,0,256, 256) x:Render() x:Draw() end)

// using screenspace quads..
lua_run_cl require("chrome") x,y = chrome.NewBrowser(400, 300) x:LoadFile("example.html") print(x) tex_Morph0 = render.GetMorphTex0() mat_ColorMod = Material( "pp/fb" )

lua_run_cl function HackD3D() local rt = render.GetRenderTarget() render.SetRenderTarget(tex_Morph0) render.SetMaterial(mat_ColorMod) render.DrawScreenQuad() render.SetRenderTarget(rt) end

lua_run_cl hook.Add("RenderScreenspaceEffects", "blah", function() HackD3D() x:Render(false) x:Draw(50, 50) end)

*/

struct PANELVERTEX
{
	FLOAT x, y, z;
	FLOAT u, v;
};

#define D3DFVF_PANELVERTEX (D3DFVF_XYZ | D3DFVF_TEX1)

IDirect3DVertexBuffer9 *vertBuff = NULL;

LUA_FUNCTION(browser_draw)
{
	BROWSERFROMLUA()

	if(!browser->tex || deviceLost)
		return 0;

	int xpos = gLua->GetInteger(2), ypos = gLua->GetInteger(3);
	int Width = browser->width, Height = browser->height;

	float Left = xpos - 0.5;
	float Top = ypos - 0.5;

	float Right = Left + Width;
	float Bottom = Top + Height;
	//float Left = xpos * 2.0f / viewport.Width - 1.0f;
	//float Top = 1.0f - ypos * 2.0f / viewport.Height;

	//float Right = (xpos + Width) * 2.0f / viewport.Width - 1.0f;
	//float Bottom = 1.0f - (ypos + Height) * 2.0f / viewport.Height;

	PANELVERTEX pVertices[4];

	pVertices[0].x = pVertices[3].x = Left;
	pVertices[1].x = pVertices[2].x = Right;

	pVertices[0].y = pVertices[1].y = Top;
	pVertices[2].y = pVertices[3].y = Bottom;

	pVertices[0].z = pVertices[1].z = pVertices[2].z = pVertices[3].z = 1.0f;

	pVertices[1].u = pVertices[2].u = 1.0f;
	pVertices[0].u = pVertices[3].u = 0.0f;

	pVertices[0].v = pVertices[1].v = 0.0f;
	pVertices[2].v = pVertices[3].v = 1.0f;

	IDirect3DPixelShader9 *pshader = NULL;
	IDirect3DVertexShader9 *vshader = NULL;
	IDirect3DBaseTexture9 *texture = NULL;
	DWORD FVF, colorop, colorarg1, colorarg2, alphablend, lighting;

	g_pDevice->GetFVF( &FVF );
/*
	g_pDevice->GetPixelShader(&pshader);
	g_pDevice->GetVertexShader(&vshader);
	
	g_pDevice->SetPixelShader(NULL);
	g_pDevice->SetVertexShader(NULL);
*/

	g_pDevice->GetTexture(0, &texture);
	g_pDevice->GetTextureStageState(0, D3DTSS_COLOROP, &colorop);
	g_pDevice->GetTextureStageState(0, D3DTSS_COLORARG1, &colorarg1);
	g_pDevice->GetTextureStageState(0, D3DTSS_COLORARG2, &colorarg2);

	g_pDevice->GetRenderState(D3DRS_ALPHABLENDENABLE, &alphablend);
	g_pDevice->GetRenderState(D3DRS_LIGHTING, &lighting);

	g_pDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
	g_pDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	g_pDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );

	g_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
//	g_pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

	g_pDevice->SetTexture(0, browser->tex);
	g_pDevice->SetFVF( D3DFVF_PANELVERTEX );

	g_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, pVertices, sizeof(PANELVERTEX));

	g_pDevice->SetTexture(0, texture);

	g_pDevice->SetTextureStageState( 0, D3DTSS_COLOROP, colorop );
	g_pDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, colorarg1 );
	g_pDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, colorarg2 );

	g_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
//	g_pDevice->SetRenderState(D3DRS_LIGHTING, lighting);

	g_pDevice->SetFVF(FVF);
/*
	g_pDevice->SetPixelShader(pshader);
	g_pDevice->SetVertexShader(vshader);
*/

	return 0;
}

LUA_FUNCTION(browser_resize)
{
	BROWSERFROMLUA()

	gLua->CheckType(2, GLua::TYPE_NUMBER);
	gLua->CheckType(3, GLua::TYPE_NUMBER);

	int width = gLua->GetInteger(2), height = gLua->GetInteger(3);

	browser->tex->Release();
	browser->tex = NULL;

	IDirect3DTexture9 *tex = NULL;
	CreateTexture(gLua, &tex, &width, &height);

	if(width == 0)
		return 0;

	browser->tex = tex;
	browser->width = width, browser->height = height;

	view->resize(width, height);

	return 0;
}

LUA_FUNCTION(browser_mousemove)
{
	BROWSERFROMLUA()

	int x = gLua->GetInteger(2);
	int y = gLua->GetInteger(3);
	view->injectMouseMove(x, y);

	return 0;
}

LUA_FUNCTION(browser_mousescroll)
{
	BROWSERFROMLUA()

	int delta = gLua->GetInteger(2);

	view->injectMouseWheel(delta);

	return 0;
}

LUA_FUNCTION(browser_mouseupdown)
{
	BROWSERFROMLUA()

	bool down = gLua->GetBool(2);
	int button = gLua->GetInteger(3);

	if(down)
	{
		view->injectMouseDown((Awesomium::MouseButton)button);
	} else {
		view->injectMouseUp((Awesomium::MouseButton)button);
	}

	return 0;
}

LUA_FUNCTION(browser_keyevent)
{
	BROWSERFROMLUA()

	int vkey = gLua->GetInteger(2);
	bool translate = gLua->GetBool(3);

	bool dispatchKey = true;

	if(translate)
	{
		vkey = inputsystem->ButtonCodeToVirtualKey((ButtonCode_t)vkey);
		if(vkey != VK_RETURN)
			dispatchKey = false;
	} 

	if(dispatchKey)
	{
		view->injectKeyboardEvent(0, WM_CHAR, vkey, 0);
	}

	view->injectKeyboardEvent(0, WM_KEYDOWN, vkey, 0);
	view->injectKeyboardEvent(0, WM_KEYUP, vkey, 0);

	return 0;
}

LUA_FUNCTION(browser_loadurl)
{
	BROWSERFROMLUA()

	gLua->CheckType(2, GLua::TYPE_STRING);

	view->loadURL(gLua->GetString(2));

	return 0;
}

LUA_FUNCTION(browser_loadhtml)
{
	BROWSERFROMLUA()

	gLua->CheckType(2, GLua::TYPE_STRING);

	view->loadHTML(gLua->GetString(2));

	return 0;
}

LUA_FUNCTION(browser_loadfile)
{
	BROWSERFROMLUA()

	gLua->CheckType(2, GLua::TYPE_STRING);

	view->loadFile(gLua->GetString(2));

	return 0;
}

LUA_FUNCTION(browser_exec)
{
	BROWSERFROMLUA()

	gLua->CheckType(2, GLua::TYPE_STRING);

	view->executeJavascript(gLua->GetString(2));

	return 0;
}

LUA_FUNCTION(browser_setclientproperty)
{
	BROWSERFROMLUA()

	gLua->CheckType(2, GLua::TYPE_STRING);
	gLua->CheckType(3, GLua::TYPE_STRING);

	view->setProperty(gLua->GetString(2), (const char *)gLua->GetString(3));

	return 0;
}

LUA_FUNCTION(browser_registerclientcallback)
{
	BROWSERFROMLUA()

	gLua->CheckType(2, GLua::TYPE_STRING);

	view->setCallback(gLua->GetString(2));

	return 0;
}

LUA_FUNCTION(browser_gc)
{
	BROWSERFROMLUA()

	gLua->Msg("cleaning up browser\n");

	view->setListener(NULL);

	view->loadURL("about:blank");

	gLua->FreeReference(browser->handler);

	browser->free = true;

	freeBrowsers.push_back(browser);

	return 0;
}

int Start( lua_State* L )
{
	ILuaInterface *gLua = Lua();

	typedef DWORD* (__cdecl *GetD3DDevicePointer_t)();
	HMODULE hShaderAPI = GetModuleHandleA( "shaderapidx9.dll" );

	// from G-D by s0beit
	GetD3DDevicePointer_t pGetD3DDevicePointer = (GetD3DDevicePointer_t)((DWORD)hShaderAPI + 0xB9E0);
	DWORD *dwDevice = pGetD3DDevicePointer();

	if(dwDevice)
	{
		g_pDevice = (IDirect3DDevice9*)*dwDevice;
		gLua->Msg("D3D Device: %x\n", g_pDevice);
	} else	{
		gLua->Error("Could not hack D3D device.");
		return 0;
	}

	g_pLua = Lua(); // this should be ok because events are dispatched through chrome_update

	webCore = new Awesomium::WebCore(Awesomium::LOG_NORMAL, true, Awesomium::PF_BGRA);

	webCore->setBaseDirectory(modulemanager->GetBaseFolder());

	CreateInterfaceFn inputsystemFactory = Sys_GetFactory("inputsystem.dll");
	inputsystem = (IInputSystem *)inputsystemFactory(INPUTSYSTEM_INTERFACE_VERSION, NULL);

	CreateInterfaceFn materialsystemFactory = Sys_GetFactory( "materialsystem.dll" );
	materialsystem = (IMaterialSystem*)materialsystemFactory( MATERIAL_SYSTEM_INTERFACE_VERSION, NULL );

	ILuaObject* table = gLua->GetNewTable();
		table->SetMember("NewBrowser", chrome_newbrowser);
	gLua->SetGlobal("chrome", table);
	table->UnReference();

	table = gLua->GetMetaTable(META_BROWSER, TYPE_BROWSER);
		table->SetMember("__gc", browser_gc);
		ILuaObject* __index = gLua->GetNewTable();
			__index->SetMember("Free", browser_gc);
			__index->SetMember("Render", browser_render);
			__index->SetMember("Draw", browser_draw);
			__index->SetMember("Resize", browser_resize);
			__index->SetMember("MouseMove", browser_mousemove);
			__index->SetMember("MouseScroll", browser_mousescroll);
			__index->SetMember("MouseUpDown", browser_mouseupdown);
			__index->SetMember("KeyEvent", browser_keyevent);
			__index->SetMember("LoadURL", browser_loadurl);
			__index->SetMember("LoadHTML", browser_loadhtml);
			__index->SetMember("LoadFile", browser_loadfile);
			__index->SetMember("Exec", browser_exec);
			__index->SetMember("SetClientProperty", browser_setclientproperty);
			__index->SetMember("RegisterClientCallback", browser_registerclientcallback);
		table->SetMember("__index", __index);
		__index->UnReference();
	table->UnReference();

	// hook.Add("Think", "ChromePoll", chrome.poll)
	ILuaObject *hookt = gLua->GetGlobal("hook");
	ILuaObject *addf = hookt->GetMember("Add");
	addf->Push();
	gLua->Push("Tick");
	gLua->Push("ChromePoll");
	gLua->Push(chrome_update);
	gLua->Call(3);
	return 0;
}

int Close( lua_State* L )
{
	if(vertBuff)
		Lua()->Msg("releasing vert buff %d\n", vertBuff->Release());

	while(!allBrowsers.empty())
	{
		Lua()->Msg("freed browser\n");

		Browser *browser = allBrowsers.back();
		allBrowsers.pop_back();

		if(!browser->free)
		{
			browser->view->setListener(NULL);

			browser->view->loadURL("about:blank");
			//webCore->update();

			Lua()->FreeReference(browser->handler);
		}

		delete browser->listener;
		browser->view->destroy();

		Lua()->Msg("releasing browser tex %d\n", browser->tex->Release());

		//if(browser->vbuff)
		//	browser->vbuff->Release();

		delete browser;
	}

	if(webCore)
		delete webCore;

	return 0;
}