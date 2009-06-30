#include "gm_chrome.h"
#undef GetObject

GMOD_MODULE(Start, Close);

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

std::stack<Browser *> freeBrowsers;
std::stack<Browser *> allBrowsers;

#define GETMEMBERFROMREF(mem) 	g_pLua->PushReference(browser->handler); \
	ILuaObject *handler = g_pLua->GetObject(); \
	if(!handler) return; \
	ILuaObject *func = handler->GetMember(mem);

class LuaWebViewListener : public Awesomium::WebViewListener
{
public:
	LuaWebViewListener(Browser *myBrowser) : browser(myBrowser)
	{
	}

	void onBeginNavigation(const std::string& url)
	{
		GETMEMBERFROMREF("onBeginNavigation")
			g_pLua->Push(func);
		g_pLua->Push(handler);
		g_pLua->Push(url.c_str());
		g_pLua->Call(2);
	}

	void onBeginLoading(const std::string& url, int statusCode, const std::wstring& mimeType)
	{
		GETMEMBERFROMREF("onBeginLoading")
			g_pLua->Push(func);
		g_pLua->Push(handler);
		g_pLua->Push(url.c_str());
		g_pLua->Push((float)statusCode);
		g_pLua->Call(3);
	}

	void onFinishLoading()
	{
		GETMEMBERFROMREF("onFinishLoading")
			g_pLua->Push(func);
		g_pLua->Push(handler);
		g_pLua->Call(1);
	}

	void onCallback(const std::string& name, const Awesomium::JSArguments& args)
	{
		int argc = 2;

		GETMEMBERFROMREF("onCallback")
			g_pLua->Push(func);
		g_pLua->Push(handler);
		g_pLua->Push(name.c_str());
		for(Awesomium::JSArguments::const_iterator x = args.begin(); x != args.end(); x++)
		{
			g_pLua->Push( (*x).toString().c_str() );
			argc++;
		}
		g_pLua->Call(argc);
	}

	void onReceiveTitle(const std::wstring& title)
	{
		std::string sTitle;
		sTitle.assign(title.begin(), title.end()); 

		GETMEMBERFROMREF("onReceiveTitle")
			g_pLua->Push(func);
		g_pLua->Push(handler);
		g_pLua->Push(sTitle.c_str());
		g_pLua->Call(2);
	}

	void onChangeTooltip(const std::wstring& tooltip)
	{
		std::string sTooltip;
		sTooltip.assign(tooltip.begin(), tooltip.end()); 

		GETMEMBERFROMREF("onChangeTooltip")
			g_pLua->Push(func);
		g_pLua->Push(handler);
		g_pLua->Push(sTooltip.c_str());
		g_pLua->Call(2);
	}

	void onChangeCursor(const HCURSOR& cursor)
	{
		GETMEMBERFROMREF("onChangeCursor")
			g_pLua->Push(func);
		g_pLua->Push(handler);
		g_pLua->PushLong((long)cursor);
		g_pLua->Call(2);
	}

	void onChangeKeyboardFocus(bool isFocused)
	{
		GETMEMBERFROMREF("onChangeKeyboardFocus")
			g_pLua->Push(func);
		g_pLua->Push(handler);
		g_pLua->Push(isFocused);
		g_pLua->Call(2);
	}
private:
	Browser *browser;
};

#define BROWSERFROMLUA() ILuaInterface *gLua = Lua(); \
	if (gLua->GetType(1) != TYPE_BROWSER) gLua->TypeError(META_BROWSER, 1); \
	Browser *browser = (Browser *)gLua->GetUserData(1); \
	if(browser->free) return 0; \
	Awesomium::WebView *view = browser->view; \
	if(!view) return 0;

