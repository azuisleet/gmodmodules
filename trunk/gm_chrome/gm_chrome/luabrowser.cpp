#include "gm_chrome.h"
#undef GetObject

#define META_BROWSER "ChromeBrowser"
#define TYPE_BROWSER 7002

typedef std::vector<Browser *> browserVec;
browserVec freeBrowsers;
browserVec allBrowsers;

#define GETMEMBERFROMREF(mem) 	g_pLua->PushReference(browser->handler); \
	ILuaObject *handler = g_pLua->GetObject(); \
	if(!handler) return; \
	ILuaObject *func = handler->GetMember(mem);

#define BROWSERFROMLUA() ILuaInterface *gLua = Lua(); \
	if (gLua->GetType(1) != TYPE_BROWSER) gLua->TypeError(META_BROWSER, 1); \
	Browser *browser = (Browser *)gLua->GetUserData(1); \
	if(browser->free) return 0; \
	Awesomium::WebView *view = browser->view;

class LuaWebViewListener : public Awesomium::WebViewListener
{
public:
	LuaWebViewListener(Browser *myBrowser) : browser(myBrowser){}

	void onBeginNavigation(const std::string& url, const std::wstring& frameName)
	{
		GETMEMBERFROMREF("onBeginNavigation")
			g_pLua->Push(func);
		g_pLua->Push(handler);
		g_pLua->Push(url.c_str());
		g_pLua->Call(2);
	}

	void onBeginLoading(const std::string& url, const std::wstring& frameName, int statusCode, const std::wstring& mimeType)
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
	}

	void onReceiveTitle(const std::wstring& title, const std::wstring& frameName)
	{
	}

	void onChangeTooltip(const std::wstring& tooltip)
	{
	}

	void onChangeCursor(const HCURSOR& cursor)
	{
	}

	void onChangeKeyboardFocus(bool isFocused)
	{
		GETMEMBERFROMREF("onChangeFocus")
			g_pLua->Push(func);
		g_pLua->Push(handler);
		g_pLua->Push(isFocused);
		g_pLua->Call(2);
	}

	void onChangeTargetURL(const std::string& url)
	{
	}

private:
	Browser *browser;
};

LUA_FUNCTION(chrome_update)
{
	webCore->update();
	return 0;
}

LUA_FUNCTION(chrome_newbrowser)
{
	ILuaInterface *gLua = Lua();

	gLua->CheckType(1, GLua::TYPE_NUMBER); // width
	gLua->CheckType(2, GLua::TYPE_NUMBER); // height
	gLua->CheckType(3, GLua::TYPE_TEXTURE); // texture

	int width = gLua->GetInteger(1), height = gLua->GetInteger(2);
	ITexture *texture = (ITexture *)gLua->GetUserData(3);

	int handler = gLua->GetReference(4); // callback table

	int texwidth, texheight;
	Browser *browser = NULL;

	ClampSizesToTexture(width, height, texwidth, texheight, texture);

	gLua->Msg("Requesting browser %d %d / %d %d\n", width, height, texwidth, texheight);

	if(freeBrowsers.empty())
	{
		gLua->Msg("Had to make new browser\n");
		browser = new Browser;
		browser->view = webCore->createWebView(width, height, false, true, 30);

		allBrowsers.push_back(browser);
	} else {
		gLua->Msg("Found an existing\n");
		browser = freeBrowsers.back();
		freeBrowsers.pop_back();

		browser->view->resize(width, height);
	}

	browser->listener = new LuaWebViewListener(browser);
	browser->texture = texture;
	browser->regen = new ChromeRegenerator(browser);

	AttachRegenToTexture(gLua, browser->texture, browser->regen);

	browser->width = width, browser->height = height;
	browser->texwidth = texwidth, browser->texheight = texheight;

	browser->handler = handler;
	browser->view->setListener(browser->listener);

	browser->free = false;

	ILuaObject* BrowserLua = gLua->GetMetaTable(META_BROWSER, TYPE_BROWSER);
		gLua->PushUserData(BrowserLua, (void *)browser);
	BrowserLua->UnReference();

	return 1;
}

void FreeBrowser(ILuaInterface *gLua, Browser *browser)
{
	gLua->Msg("Freeing browser\n");

	DetachRegenFromTexture(gLua, browser->texture);

	browser->view->setListener(NULL);
	browser->view->loadURL("about:blank");
	browser->texture = NULL;
	browser->free = true;

	delete browser->listener;
	delete browser->regen;
}

LUA_FUNCTION(browser_free)
{
	BROWSERFROMLUA()

	FreeBrowser(gLua, browser);

	freeBrowsers.push_back(browser);

	return 0;
}

LUA_FUNCTION(browser_update)
{
	BROWSERFROMLUA()

	if(!browser->view->isDirty())
		return 0;

	// could extend Awesomium to give me the dirty area before
	RegenerateTexture(browser->texture);

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
	bool down = gLua->GetBool(3);
	bool translate = gLua->GetBool(4);

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
		return 0;
	}

	if(down)
		view->injectKeyboardEvent(0, WM_KEYDOWN, vkey, 0);
	else
		view->injectKeyboardEvent(0, WM_KEYUP, vkey, 0);

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

/*
	doesn't unreference!
	lua_run_cl require("chrome")

	lua_run_cl x = Material("chrome/radio") y = x:GetMaterialTexture("$basetexture") print(y) z = chrome.NewBrowser(512, 512, y) print(z) a = surface.GetTextureID("chrome/radio")

	lua_run_cl hook.Add("HUDPaint", "blah", function() z:Update() surface.SetDrawColor(255, 255, 255, 255) surface.SetTexture(a) surface.DrawTexturedRect(0,0, 1024, 512) end)

*/

void LuaBrowser_Init(ILuaInterface *gLua)
{
	ILuaObject* table = gLua->GetNewTable();
		table->SetMember("NewBrowser", chrome_newbrowser);
	gLua->SetGlobal("chrome", table);

	table = gLua->GetMetaTable(META_BROWSER, TYPE_BROWSER);
		//table->SetMember("__gc", browser_free);
		ILuaObject* __index = gLua->GetNewTable();
			__index->SetMember("Free", browser_free);
			__index->SetMember("Update", browser_update);
			__index->SetMember("LoadURL", browser_loadurl);
			__index->SetMember("LoadHTML", browser_loadhtml);
			__index->SetMember("LoadFile", browser_loadfile);
			__index->SetMember("MouseMove", browser_mousemove);
			__index->SetMember("MouseScroll", browser_mousescroll);
			__index->SetMember("MouseUpDown", browser_mouseupdown);
			__index->SetMember("KeyEvent", browser_keyevent);
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

	table->UnReference();
}

void LuaBrowser_Close(ILuaInterface *gLua)
{
	webCore->update();

	freeBrowsers.clear();

	for(browserVec::iterator iter = allBrowsers.begin(); iter != allBrowsers.end(); iter++)
	{
		Browser *browser = *iter;

		if(!browser->free)
			FreeBrowser(gLua, browser);

#ifndef STAY_IN_MEMORY
		browser->view->destroy();
		delete browser;
#else
		freeBrowsers.push_back(browser);
#endif
	}

#ifndef STAY_IN_MEMORY
	allBrowsers.clear();
#endif
}