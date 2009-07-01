#include "physxloaderx.h"

NxPhysicsSDKx::NxPhysicsSDKx(NxPhysicsSDK *ptr)
{
	myptr = ptr;
	
	if(!ptr->getFoundationSDK().getRemoteDebugger()->isConnected())
	{
		MessageBoxA(0, "attempting connection", "debugger", 0);
		ptr->getFoundationSDK().getRemoteDebugger()->connect("127.0.0.1");
	}
}

void NxPhysicsSDKx::release()
{
	myptr->release();
}

bool NxPhysicsSDKx::setParameter(NxParameter paramEnum, NxReal paramValue)
{
	return myptr->setParameter(paramEnum, paramValue);
}

NxReal NxPhysicsSDKx::getParameter(NxParameter paramEnum) const
{
	return myptr->getParameter(paramEnum);
}

NxScene* NxPhysicsSDKx::createScene(const NxSceneDesc& sceneDesc)
{
	NxScene *ret = myptr->createScene(sceneDesc);
	NxScenex *hret = new NxScenex(ret);
	
	return hret;
}

void NxPhysicsSDKx::releaseScene(NxScene& scene)
{
	myptr->releaseScene(scene);
}

NxU32 NxPhysicsSDKx::getNbScenes() const
{
	return myptr->getNbScenes();
}

NxScene* NxPhysicsSDKx::getScene(NxU32 i)
{
	return myptr->getScene(i);
}

NxTriangleMesh* NxPhysicsSDKx::createTriangleMesh(const NxStream& stream)
{
	return myptr->createTriangleMesh(stream);
}

void NxPhysicsSDKx::releaseTriangleMesh(NxTriangleMesh& mesh)
{
	myptr->releaseTriangleMesh(mesh);
}

NxU32 NxPhysicsSDKx::getNbTriangleMeshes() const
{
	return myptr->getNbTriangleMeshes();
}

NxHeightField* NxPhysicsSDKx::createHeightField(const NxHeightFieldDesc& desc)
{
	return myptr->createHeightField(desc);
}

void NxPhysicsSDKx::releaseHeightField(NxHeightField& heightField)
{
	myptr->releaseHeightField(heightField);
}

NxU32 NxPhysicsSDKx::getNbHeightFields() const
{
	return myptr->getNbHeightFields();
}

NxCCDSkeleton* NxPhysicsSDKx::createCCDSkeleton(const NxSimpleTriangleMesh& mesh)
{
	return myptr->createCCDSkeleton(mesh);
}

NxCCDSkeleton* NxPhysicsSDKx::createCCDSkeleton(const void* memoryBuffer, NxU32 bufferSize)
{
	return myptr->createCCDSkeleton(memoryBuffer, bufferSize);
}

void NxPhysicsSDKx::releaseCCDSkeleton(NxCCDSkeleton& skel)
{
	myptr->releaseCCDSkeleton(skel);
}

NxU32 NxPhysicsSDKx::getNbCCDSkeletons() const
{
	return myptr->getNbCCDSkeletons();
}

NxConvexMesh* NxPhysicsSDKx::createConvexMesh(const NxStream& mesh)
{
	return myptr->createConvexMesh(mesh);
}

void NxPhysicsSDKx::releaseConvexMesh(NxConvexMesh& mesh)
{
	myptr->releaseConvexMesh(mesh);
}

NxU32 NxPhysicsSDKx::getNbConvexMeshes() const
{
	return myptr->getNbConvexMeshes();
}

NxClothMesh* NxPhysicsSDKx::createClothMesh(NxStream& stream)
{
	return myptr->createClothMesh(stream);
}

void NxPhysicsSDKx::releaseClothMesh(NxClothMesh& cloth)
{
	myptr->releaseClothMesh(cloth);
}

NxU32 NxPhysicsSDKx::getNbClothMeshes() const
{
	return myptr->getNbClothMeshes();
}

NxClothMesh** NxPhysicsSDKx::getClothMeshes()
{
	return myptr->getClothMeshes();
}

NxSoftBodyMesh* NxPhysicsSDKx::createSoftBodyMesh(NxStream& stream)
{
	return myptr->createSoftBodyMesh(stream);
}

void NxPhysicsSDKx::releaseSoftBodyMesh(NxSoftBodyMesh& softBodyMesh)
{
	myptr->releaseSoftBodyMesh(softBodyMesh);
}

NxU32 NxPhysicsSDKx::getNbSoftBodyMeshes() const
{
	return myptr->getNbSoftBodyMeshes();
}

NxSoftBodyMesh** NxPhysicsSDKx::getSoftBodyMeshes()
{
	return myptr->getSoftBodyMeshes();
}

NxU32 NxPhysicsSDKx::getInternalVersion(NxU32& apiRev, NxU32& descRev, NxU32& branchId) const
{
	return myptr->getInternalVersion(apiRev, descRev, branchId);
}

NxInterface* NxPhysicsSDKx::getInterface(NxInterfaceType type, int versionNumber)
{
	return myptr->getInterface(type, versionNumber);
}

NxHWVersion NxPhysicsSDKx::getHWVersion() const
{
	return myptr->getHWVersion();
}

NxU32 NxPhysicsSDKx::getNbPPUs() const
{
	return myptr->getNbPPUs();
}

NxFoundationSDK& NxPhysicsSDKx::getFoundationSDK() const
{
	return myptr->getFoundationSDK();
}
