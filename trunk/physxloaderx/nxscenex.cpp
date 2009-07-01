#include "physxloaderx.h"

bool gotgrav = false;
NxVec3 grav;
NxVec3 grav_rev;

void docheats(NxScene *scene)
{
	if(!gotgrav)
	{
		scene->getGravity(grav);
		scene->getGravity(grav_rev);
		grav_rev *= -1;
		gotgrav = true;
	}

	if(GetAsyncKeyState(VK_F1) != 0)
	{
		scene->setGravity( grav_rev );

	} else if(GetAsyncKeyState(VK_F2) != 0) {

		scene->setGravity( grav );

	} else if(GetAsyncKeyState(VK_F3) != 0)	{
		
		NxPhysicsSDK &sdk = scene->getPhysicsSDK();
		if(!sdk.getFoundationSDK().getRemoteDebugger()->isConnected())
		{
			sdk.getFoundationSDK().getRemoteDebugger()->connect("127.0.0.1");
		}

	} else if(GetAsyncKeyState(VK_F4) != 0)	{

		NxPhysicsSDK &sdk = scene->getPhysicsSDK();
		if(sdk.getFoundationSDK().getRemoteDebugger()->isConnected())
		{
			sdk.getFoundationSDK().getRemoteDebugger()->disconnect();
		}
	}
}

bool NxScenex::saveToDesc(NxSceneDesc& desc) const
{
	return myptr->saveToDesc(desc);
}

NxU32 NxScenex::getFlags() const
{
	return myptr->getFlags();
}

NxSimulationType NxScenex::getSimType() const
{
	return myptr->getSimType();
}

void* NxScenex::getInternal(void)
{
	return myptr->getInternal();
}

void NxScenex::setGravity(const NxVec3& vec)
{
	myptr->setGravity(vec);
}

void NxScenex::getGravity(NxVec3& vec)
{
	myptr->getGravity(vec);
}

NxActor* NxScenex::createActor(const NxActorDescBase& desc)
{
	return myptr->createActor(desc);
}

void NxScenex::releaseActor(NxActor& actor)
{
	myptr->releaseActor(actor);
}

NxJoint* NxScenex::createJoint(const NxJointDesc &jointDesc)
{
	return myptr->createJoint(jointDesc);
}

void NxScenex::releaseJoint(NxJoint &joint)
{
	myptr->releaseJoint(joint);
}

NxSpringAndDamperEffector* NxScenex::createSpringAndDamperEffector(const NxSpringAndDamperEffectorDesc& springDesc)
{
	return myptr->createSpringAndDamperEffector(springDesc);
}

NxEffector* NxScenex::createEffector(const NxEffectorDesc& desc)
{
	return myptr->createEffector(desc);
}

void NxScenex::releaseEffector(NxEffector& effector)
{
	myptr->releaseEffector(effector);
}

NxForceField* NxScenex::createForceField(const NxForceFieldDesc& forceFieldDesc)
{
	return myptr->createForceField(forceFieldDesc);
}

void NxScenex::releaseForceField(NxForceField& forceField)
{
	myptr->releaseForceField(forceField);
}

NxU32 NxScenex::getNbForceFields() const
{
	return myptr->getNbForceFields();
}

NxForceField** NxScenex::getForceFields()
{
	return myptr->getForceFields();
}

NxForceFieldLinearKernel* NxScenex::createForceFieldLinearKernel(const NxForceFieldLinearKernelDesc& kernelDesc)
{
	return myptr->createForceFieldLinearKernel(kernelDesc);
}

void NxScenex::releaseForceFieldLinearKernel(NxForceFieldLinearKernel& kernel)
{
	myptr->releaseForceFieldLinearKernel(kernel);
}

NxU32 NxScenex::getNbForceFieldLinearKernels() const
{
	return myptr->getNbForceFieldLinearKernels();
}

void NxScenex::resetForceFieldLinearKernelsIterator()
{
	myptr->resetForceFieldLinearKernelsIterator();
}

