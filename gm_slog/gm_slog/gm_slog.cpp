#include "GMLuaModule.h"
#include "gm_slog.h"
#include "sigscan.h"

#include "interface.h"
#include "eiface.h"
#include "tmpclient.h"

IVEngineServer *engine = NULL;

#include <windows.h>
#include <detours.h>
#include "vfnhook.h"
#include "tier1.h"

ILuaInterface *globalLua;

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

#define RECVCMD "\x8B\x44\x24\x04\x85\xC0\x56\x8B\xF1\x74"
#define RECVCMDMASK "xxxxxxxxxx"
#define RECVCMDLEN 10

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
	ILuaInterface *gLua = Lua();

	CreateInterfaceFn interfaceFactory = Sys_GetFactory( "engine.dll" );
	engine = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL);

	CSigScan::sigscan_dllfunc = (CreateInterfaceFn)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL);

	bool scan = CSigScan::GetDllMemInfo();

	RecvCmd_Sig.Init((unsigned char *)RECVCMD, RECVCMDMASK, RECVCMDLEN);

	if (!RecvCmd_Sig.sig_addr)
	{
		gLua->Error("[gm_slog] CBaseClient::ExecuteStringCommand not found");
		return 0;
	}

	CDetour::ExecuteStringTrampoline = *(execstringcmd_t)&RecvCmd_Sig.sig_addr;

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	DetourAttach(&(PVOID&)CDetour::ExecuteStringTrampoline,
		(PVOID)(&(PVOID&)CDetour::ExecuteStringCommand));

	DetourTransactionCommit();

	gLua->SetGlobal("AppendLog", AppendLog);
	globalLua = gLua;

	CreateDirectoryA("garrysmod/commandlogs", NULL);

	return 0;
}

int Close(lua_State *L)
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	DetourDetach(&(PVOID&)CDetour::ExecuteStringTrampoline,
		(PVOID)(&(PVOID&)CDetour::ExecuteStringCommand));

	DetourTransactionCommit();

	return 0;
}