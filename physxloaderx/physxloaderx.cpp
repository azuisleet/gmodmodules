#include "physxloaderx.h"
#include <stdio.h>

NXPHYSXLOADERDLL_API NxPhysicsSDK* NX_CALL_CONV NxCreatePhysicsSDK(NxU32 sdkVersion, NxUserAllocator* allocator, NxUserOutputStream* outputStream, const NxPhysicsSDKDesc& desc, NxSDKCreateError* errorCode)
{
	NxPhysicsSDK *ret = NxCreatePhysicsSDKx(sdkVersion, allocator, outputStream, desc, errorCode);
	NxPhysicsSDKx *hret = new NxPhysicsSDKx(ret);

	return hret;
}

NXPHYSXLOADERDLL_API NxPhysicsSDK* NX_CALL_CONV NxCreatePhysicsSDKWithID(NxU32 sdkVersion, char *companyNameStr, char *appNameStr, char *appVersionStr, char *appUserDefinedStr, NxUserAllocator* allocator, NxUserOutputStream* outputStream, const NxPhysicsSDKDesc &desc, NxSDKCreateError* errorCode)
{
	NxPhysicsSDK *ret = NxCreatePhysicsSDKWithIDx(sdkVersion, companyNameStr, appNameStr, appVersionStr, appUserDefinedStr, allocator, outputStream, desc, errorCode);
	NxPhysicsSDKx *hret = new NxPhysicsSDKx(ret);

	return hret;
}

NXPHYSXLOADERDLL_API void NX_CALL_CONV NxReleasePhysicsSDK(NxPhysicsSDK* sdk)
{
	sdk->getFoundationSDK().getRemoteDebugger()->flush();
	sdk->getFoundationSDK().getRemoteDebugger()->disconnect();

	NxReleasePhysicsSDKx(sdk);
}

NXPHYSXLOADERDLL_API NxUserAllocator* NX_CALL_CONV NxGetPhysicsSDKAllocator()
{
	return NxGetPhysicsSDKAllocatorx();
}

NXPHYSXLOADERDLL_API NxFoundationSDK* NX_CALL_CONV NxGetFoundationSDK()
{
	return NxGetFoundationSDKx();
}

NXPHYSXLOADERDLL_API NxPhysicsSDK* NX_CALL_CONV NxGetPhysicsSDK()
{
	return NxGetPhysicsSDKx();
}

NXPHYSXLOADERDLL_API NxUtilLib* NX_CALL_CONV NxGetUtilLib()
{
	return NxGetUtilLibx();
}

NXPHYSXLOADERDLL_API NxCookingInterface* NX_CALL_CONV NxGetCookingLib(NxU32 sdk_version_number)
{
	return NxGetCookingLibx(sdk_version_number);
}

NXPHYSXLOADERDLL_API NxCookingInterface* NX_CALL_CONV NxGetCookingLibWithID(NxU32 sdk_version_number, char *companyNameStr, char *appNameStr, char *appVersionStr,	char *appUserDefinedStr)
{
	return NxGetCookingLibWithIDx(sdk_version_number, companyNameStr, appNameStr, appVersionStr, appUserDefinedStr);
}