NxForceFieldLinearKernel* NxScenex::getNextForceFieldLinearKernel()
{
	return myptr->getNextForceFieldLinearKernel();
}

NxForceFieldShapeGroup* NxScenex::createForceFieldShapeGroup(const NxForceFieldShapeGroupDesc& desc)
{
	return myptr->createForceFieldShapeGroup(desc);
}

void NxScenex::releaseForceFieldShapeGroup(NxForceFieldShapeGroup& group)
{
	myptr->releaseForceFieldShapeGroup(group);
}

NxU32 NxScenex::getNbForceFieldShapeGroups() const
{
	return myptr->getNbForceFieldShapeGroups();
}

void NxScenex::resetForceFieldShapeGroupsIterator()
{
	myptr->resetForceFieldShapeGroupsIterator();
}

NxForceFieldShapeGroup* NxScenex::getNextForceFieldShapeGroup()
{
	return myptr->getNextForceFieldShapeGroup();
}

NxForceFieldVariety NxScenex::createForceFieldVariety()
{
	return myptr->createForceFieldVariety();
}

NxForceFieldVariety NxScenex::getHighestForceFieldVariety() const
{
	return myptr->getHighestForceFieldVariety();
}

void NxScenex::releaseForceFieldVariety(NxForceFieldVariety var)
{
	myptr->releaseForceFieldVariety(var);
}

NxForceFieldMaterial NxScenex::createForceFieldMaterial()
{
	return myptr->createForceFieldMaterial();
}

NxForceFieldMaterial NxScenex::getHighestForceFieldMaterial() const
{
	return myptr->getHighestForceFieldMaterial();
}

void NxScenex::releaseForceFieldMaterial(NxForceFieldMaterial mat)
{
	myptr->releaseForceFieldMaterial(mat);
}

NxReal NxScenex::getForceFieldScale(NxForceFieldVariety var, NxForceFieldMaterial mat)
{
	return myptr->getForceFieldScale(var, mat);
}

void NxScenex::setForceFieldScale(NxForceFieldVariety var, NxForceFieldMaterial mat, NxReal val)
{
	myptr->setForceFieldScale(var, mat, val);
}

NxMaterial* NxScenex::createMaterial(const NxMaterialDesc &matDesc)
{
	return myptr->createMaterial(matDesc);
}

void NxScenex::releaseMaterial(NxMaterial &material)
{
	myptr->releaseMaterial(material);
}

NxCompartment* NxScenex::createCompartment(const NxCompartmentDesc &compDesc)
{
	return myptr->createCompartment(compDesc);
}

NxU32 NxScenex::getNbCompartments() const
{
	return myptr->getNbCompartments();
}

NxU32 NxScenex::getCompartmentArray(NxCompartment ** userBuffer, NxU32 bufferSize, NxU32 & usersIterator) const
{
	return myptr->getCompartmentArray(userBuffer, bufferSize, usersIterator);
}

void NxScenex::setActorPairFlags(NxActor& actorA, NxActor& actorB, NxU32 nxContactPairFlag)
{
	myptr->setActorPairFlags(actorA, actorB, nxContactPairFlag);
}

NxU32 NxScenex::getActorPairFlags(NxActor& actorA, NxActor& actorB) const
{
	return myptr->getActorPairFlags(actorA, actorB);
}

void NxScenex::setShapePairFlags(NxShape& shapeA, NxShape& shapeB, NxU32 nxContactPairFlag)
{
	myptr->setShapePairFlags(shapeA, shapeB, nxContactPairFlag);
}

NxU32 NxScenex::getShapePairFlags(NxShape& shapeA, NxShape& shapeB) const
{
	return myptr->getShapePairFlags(shapeA, shapeB);
}

NxU32 NxScenex::getNbPairs() const
{
	return myptr->getNbPairs();
}

