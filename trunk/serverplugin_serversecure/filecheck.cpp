#include "plugin.h"
#include "sigscan.h"
#include "networkstringtabledefs.h"

//_ZN8CNetChan22IsValidFileForTransferEPKc
#define SIG_CHECKFILE "\x56\x8B\x74\x24\x08\x85\xF6\x74\x12\x80"
#define SIG_CHECKFILEMASK "xxxxxxxxxx"
#define SIG_CHECKFILELEN 10
CSigScan sigcheckfileext_Sig;

typedef int (*checkext_t)(const char *);
int (*checkext_trampoline)(const char *file) = 0;

INetworkStringTableContainer *netstringtables;

int checkext_hook(char *filename)
{
	if(filename == NULL)
		return 0;

	int safe = checkext_trampoline(filename);
	if(!safe)
		return 0;

	INetworkStringTable *downloads = netstringtables->FindTable("downloadables");
	if(downloads == NULL)
	{
		Msg("Missing downloadables string table\n");
		return 0;
	}
	
	int len = strlen(filename);
	int index = downloads->FindStringIndex(filename);

	if(index == INVALID_STRING_INDEX && (len > 5 && strncmp(filename, "maps/", 5) == 0))
	{
		for(int i = 0; i < len; i++)
		{
			if(filename[i] == '/')
				filename[i] = '\\';
		}

		index = downloads->FindStringIndex(filename);
	}

	if(index != INVALID_STRING_INDEX)
	{
		return safe;
	}

	if(len == 22 && strncmp(filename, "downloads/", 10) == 0 && strncmp(filename + len - 4, ".dat", 4) == 0)
	{
		return safe;
	}

	return 0;
}

void FileFilter_Load()
{
	CreateInterfaceFn engineFactory =  Sys_GetFactory("engine.dll");

	netstringtables = (INetworkStringTableContainer *)engineFactory(INTERFACENAME_NETWORKSTRINGTABLESERVER, NULL);
	if(netstringtables == NULL)
	{
		Msg("Unable to find string tables!\n");
		return;
	}

	CSigScan::sigscan_dllfunc = engineFactory;
	bool scan = CSigScan::GetDllMemInfo();

	sigcheckfileext_Sig.Init((unsigned char *)SIG_CHECKFILE, SIG_CHECKFILEMASK, SIG_CHECKFILELEN);

	Msg("CheckFile signature %x\n", sigcheckfileext_Sig.sig_addr);

	if(sigcheckfileext_Sig.sig_addr)
	{
		checkext_trampoline = (checkext_t)sigcheckfileext_Sig.sig_addr;

		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
			DetourAttach(&(PVOID&)checkext_trampoline, (PVOID)(&(PVOID&)checkext_hook));
		DetourTransactionCommit();
	}
}

void FileFilter_Unload()
{
	if(sigcheckfileext_Sig.sig_addr)
	{
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
			DetourDetach(&(PVOID&)checkext_trampoline, (PVOID)(&(PVOID&)checkext_hook));
		DetourTransactionCommit();
	}
}