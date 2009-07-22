#include <windows.h>
#include "GMLuaModule.h"
#include "gm_luaprof.h"
#include "lua.h"
#include <string>
#include <stack>
#include <map>

#include <detours.h>
#pragma comment(lib, "detours.lib")

// CPrecisionTimer class by alexey_1979
class CPrecisionTimer
{
public:
	double Stop()
	{
		LARGE_INTEGER curval;
		QueryPerformanceCounter(&curval);
		long double f = ToDouble(m_start);
		long double f1 = ToDouble(curval);
		long double f3 = ToDouble(m_freq);
		return (f1 - f)/f3;
	}
	long double ToDouble(LARGE_INTEGER& val) { long double f = val.u.HighPart; f = f * (DWORD)-1; f += val.u.LowPart; return f; }
	void Start() { QueryPerformanceCounter(&m_start); }
	CPrecisionTimer() { QueryPerformanceFrequency(&m_freq); }
	virtual ~CPrecisionTimer() {};
private:
	LARGE_INTEGER m_start;
	LARGE_INTEGER m_freq;
};

struct luaProfMap
{
	double avg, max;
	double numCalls;
};

struct luaProfStack
{
	std::string friendlyname;
	CPrecisionTimer timer;
};

std::map<std::string,  luaProfMap *> funcInfo;
std::stack<luaProfStack *> funcStack;

bool canHook = true;

static void luahook(lua_State *L, lua_Debug *ar)
{
	ILuaInterface *gLua = Lua();

	int currentline;
	lua_Debug previous_ar;

	if (lua_getstack(L, 1, &previous_ar) == 0) {
		currentline = -1;
	} else {
		lua_getinfo(L, "l", &previous_ar);
		currentline = previous_ar.currentline;
	}

	lua_getinfo(L, "nS", ar);

	char funcname[128];
	_snprintf(funcname, sizeof(funcname), "%s(%d)%s", ar->short_src, ar->linedefined, ar->name);
	std::string friendlyname = funcname;

	if(ar->event == LUA_HOOKCALL)
	{
		luaProfStack *item = new luaProfStack;
		item->timer.Start();
		item->friendlyname = friendlyname;
		funcStack.push(item);

	} else if (!funcStack.empty()) {
		luaProfStack *top = funcStack.top();
		funcStack.pop();

		double time = top->timer.Stop();

		std::string str = top->friendlyname;

		std::map<std::string, luaProfMap *>::iterator x = funcInfo.find(str);
		if(x == funcInfo.end())
		{
			luaProfMap *map = new luaProfMap;
			map->avg = time;
			map->max = time;
			map->numCalls = 1;
			
			//Lua()->Msg("func info %s %f\n", str.c_str(), time);
			funcInfo[str] = map;
		} else {
			std::pair<std::string, luaProfMap *> pair = *x;
			luaProfMap *map = pair.second;
			map->avg += time;
			map->avg /= 2;
			map->numCalls++;

			map->max = max(map->max, time);
		}

		delete top;
	}


}

void BuildTable(ILuaInterface *gLua)
{
	if(canHook) return;

	std::map<std::string, luaProfMap *>::iterator x;

	ILuaObject *table = gLua->GetNewTable();

	while(!funcStack.empty())
	{
		luaProfStack *top = funcStack.top();
		funcStack.pop();
		delete top;
	}

	for(x= funcInfo.begin() ; x != funcInfo.end() ; )
	{
		std::pair<std::string, luaProfMap *> pair = *x;
		luaProfMap *map = pair.second;
		std::string str = pair.first;

		const char *cstr = str.c_str();

		//gLua->Msg("func data %s %f %f\n", cstr, map->avg, map->max);

		ILuaObject *refn = gLua->GetNewTable();
			refn->SetMember("avg", (float)map->avg);
			refn->SetMember("max", (float)map->max);
			refn->SetMember("numCalls", (float)map->numCalls);
		table->SetMember(cstr, refn);
		refn->UnReference();

		funcInfo.erase(x++);
		delete map;
	}

	ILuaObject *prof = gLua->GetGlobal("profiler");
	prof->SetMember("FuncData", table);
	prof->UnReference();
	table->UnReference();
}

LUA_FUNCTION(StartProfiler)
{
	ILuaInterface *gLua = Lua();

	lua_sethook(L, luahook, LUA_MASKCALL | LUA_MASKRET, 0);
	canHook = false;
	return 0;
}

LUA_FUNCTION(StopProfiler)
{
	BuildTable(Lua());

	lua_sethook(L, NULL, LUA_MASKCALL | LUA_MASKRET, 0);
	canHook = true;
	return 0;
}

int (*lua_sethook_trampoline)(lua_State *L, lua_Hook func, int mask, int count) = 0;
int lua_newhook (lua_State *L, lua_Hook func, int mask, int count)
{
	if(canHook)
		return lua_sethook_trampoline(L, func, mask, count);
	return 0;
}


int Start(lua_State *L)
{
	ILuaInterface *gLua = Lua();

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	lua_sethook_trampoline = lua_sethook;

	DetourAttach(&(PVOID&)lua_sethook_trampoline,
		(PVOID)(&(PVOID&)lua_newhook));

	DetourTransactionCommit();

	gLua->SetGlobal("StartProfiler", StartProfiler);
	gLua->SetGlobal("StopProfiler", StopProfiler);

	return 0;
}

int Close(lua_State *L)
{
	if(!canHook)
		StopProfiler(L);

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	DetourDetach(&(PVOID&)lua_sethook_trampoline,
		(PVOID)(&(PVOID&)lua_newhook));

	DetourTransactionCommit();

	return 0;
}