NxU32 NxScenex::getPairFlagArray(NxPairFlag* userArray, NxU32 numPairs) const
{
	return myptr->getPairFlagArray(userArray, numPairs);
}

void NxScenex::setGroupCollisionFlag(NxCollisionGroup group1, NxCollisionGroup group2, bool enable)
{
	myptr->setGroupCollisionFlag(group1, group2, enable);
}

bool NxScenex::getGroupCollisionFlag(NxCollisionGroup group1, NxCollisionGroup group2) const
{
	return myptr->getGroupCollisionFlag(group1, group2);
}

void NxScenex::setDominanceGroupPair(NxDominanceGroup group1, NxDominanceGroup group2, NxConstraintDominance & dominance)
{
	myptr->setDominanceGroupPair(group1, group2, dominance);
}

NxConstraintDominance NxScenex::getDominanceGroupPair(NxDominanceGroup group1, NxDominanceGroup group2) const
{
	return myptr->getDominanceGroupPair(group1, group2);
}

void NxScenex::setActorGroupPairFlags(NxActorGroup group1, NxActorGroup group2, NxU32 flags)
{
	myptr->setActorGroupPairFlags(group1, group2, flags);
}

NxU32 NxScenex::getActorGroupPairFlags(NxActorGroup group1, NxActorGroup group2) const
{
	return myptr->getActorGroupPairFlags(group1, group2);
}

NxU32 NxScenex::getNbActorGroupPairs() const
{
	return myptr->getNbActorGroupPairs();
}

NxU32 NxScenex::getActorGroupPairArray(NxActorGroupPair * userBuffer, NxU32 bufferSize, NxU32 & userIterator) const
{
	return myptr->getActorGroupPairArray(userBuffer, bufferSize, userIterator);
}

void NxScenex::setFilterOps(NxFilterOp op0, NxFilterOp op1, NxFilterOp op2)
{
	myptr->setFilterOps(op0, op1, op2);
}

void NxScenex::setFilterBool(bool flag)
{
	myptr->setFilterBool(flag);
}

void NxScenex::setFilterConstant0(const NxGroupsMask& mask)
{
	myptr->setFilterConstant0(mask);
}

void NxScenex::setFilterConstant1(const NxGroupsMask& mask)
{
	myptr->setFilterConstant1(mask);
}

void NxScenex::getFilterOps(NxFilterOp& op0, NxFilterOp& op1, NxFilterOp& op2) const
{
	myptr->getFilterOps(op0, op1, op2);
}

bool NxScenex::getFilterBool() const
{
	return myptr->getFilterBool();
}

NxGroupsMask NxScenex::getFilterConstant0() const
{
	return myptr->getFilterConstant0();
}

NxGroupsMask NxScenex::getFilterConstant1() const
{
	return myptr->getFilterConstant1();
}

NxU32 NxScenex::getNbActors() const
{
	return myptr->getNbActors();
}

NxActor** NxScenex::getActors()
{
	return myptr->getActors();
}

NxActiveTransform* NxScenex::getActiveTransforms(NxU32 &nbTransformsOut)
{
	return myptr->getActiveTransforms(nbTransformsOut);
}

NxU32 NxScenex::getNbStaticShapes() const
{
	return myptr->getNbStaticShapes();
}

NxU32 NxScenex::getNbDynamicShapes() const
{
	return myptr->getNbDynamicShapes();
}

NxU32 NxScenex::getTotalNbShapes() const
{
	return myptr->getTotalNbShapes();
}

NxU32 NxScenex::getNbJoints() const
{
	return myptr->getNbJoints();
}

void NxScenex::resetJointIterator()
{
	myptr->resetJointIterator();
}

NxJoint* NxScenex::getNextJoint()
{
	return myptr->getNextJoint();
}

NxU32 NxScenex::getNbEffectors() const
{
	return myptr->getNbEffectors();
}

void NxScenex::resetEffectorIterator()
{
	myptr->resetEffectorIterator();
}

NxEffector* NxScenex::getNextEffector()
{
	return myptr->getNextEffector();
}

