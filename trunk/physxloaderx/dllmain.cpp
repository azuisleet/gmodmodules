#include "physxloaderx.h"

HMODULE physx;
NxCreatePhysicsSDK_t NxCreatePhysicsSDKx;
NxCreatePhysicsSDKWithID_t NxCreatePhysicsSDKWithIDx;
NxReleasePhysicsSDK_t NxReleasePhysicsSDKx;
NxGetPhysicsSDKAllocator_t NxGetPhysicsSDKAllocatorx;
NxGetFoundationSDK_t NxGetFoundationSDKx;
NxGetPhysicsSDK_t NxGetPhysicsSDKx;
NxGetUtilLib_t NxGetUtilLibx;
NxGetCookingLib_t NxGetCookingLibx;
NxGetCookingLibWithID_t NxGetCookingLibWithIDx;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		physx = LoadLibraryA("PhysXLoader_original.dll");

		NxCreatePhysicsSDKx = (NxCreatePhysicsSDK_t)GetProcAddress(physx, "NxCreatePhysicsSDK");
		NxCreatePhysicsSDKWithIDx = (NxCreatePhysicsSDKWithID_t)GetProcAddress(physx, "NxCreatePhysicsSDKWithID");
		NxReleasePhysicsSDKx = (NxReleasePhysicsSDK_t)GetProcAddress(physx, "NxReleasePhysicsSDK");
		NxGetPhysicsSDKAllocatorx = (NxGetPhysicsSDKAllocator_t)GetProcAddress(physx, "NxGetPhysicsSDKAllocator");
		NxGetFoundationSDKx = (NxGetFoundationSDK_t)GetProcAddress(physx, "NxGetFoundationSDK");
		NxGetPhysicsSDKx = (NxGetPhysicsSDK_t)GetProcAddress(physx, "NxGetPhysicsSDK");
		NxGetUtilLibx = (NxGetUtilLib_t)GetProcAddress(physx, "NxGetUtilLib");
		NxGetCookingLibx = (NxGetCookingLib_t)GetProcAddress(physx, "NxGetCookingLib");
		NxGetCookingLibWithIDx = (NxGetCookingLibWithID_t)GetProcAddress(physx, "NxGetCookingLibWithID");
	break;
	case DLL_PROCESS_DETACH:
		FreeLibrary(physx);
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}

