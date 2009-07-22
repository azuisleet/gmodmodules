#ifdef _WIN32
#pragma once
#endif

#define STAY_IN_MEMORY

#define _RETAIL
#include "GMLuaModule.h"

#include <WebCore.h>

#include <vgui/ISurface.h>
#include <inputsystem/iinputsystem.h>
#include <vgui/IPanel.h>
#include <vgui_controls/Panel.h>

#include <imaterialsystem.h>
#include <itexture.h>
#include <pixelwriter.h>
#include <imaterial.h>
#include <imaterialproxy.h>
#include <imaterialvar.h>

#include <KeyValues.h>

class ChromeRegenerator;

struct Browser
{
	Awesomium::WebView *view;
	Awesomium::WebViewListener *listener;

	ITexture *texture;
	ChromeRegenerator *regen;

	int width, height;
	int texwidth, texheight;

	int handler;
	bool free;

	bool wipeTex;
};

class ChromeRegenerator : public ITextureRegenerator
{
public:
	ChromeRegenerator( Browser *browser) : pBrowser(browser) {};
	virtual void RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pSubRect );
	virtual void Release( void );

private:
	Browser *pBrowser;
};

extern IMaterialSystem		*materialsystem;
extern IInputSystem			*inputsystem;
extern Awesomium::WebCore	*webCore;
extern ILuaInterface		*g_pLua;

void ClampSizesToTexture(int& width, int& height, int& texwidth, int& texheight, ITexture *tex);
void AttachRegenToTexture(ILuaInterface *gLua, ITexture *tex, ITextureRegenerator *regen);
void DetachRegenFromTexture(ILuaInterface *gLua, ITexture *tex);
void RegenerateTexture(ITexture *tex);

void LuaBrowser_Init(ILuaInterface *gLua);
void LuaBrowser_Close(ILuaInterface *gLua);