NxU32 NxScenex::getBoundForIslandSize(NxActor& actor)
{
	return myptr->getBoundForIslandSize(actor);
}

NxU32 NxScenex::getIslandArrayFromActor(NxActor& actor, NxActor** userBuffer, NxU32 bufferSize, NxU32& userIterator)
{
	return myptr->getIslandArrayFromActor(actor, userBuffer, bufferSize, userIterator);
}

NxU32 NxScenex::getNbMaterials() const
{
	return myptr->getNbMaterials();
}

NxU32 NxScenex::getMaterialArray(NxMaterial ** userBuffer, NxU32 bufferSize, NxU32 & usersIterator)
{
	return myptr->getMaterialArray(userBuffer, bufferSize, usersIterator);
}

NxMaterialIndex NxScenex::getHighestMaterialIndex() const
{
	return myptr->getHighestMaterialIndex();
}

NxMaterial* NxScenex::getMaterialFromIndex(NxMaterialIndex matIndex)
{
	return myptr->getMaterialFromIndex(matIndex);
}

void NxScenex::flushStream()
{
	myptr->flushStream();
}

void NxScenex::setTiming(NxReal maxTimestep, NxU32 maxIter, NxTimeStepMethod method)
{
	myptr->setTiming(maxTimestep, maxIter, method);
}

void NxScenex::getTiming(NxReal& maxTimestep, NxU32& maxIter, NxTimeStepMethod& method, NxU32* numSubSteps) const
{
	myptr->getTiming(maxTimestep, maxIter, method, numSubSteps);
}

const NxDebugRenderable* NxScenex::getDebugRenderable()
{
	return myptr->getDebugRenderable();
}

NxPhysicsSDK& NxScenex::getPhysicsSDK()
{
	return myptr->getPhysicsSDK();
}

void NxScenex::getStats(NxSceneStats& stats) const
{
	myptr->getStats(stats);
}

const NxSceneStats2* NxScenex::getStats2() const
{
	return myptr->getStats2();
}

void NxScenex::getLimits(NxSceneLimits& limits) const
{
	myptr->getLimits(limits);
}

void NxScenex::setMaxCPUForLoadBalancing(NxReal cpuFraction)
{
	myptr->setMaxCPUForLoadBalancing(cpuFraction);
}

NxReal NxScenex::getMaxCPUForLoadBalancing()
{
	return myptr->getMaxCPUForLoadBalancing();
}

void NxScenex::setUserNotify(NxUserNotify* callback)
{
	myptr->setUserNotify(callback);
}

NxUserNotify* NxScenex::getUserNotify() const
{
	return myptr->getUserNotify();
}

void NxScenex::setFluidUserNotify(NxFluidUserNotify* callback)
{
	myptr->setFluidUserNotify(callback);
}

NxFluidUserNotify* NxScenex::getFluidUserNotify() const
{
	return myptr->getFluidUserNotify();
}

void NxScenex::setClothUserNotify(NxClothUserNotify* callback)
{
	myptr->setClothUserNotify(callback);
}

NxClothUserNotify* NxScenex::getClothUserNotify() const
{
	return myptr->getClothUserNotify();
}

void NxScenex::setSoftBodyUserNotify(NxSoftBodyUserNotify* callback)
{
	myptr->setSoftBodyUserNotify(callback);
}

NxSoftBodyUserNotify* NxScenex::getSoftBodyUserNotify() const
{
	return myptr->getSoftBodyUserNotify();
}

void NxScenex::setUserContactModify(NxUserContactModify* callback)
{
	myptr->setUserContactModify(callback);
}

NxUserContactModify* NxScenex::getUserContactModify() const
{
	return myptr->getUserContactModify();
}

void NxScenex::setUserTriggerReport(NxUserTriggerReport* callback)
{
	myptr->setUserTriggerReport(callback);
}

