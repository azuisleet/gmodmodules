#include "gm_chrome.h"
#undef GetObject

GMOD_MODULE(Start, Close);

Awesomium::WebCore	*webCore		= NULL;
ILuaInterface		*g_pLua			= NULL;

IMaterialSystem		*materialsystem = NULL;
IInputSystem		*inputsystem	= NULL;

// need to fix alpha, we can't just use BGR because it won't let us 0 alpha the part outside of the browser
// because sadly we can't use NPOW2

// you'll also notice that the alpha in the first column and row is set to 0, there are some texture filtering issues
// that cause the ends of the image to blend with the other end.. this is an ok hack because nobody is going to miss
// a couple pixels.
inline void FixNPOW2Alpha(unsigned char *data, int texheight, int height, int width, int rowspan)
{
	int RowOffset;
	for(int row = 0; row < texheight; row++)
	{
		RowOffset = row * rowspan;

		for(int colOffset = 0; colOffset < rowspan; colOffset += 4)
		{
			int yoffset = colOffset >> 2;
			if(row > 0 && row < height && yoffset > 0 && yoffset < width)
				data[RowOffset + colOffset + 3] = 255;
			else
				memset(data + (RowOffset + colOffset), 0, 4);
		}
	}
}

// tex needs to be procedural!!
void ChromeRegenerator::RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pSubRect )
{
	Awesomium::WebView *view = pBrowser->view;
	if(!view->isDirty())
		return;

	unsigned char *data = pVTFTexture->ImageData( 0, 0, 0 );
	int rowspan = pVTFTexture->RowSizeInBytes( 0 );

	view->render(data, rowspan, 4);

	FixNPOW2Alpha(data, pBrowser->texheight, pBrowser->height, pBrowser->width, rowspan);
}

void ChromeRegenerator::Release()
{

}

void ClampSizesToTexture(int& width, int& height, int& texwidth, int& texheight, ITexture *tex)
{
	texwidth = tex->GetActualWidth(), texheight = tex->GetActualHeight();
	width = min(width, texwidth), height = min(height, texheight);
}

void AttachRegenToTexture(ILuaInterface *gLua, ITexture *tex, ITextureRegenerator *regen)
{
	tex->SetTextureRegenerator(NULL);

	gLua->Msg("Attaching regen to texture \"%s\"\n", tex->GetName());

	tex->SetTextureRegenerator(regen);
}

void DetachRegenFromTexture(ILuaInterface *gLua, ITexture *tex)
{
	gLua->Msg("Detaching regen from texture \"%s\"\n", tex->GetName());

	tex->SetTextureRegenerator(NULL);
}

void RegenerateTexture(ITexture *tex)
{
	tex->Download();
}

int Start( lua_State* L )
{
	ILuaInterface *gLua = Lua();
	g_pLua = gLua;

	if(webCore != NULL)
		Close(L);

	webCore = new Awesomium::WebCore(Awesomium::LOG_VERBOSE, true, Awesomium::PF_BGRA);

	if(!webCore)
	{
		gLua->Error("Unable to create Awesomium webCore");
		return 0;
	}

	CreateInterfaceFn inputsystemFactory = Sys_GetFactory("inputsystem.dll");
	inputsystem = (IInputSystem *)inputsystemFactory(INPUTSYSTEM_INTERFACE_VERSION, NULL);

	if(!inputsystem)
	{
		gLua->Error("Unable to get " INPUTSYSTEM_INTERFACE_VERSION);
		return 0;
	}

	CreateInterfaceFn materialsystemFactory = Sys_GetFactory( "materialsystem.dll" );
	materialsystem = (IMaterialSystem*)materialsystemFactory( MATERIAL_SYSTEM_INTERFACE_VERSION, NULL );

	if(!materialsystem)
	{
		gLua->Error("Unable to get " MATERIAL_SYSTEM_INTERFACE_VERSION);
		return 0;
	}

	LuaBrowser_Init(gLua);
	return 0;
}

int Close( lua_State* L )
{
	LuaBrowser_Close(Lua());

	if(webCore)
		delete webCore;

	return 0;
}