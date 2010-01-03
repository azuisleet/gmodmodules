#define _RETAIL 1
#define GAME_DLL 1
#define WIN32_LEAN_AND_MEAN

#include "GMLuaModule.h"

#define CRYPTOPP_DEFAULT_NO_DLL
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include "dll.h"
#include "md5.h"