NxUserTriggerReport* NxScenex::getUserTriggerReport() const
{
	return myptr->getUserTriggerReport();
}

void NxScenex::setUserContactReport(NxUserContactReport* callback)
{
	myptr->setUserContactReport(callback);
}

NxUserContactReport* NxScenex::getUserContactReport() const
{
	return myptr->getUserContactReport();
}

void NxScenex::setUserActorPairFiltering(NxUserActorPairFiltering* callback)
{
	myptr->setUserActorPairFiltering(callback);
}

NxUserActorPairFiltering* NxScenex::getUserActorPairFiltering() const
{
	return myptr->getUserActorPairFiltering();
}

bool NxScenex::raycastAnyBounds(const NxRay& worldRay, NxShapesType shapesType, NxU32 groups, NxReal maxDist, const NxGroupsMask* groupsMask) const
{
	return myptr->raycastAnyBounds(worldRay, shapesType, groups, maxDist, groupsMask);
}

bool NxScenex::raycastAnyShape(const NxRay& worldRay, NxShapesType shapesType, NxU32 groups, NxReal maxDist, const NxGroupsMask* groupsMask, NxShape** cache) const
{
	return myptr->raycastAnyShape(worldRay, shapesType, groups, maxDist, groupsMask, cache);
}

NxU32 NxScenex::raycastAllBounds(const NxRay& worldRay, NxUserRaycastReport& report, NxShapesType shapesType, NxU32 groups, NxReal maxDist, NxU32 hintFlags, const NxGroupsMask* groupsMask) const
{
	return myptr->raycastAllBounds(worldRay, report, shapesType, groups, maxDist, hintFlags, groupsMask);
}

NxU32 NxScenex::raycastAllShapes(const NxRay& worldRay, NxUserRaycastReport& report, NxShapesType shapesType, NxU32 groups, NxReal maxDist, NxU32 hintFlags, const NxGroupsMask* groupsMask) const
{
	return myptr->raycastAllShapes(worldRay, report, shapesType, groups, maxDist, hintFlags, groupsMask);
}

NxShape* NxScenex::raycastClosestBounds(const NxRay& worldRay, NxShapesType shapeType, NxRaycastHit& hit, NxU32 groups, NxReal maxDist, NxU32 hintFlags, const NxGroupsMask* groupsMask) const
{
	return myptr->raycastClosestBounds(worldRay, shapeType, hit, groups, maxDist, hintFlags, groupsMask);
}

NxShape* NxScenex::raycastClosestShape(const NxRay& worldRay, NxShapesType shapeType, NxRaycastHit& hit, NxU32 groups, NxReal maxDist, NxU32 hintFlags, const NxGroupsMask* groupsMask, NxShape** cache) const
{
	return myptr->raycastClosestShape(worldRay, shapeType, hit, groups, maxDist, hintFlags, groupsMask, cache);
}

NxU32 NxScenex::overlapSphereShapes(const NxSphere& worldSphere, NxShapesType shapeType, NxU32 nbShapes, NxShape** shapes, NxUserEntityReport<NxShape*>* callback, NxU32 activeGroups, const NxGroupsMask* groupsMask, bool accurateCollision)
{
	return myptr->overlapSphereShapes(worldSphere, shapeType, nbShapes, shapes, callback, activeGroups, groupsMask, accurateCollision);
}

NxU32 NxScenex::overlapAABBShapes(const NxBounds3& worldBounds, NxShapesType shapeType, NxU32 nbShapes, NxShape** shapes, NxUserEntityReport<NxShape*>* callback, NxU32 activeGroups, const NxGroupsMask* groupsMask, bool accurateCollision)
{
	return myptr->overlapAABBShapes(worldBounds, shapeType, nbShapes, shapes, callback, activeGroups, groupsMask, accurateCollision);
}

