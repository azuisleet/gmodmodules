#define NOMINMAX
#include <windows.h>
#include <physxloader.h>
#include <NxScene.h>
#include "nxphysicssdkx.h"
#include "nxscenex.h"

typedef NxPhysicsSDK* (*NxCreatePhysicsSDK_t)(NxU32 sdkVersion, NxUserAllocator* allocator, NxUserOutputStream* outputStream, const NxPhysicsSDKDesc& desc, NxSDKCreateError* errorCode);
typedef NxPhysicsSDK* (*NxCreatePhysicsSDKWithID_t)(NxU32 sdkVersion, char *companyNameStr, char *appNameStr, char *appVersionStr, char *appUserDefinedStr, NxUserAllocator* allocator, NxUserOutputStream* outputStream, const NxPhysicsSDKDesc &desc, NxSDKCreateError* errorCode);
typedef void (*NxReleasePhysicsSDK_t)(NxPhysicsSDK* sdk);
typedef NxUserAllocator* (*NxGetPhysicsSDKAllocator_t)(void);
typedef NxFoundationSDK* (*NxGetFoundationSDK_t)(void);
typedef NxPhysicsSDK* (*NxGetPhysicsSDK_t)(void);
typedef NxUtilLib* (*NxGetUtilLib_t)(void);
typedef NxCookingInterface* (*NxGetCookingLib_t)(NxU32 sdk_version_number);
typedef NxCookingInterface* (*NxGetCookingLibWithID_t)(NxU32 sdk_version_number, char *companyNameStr, char *appNameStr, char *appVersionStr,	char *appUserDefinedStr);

extern HMODULE physx;
extern NxCreatePhysicsSDK_t NxCreatePhysicsSDKx;
extern NxCreatePhysicsSDKWithID_t NxCreatePhysicsSDKWithIDx;
extern NxReleasePhysicsSDK_t NxReleasePhysicsSDKx;
extern NxGetPhysicsSDKAllocator_t NxGetPhysicsSDKAllocatorx;
extern NxGetFoundationSDK_t NxGetFoundationSDKx;
extern NxGetPhysicsSDK_t NxGetPhysicsSDKx;
extern NxGetUtilLib_t NxGetUtilLibx;
extern NxGetCookingLib_t NxGetCookingLibx;
extern NxGetCookingLibWithID_t NxGetCookingLibWithIDx;