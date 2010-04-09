#define _RETAIL


#include "GMLuaModule.h"
#include "gm_slog.h"
#include "sigscan.h"

#include "interface.h"
#include "eiface.h"
#include "iclient.h"
#include "igameevents.h"

IVEngineServer	*engine = NULL;
IGameEventManager2 *gameeventmanager = NULL;

#include <windows.h>
#include <detours.h>
#include "vfnhook.h"
#include "tier1.h"

ILuaInterface *globalLua;

DEFVFUNC_(origFireEvent, bool, (IGameEventManager2 *gem, IGameEvent *event, bool bDontBroadcast));
bool VFUNC newFireEvent(IGameEventManager2 *gem, IGameEvent *event, bool bDontBroadcast = false)
{
	if(strcmp(event->GetName(), "player_disconnect") == 0)
	{
		event->SetString("reason", "Player dropped from server.");
	}

	return origFireEvent(gem, event, bDontBroadcast);
}

// virtual bool	ExecuteStringCommand( const char *s ) = 0;
class CDetour
{
public:
	bool ExecuteStringCommand(const char *s);
	static bool (CDetour::* ExecuteStringTrampoline)(const char *s);

};
typedef bool (__thiscall CDetour::* *execstringcmd_t)(const char *);

bool (CDetour::* CDetour::ExecuteStringTrampoline)(const char *s) = 0;

bool CDetour::ExecuteStringCommand(const char *s)
{
	IClient *cl = (IClient *)this;

	ILuaObject *func = globalLua->GetGlobal("Cmd_RecvCommand");
	if(func->isFunction())
	{
		globalLua->Push(func);

		globalLua->Push(cl->GetClientName());
		globalLua->Push(s);
		globalLua->Push(cl->GetNetworkIDString());

		globalLua->Call(3, 1);

		ILuaObject *returno = globalLua->GetReturn(0);
		bool ignore = returno->GetBool();

		if(ignore)
			return true;
	}

	return (this->*ExecuteStringTrampoline)(s);
}

#define RECVCMD "\x81\xEC\x08\x05\x00\x00\x56\x8B\xB4\x24\x10\x05\x00\x00"
#define RECVCMDMASK "xxxxxxxxxxxxxx"
#define RECVCMDLEN 14

CSigScan RecvCmd_Sig;

LUA_FUNCTION(AppendLog)
{
	ILuaInterface *gLua = Lua();
	const char *name = gLua->GetString(1);
	const char *buff = gLua->GetString(2);
	int len = gLua->StringLength(2);

	char buffer[MAX_PATH];
	_snprintf(buffer, sizeof(buffer), "garrysmod/commandlogs/%s.txt", name);

	HANDLE hFile = CreateFileA(buffer, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_FLAG_OVERLAPPED, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		gLua->Msg("Unable to open %s\n", buffer);
		CloseHandle(hFile);
		return 0;
	}

	OVERLAPPED o;
	memset(&o,0,sizeof(o));

	o.Offset = 0xffffffff;
	o.OffsetHigh = -1;

	DWORD dwBytesWritten;
	WriteFile(hFile, buff, len, &dwBytesWritten, &o);

	CloseHandle(hFile);

	return 0;
}

int Start(lua_State *L)
{
	CreateDirectoryA("garrysmod/commandlogs", NULL);

	ILuaInterface *gLua = Lua();

	CreateInterfaceFn interfaceFactory = Sys_GetFactory( "engine.dll" );
	engine = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL);
	gameeventmanager = (IGameEventManager2 *)interfaceFactory(INTERFACEVERSION_GAMEEVENTSMANAGER2, NULL);

	CSigScan::sigscan_dllfunc = (CreateInterfaceFn)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL);

	bool scan = CSigScan::GetDllMemInfo();

	RecvCmd_Sig.Init((unsigned char *)RECVCMD, RECVCMDMASK, RECVCMDLEN);
	CDetour::ExecuteStringTrampoline = *(execstringcmd_t)&RecvCmd_Sig.sig_addr;

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	DetourAttach(&(PVOID&)CDetour::ExecuteStringTrampoline,
		(PVOID)(&(PVOID&)CDetour::ExecuteStringCommand));

	DetourTransactionCommit();

	HOOKVFUNC(gameeventmanager, 7, origFireEvent, newFireEvent);

	gLua->SetGlobal("AppendLog", AppendLog);
	globalLua = gLua;

	return 0;
}

int Close(lua_State *L)
{
	UNHOOKVFUNC(gameeventmanager, 7, origFireEvent);

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	DetourDetach(&(PVOID&)CDetour::ExecuteStringTrampoline,
		(PVOID)(&(PVOID&)CDetour::ExecuteStringCommand));

	DetourTransactionCommit();

	return 0;
}