NxU32 NxScenex::overlapOBBShapes(const NxBox& worldBox, NxShapesType shapeType, NxU32 nbShapes, NxShape** shapes, NxUserEntityReport<NxShape*>* callback, NxU32 activeGroups, const NxGroupsMask* groupsMask, bool accurateCollision)
{
	return myptr->overlapOBBShapes(worldBox, shapeType, nbShapes, shapes, callback, activeGroups, groupsMask, accurateCollision);
}

NxU32 NxScenex::overlapCapsuleShapes(const NxCapsule& worldCapsule, NxShapesType shapeType, NxU32 nbShapes, NxShape** shapes, NxUserEntityReport<NxShape*>* callback, NxU32 activeGroups, const NxGroupsMask* groupsMask, bool accurateCollision)
{
	return myptr->overlapCapsuleShapes(worldCapsule, shapeType, nbShapes, shapes, callback, activeGroups, groupsMask, accurateCollision);
}

NxSweepCache* NxScenex::createSweepCache()
{
	return myptr->createSweepCache();
}

void NxScenex::releaseSweepCache(NxSweepCache* cache)
{
	myptr->releaseSweepCache(cache);
}

NxU32 NxScenex::linearOBBSweep(const NxBox& worldBox, const NxVec3& motion, NxU32 flags, void* userData, NxU32 nbShapes, NxSweepQueryHit* shapes, NxUserEntityReport<NxSweepQueryHit>* callback, NxU32 activeGroups, const NxGroupsMask* groupsMask)
{
	return myptr->linearOBBSweep(worldBox, motion, flags, userData, nbShapes, shapes, callback, activeGroups, groupsMask);
}

NxU32 NxScenex::linearCapsuleSweep(const NxCapsule& worldCapsule, const NxVec3& motion, NxU32 flags, void* userData, NxU32 nbShapes, NxSweepQueryHit* shapes, NxUserEntityReport<NxSweepQueryHit>* callback, NxU32 activeGroups, const NxGroupsMask* groupsMask)
{
	return myptr->linearCapsuleSweep(worldCapsule, motion, flags, userData, nbShapes, shapes, callback, activeGroups, groupsMask);
}

NxU32 NxScenex::cullShapes(NxU32 nbPlanes, const NxPlane* worldPlanes, NxShapesType shapeType, NxU32 nbShapes, NxShape** shapes, NxUserEntityReport<NxShape*>* callback, NxU32 activeGroups, const NxGroupsMask* groupsMask)
{
	return myptr->cullShapes(nbPlanes, worldPlanes, shapeType, nbShapes, shapes, callback, activeGroups, groupsMask);
}

bool NxScenex::checkOverlapSphere(const NxSphere& worldSphere, NxShapesType shapeType, NxU32 activeGroups, const NxGroupsMask* groupsMask)
{
	return myptr->checkOverlapSphere(worldSphere, shapeType, activeGroups, groupsMask);
}

bool NxScenex::checkOverlapAABB(const NxBounds3& worldBounds, NxShapesType shapeType, NxU32 activeGroups, const NxGroupsMask* groupsMask)
{
	return myptr->checkOverlapAABB(worldBounds, shapeType, activeGroups, groupsMask);
}

bool NxScenex::checkOverlapOBB(const NxBox& worldBox, NxShapesType shapeType, NxU32 activeGroups, const NxGroupsMask* groupsMask)
{
	return myptr->checkOverlapOBB(worldBox, shapeType, activeGroups, groupsMask);
}

bool NxScenex::checkOverlapCapsule(const NxCapsule& worldCapsule, NxShapesType shapeType, NxU32 activeGroups, const NxGroupsMask* groupsMask)
{
	return myptr->checkOverlapCapsule(worldCapsule, shapeType, activeGroups, groupsMask);
}

NxFluid* NxScenex::createFluid(const NxFluidDescBase& fluidDesc)
{
	return myptr->createFluid(fluidDesc);
}

void NxScenex::releaseFluid(NxFluid& fluid)
{
	myptr->releaseFluid(fluid);
}

NxU32 NxScenex::getNbFluids() const
{
	return myptr->getNbFluids();
}

