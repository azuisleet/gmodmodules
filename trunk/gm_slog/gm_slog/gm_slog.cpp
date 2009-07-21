#define _RETAIL


#include "GMLuaModule.h"
#include "gm_slog.h"
#include "sigscan.h"

#include "interface.h"
#include "eiface.h"

IVEngineServer	*engine = NULL;

#include <windows.h>
#include <detours.h>

// void	Cmd_RecvCommand()
typedef void (__fastcall *Cmd_RecvCommand_t)(void *a,  void *b, void *c);
void (__fastcall *Cmd_RecvCommand_Orig)(void *a,  void *b, void *c);
void __fastcall Cmd_RecvCommand_New(void *a,  void *b, void *c);

#define RECVCMD "\x81\xEC\x08\x05\x00\x00\x56\x8B\xB4\x24\x10\x05\x00\x00"
#define RECVCMDMASK "xxxxxxxxxxxxxx"
#define RECVCMDLEN 14

CSigScan RecvCmd_Sig;

struct cdata_t
{
	char unknown[20];
	char name[255];
};

ILuaInterface *globalLua;

void __fastcall Cmd_RecvCommand_New(void *a, void *b, void *c)
{
	const char *cmd;
	cdata_t *client;
	_asm mov cmd, edx;
	_asm mov client, ecx;

	ILuaObject *func = globalLua->GetGlobal("Cmd_RecvCommand");
	globalLua->Push(func);

	globalLua->Push(client->name);
	globalLua->Push(cmd);

	globalLua->Call(2, 1);

	ILuaObject *returno = globalLua->GetReturn(0);
	bool ignore = returno->GetBool();

	if(!ignore)
		Cmd_RecvCommand_Orig(a, b, c);
}

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
	CSigScan::sigscan_dllfunc = (CreateInterfaceFn)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL);

	bool scan = CSigScan::GetDllMemInfo();

	RecvCmd_Sig.Init((unsigned char *)RECVCMD, RECVCMDMASK, RECVCMDLEN);
	Cmd_RecvCommand_Orig = (Cmd_RecvCommand_t)RecvCmd_Sig.sig_addr;

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	DetourAttach((&(PVOID&)Cmd_RecvCommand_Orig), (&(PVOID&)Cmd_RecvCommand_New) );

	DetourTransactionCommit();

	gLua->SetGlobal("AppendLog", AppendLog);
	globalLua = gLua;

	return 0;
}

int Close(lua_State *L)
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	DetourDetach((&(PVOID&)Cmd_RecvCommand_Orig), (&(PVOID&)Cmd_RecvCommand_New) );

	DetourTransactionCommit();

	return 0;
}