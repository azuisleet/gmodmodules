#ifdef _WIN32
#pragma once
#endif

#define _RETAIL
#include "GMLuaModule.h"

#include <stack>

#include <WebCore.h>

#include <vgui/ISurface.h>
#include <inputsystem/iinputsystem.h>
#include <vgui/IPanel.h>
#include <vgui_controls/Panel.h>

#include <KeyValues.h>

#define WIN32_LEAN_AND_MEAN
#include "windows.h"

#include <d3d9.h>
#include <d3dx9.h>