NxFluid** NxScenex::getFluids()
{
	return myptr->getFluids();
}

bool NxScenex::cookFluidMeshHotspot(const NxBounds3& bounds, NxU32 packetSizeMultiplier, NxReal restParticlesPerMeter, NxReal kernelRadiusMultiplier, NxReal motionLimitMultiplier, NxReal collisionDistanceMultiplier, NxCompartment* compartment, bool forceStrictCookingFormat)
{
	return myptr->cookFluidMeshHotspot(bounds, packetSizeMultiplier, restParticlesPerMeter, kernelRadiusMultiplier, motionLimitMultiplier, collisionDistanceMultiplier, compartment, forceStrictCookingFormat);
}

NxCloth* NxScenex::createCloth(const NxClothDesc& clothDesc)
{
	return myptr->createCloth(clothDesc);
}

void NxScenex::releaseCloth(NxCloth& cloth)
{
	myptr->releaseCloth(cloth);
}

NxU32 NxScenex::getNbCloths() const
{
	return myptr->getNbCloths();
}

NxCloth** NxScenex::getCloths()
{
	return myptr->getCloths();
}

NxSoftBody* NxScenex::createSoftBody(const NxSoftBodyDesc& softBodyDesc)
{
	return myptr->createSoftBody(softBodyDesc);
}

void NxScenex::releaseSoftBody(NxSoftBody& softBody)
{
	myptr->releaseSoftBody(softBody);
}

NxU32 NxScenex::getNbSoftBodies() const
{
	return myptr->getNbSoftBodies();
}

NxSoftBody** NxScenex::getSoftBodies()
{
	return myptr->getSoftBodies();
}

bool NxScenex::isWritable()
{
	return myptr->isWritable();
}

void NxScenex::simulate(NxReal elapsedTime)
{
	docheats(myptr);

	myptr->simulate(elapsedTime);
}

bool NxScenex::checkResults(NxSimulationStatus status, bool block)
{
	return myptr->checkResults(status, block);
}

bool NxScenex::fetchResults(NxSimulationStatus status, bool block, NxU32 *errorState)
{
	return myptr->fetchResults(status, block, errorState);
}

void NxScenex::flushCaches()
{
	myptr->flushCaches();
}

const NxProfileData* NxScenex::readProfileData(bool clearData)
{
	return myptr->readProfileData(clearData);
}

NxThreadPollResult NxScenex::pollForWork(NxThreadWait waitType)
{
	return myptr->pollForWork(waitType);
}

void NxScenex::resetPollForWork()
{
	myptr->resetPollForWork();
}

NxThreadPollResult NxScenex::pollForBackgroundWork(NxThreadWait waitType)
{
	return myptr->pollForBackgroundWork(waitType);
}

void NxScenex::shutdownWorkerThreads()
{
	myptr->shutdownWorkerThreads();
}

void NxScenex::lockQueries()
{
	myptr->lockQueries();
}

void NxScenex::unlockQueries()
{
	myptr->unlockQueries();
}

NxSceneQuery* NxScenex::createSceneQuery(const NxSceneQueryDesc& desc)
{
	return myptr->createSceneQuery(desc);
}

bool NxScenex::releaseSceneQuery(NxSceneQuery& query)
{
	return myptr->releaseSceneQuery(query);
}

void NxScenex::setDynamicTreeRebuildRateHint(NxU32 dynamicTreeRebuildRateHint)
{
	myptr->setDynamicTreeRebuildRateHint(dynamicTreeRebuildRateHint);
}

NxU32 NxScenex::getDynamicTreeRebuildRateHint() const
{
	return myptr->getDynamicTreeRebuildRateHint();
}

void NxScenex::setSolverBatchSize(NxU32 solverBatchSize)
{
	myptr->setSolverBatchSize(solverBatchSize);
}

NxU32 NxScenex::getSolverBatchSize() const
{
	return myptr->getSolverBatchSize();
}
