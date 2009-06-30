#include "bass.h"
#include "tags.h"
#include "mutex.h"
#include "list.h"

GMOD_MODULE(Start, Close)

int CreateChannel(ILuaInterface *gLua, DWORD flags);
LUA_FUNCTION(channel_gc);
LUA_FUNCTION(channel_play);
LUA_FUNCTION(channel_pause);
LUA_FUNCTION(channel_stop);
LUA_FUNCTION(channel_getlength);
LUA_FUNCTION(channel_gettag);
LUA_FUNCTION(channel_getplaying);
LUA_FUNCTION(channel_getposition);
LUA_FUNCTION(channel_getlevel);
LUA_FUNCTION(channel_fft2048);
LUA_FUNCTION(channel_set3dposition);
LUA_FUNCTION(channel_getrawtag);

LUA_FUNCTION(bass_setposition);
LUA_FUNCTION(bass_streamfile);
LUA_FUNCTION(bass_streamfileurl);
