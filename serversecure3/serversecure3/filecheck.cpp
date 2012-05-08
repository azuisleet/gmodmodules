
#include "filecheck.h"

#include "cbase.h"
#include "networkstringtabledefs.h"

#include "sigscan.h"
#include "detours.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


// _ZN8CNetChan22IsValidFileForTransferEPKc
#define SIG "\x55\x8B\xEC\x56\x8B\x75\x08\x85\xF6\x74\x2A\x80\x3E\x00"
#define SIG_MASK "xxxxxxxxxxxxxx"

static ConVar showFiles( "ss_show_files", "0", 0, "Display file checks" );

typedef bool ( *IsValidFileFn )( const char *file );


INetworkStringTableContainer *netstringtables = NULL;

IsValidFileFn validfile_trampoline = NULL;


bool Hook_IsValidFile( char *file )
{
	if ( file == NULL || V_strlen( file ) == 0 )
		return false;

	if ( showFiles.GetBool() )
		Msg( "[SS3] Checking file '%s'\n", file );

	bool bSafe = validfile_trampoline( file );

	if ( !bSafe )
		return false;

	INetworkStringTable *downloads = netstringtables->FindTable( "downloadables" );
	if ( downloads == NULL )
	{
		Msg( "[SS3] Missing 'downloadables' string table!\n" );
		return false;
	}

	int len = V_strlen( file );
	int index = downloads->FindStringIndex( file );

	if ( index == INVALID_STRING_INDEX && ( len > 5 && V_strncmp( file, "maps/", 5 ) == 0 ) )
	{
		for ( int i = 0 ; i < len ; i++ )
		{
			if ( file[ i ] == '/' )
				file[ i ] = '\\';
		}

		index = downloads->FindStringIndex( file );
	}

	if ( index != INVALID_STRING_INDEX )
	{
		return true;
	}

	if ( len == 22 && V_strncmp( file, "downloads/", 10 ) == 0 && V_strncmp( file + len - 4, ".dat", 4 ) == 0 )
	{
		return true;
	}

	Msg( "[SS3] Blocking download: '%s'\n", file );
	return false;
}

void FileCheck_Load()
{
	CreateInterfaceFn engineFactory = Sys_GetFactory( "engine.dll" );

	netstringtables = reinterpret_cast<INetworkStringTableContainer *>( engineFactory( INTERFACENAME_NETWORKSTRINGTABLESERVER, NULL ) );

	if ( netstringtables == NULL )
	{
		Msg( "[SS3] Unable to get INetworkStringTableContainer!\n" );
		return;
	}

	CSigScan::sigscan_dllfunc = engineFactory;
	CSigScan::GetDllMemInfo();

	CSigScan checkFileScan;

	checkFileScan.Init( reinterpret_cast<unsigned char *>( SIG ), SIG_MASK, strlen( SIG_MASK ) );

	if ( !checkFileScan.sig_addr )
	{
		Msg( "[SS3] Unable to scan IsValidFileForTransfer!\n" );
		return;
	}

	
	validfile_trampoline = reinterpret_cast<IsValidFileFn>( checkFileScan.sig_addr );

	DetourTransactionBegin();
	DetourUpdateThread( GetCurrentThread() );
	DetourAttach( &(PVOID&)validfile_trampoline, (PVOID)(&(PVOID&) Hook_IsValidFile ) );
	DetourTransactionCommit();

}

void FileCheck_Unload()
{
	if ( validfile_trampoline != NULL )
	{
		DetourTransactionBegin();
		DetourUpdateThread( GetCurrentThread() );
		DetourDetach( &(PVOID&)validfile_trampoline, (PVOID)(&(PVOID&) Hook_IsValidFile ) );
		DetourTransactionCommit();
	}
}