LUA_FUNCTION(chrome_newbrowser)
{
	ILuaInterface *gLua = Lua();

	gLua->CheckType(1, GLua::TYPE_NUMBER);
	gLua->CheckType(2, GLua::TYPE_NUMBER);

	int width = gLua->GetInteger(1), height = gLua->GetInteger(2);

	Browser *browser = NULL;

	if(!freeBrowsers.empty())
	{
		browser = freeBrowsers.top();
		freeBrowsers.pop();
	} else {
		IDirect3DTexture9 *tex = NULL;
		HRESULT result = D3DXCreateTexture(g_pDevice, width, height, 1, D3DUSAGE_DYNAMIC, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &tex);
		gLua->Msg("tex result: %d. w,h = %d, %d\n", result, width, height);

		if(FAILED(result))
			return 0;

		D3DSURFACE_DESC desc;
		tex->GetLevelDesc(0, &desc);

		width = desc.Width, height = desc.Height;

		gLua->Msg("tex w,h = %d, %d - %d\n", width, height, desc.Format);

		Awesomium::WebView *view = webCore->createWebView(width, height, false, true);

		browser = new Browser;
		allBrowsers.push(browser);

		browser->tex = tex;

		browser->width = width, browser->height = height;

		browser->view = view;

		browser->listener = new LuaWebViewListener(browser);
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
	if(!webCore)
		return 0;

	webCore->update();

	return 0;
}

LUA_FUNCTION(browser_render)
{
	BROWSERFROMLUA()

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
lua_run_cl require("chrome") x,y = chrome.NewBrowser(400, 300) x:LoadHTML("<h1>rofl</h1>") print(x)
lua_run_cl x:Render()
lua_run_cl hook.Add("HUDPaint", "blah", function() x:Render() x:Draw(0,0) end)

lua_run_cl require("chrome") x,y = chrome.NewBrowser(400, 300) x:LoadFile("example.html") print(x)

lua_run_cl local tex = surface.GetTextureID("chrome0") hook.Add("HUDPaint", "blah", function() x:Render(false) surface.SetTexture(tex) surface.SetDrawColor(255, 255, 255, 255) surface.DrawTexturedRect(0,0,0,0) x:Draw(50, 50) end)


lua_run_cl require("chrome") x,y = chrome.NewBrowser(400, 300) x:LoadFile("example.html") print(x) tex = surface.GetTextureID("chrome0")

lua_run_cl hook.Add("RenderScreenspaceEffects", "blah", function() x:Render(false) x:DebugDX(CurTime()) surface.SetTexture(tex) surface.SetDrawColor(255, 255, 255, 255) surface.DrawTexturedRect(0,0,5,5) x:DebugDX(CurTime()) x:Draw(50, 50) end)


lua_run_cl hook.Add("RenderScreenspaceEffects", "blah", function() x:Render(false) x:Draw(50, 50) end)

*/

struct PANELVERTEX
{
	FLOAT x, y, z;
	FLOAT u, v;
};

#define D3DFVF_PANELVERTEX (D3DFVF_XYZ | D3DFVF_TEX1)

IDirect3DVertexBuffer9 *vertBuff = NULL;

double time = 0;
LUA_FUNCTION(browser_debugdx)
{
	BROWSERFROMLUA()

	double ntime = gLua->GetDouble(1);

	if(ntime - time > 0.01 && ntime < time+1)
		return 0;

	time = ntime;

	D3DVIEWPORT9 viewport;
	g_pDevice->GetViewport(&viewport);

	D3DXMATRIX matProj;
	g_pDevice->GetTransform( D3DTS_PROJECTION, &matProj );

	gLua->Msg("Viewport, x: %d y: %d, w: %d, h: %d\n", viewport.X, viewport.Y, viewport.Width, viewport.Height);
	gLua->Msg("Project: %d %d %d\n", matProj._41, matProj._42, matProj._43);
	return 0;
}

LUA_FUNCTION(browser_draw)
{
	BROWSERFROMLUA()

	int xpos = gLua->GetInteger(2), ypos = gLua->GetInteger(3);
	int Width = browser->width, Height = browser->height;

	D3DVIEWPORT9 viewport;
	g_pDevice->GetViewport(&viewport);

	D3DXMATRIX matProj;
	g_pDevice->GetTransform( D3DTS_PROJECTION, &matProj );

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
	HRESULT result = D3DXCreateTexture(g_pDevice, width, height, 1, D3DUSAGE_DYNAMIC, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &tex);
	gLua->Msg("tex result: %d. w,h = %d, %d\n", result, width, height);

	D3DSURFACE_DESC desc;
	tex->GetLevelDesc(0, &desc);

	width = desc.Width, height = desc.Height;

	gLua->Msg("tex w,h = %d, %d - %d\n", width, height, desc.Format);

	if(FAILED(result))
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

	//view->loadURL("about:blank");
	//webCore->update();

	gLua->FreeReference(browser->handler);

	browser->free = true;

	freeBrowsers.push(browser);

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

	ILuaObject* table = gLua->GetNewTable();
		table->SetMember("NewBrowser", chrome_newbrowser);
	gLua->SetGlobal("chrome", table);
	table->UnReference();

	table = gLua->GetMetaTable(META_BROWSER, TYPE_BROWSER);
		table->SetMember("__gc", browser_gc);
		ILuaObject* __index = gLua->GetNewTable();
			__index->SetMember("DebugDX", browser_debugdx);
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
	gLua->Push("Think");
	gLua->Push("ChromePoll");
	gLua->Push(chrome_update);
	gLua->Call(3);
	return 0;
}

int Close( lua_State* L )
{
	if(vertBuff)
		vertBuff->Release();

	while(!allBrowsers.empty())
	{
		Lua()->Msg("freed browser\n");

		Browser *browser = allBrowsers.top();
		allBrowsers.pop();

		if(!browser->free)
		{
			browser->view->setListener(NULL);

			//browser->view->loadURL("about:blank");
			//webCore->update();

			Lua()->FreeReference(browser->handler);
		}

		delete browser->listener;
		browser->view->destroy();

		browser->tex->Release();

		//if(browser->vbuff)
		//	browser->vbuff->Release();

		delete browser;
	}

	if(webCore)
		delete webCore;

	return 0;
}