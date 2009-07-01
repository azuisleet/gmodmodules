class NxPhysicsSDKx : public NxPhysicsSDK
{
public:
	NxPhysicsSDK *myptr;
	NxPhysicsSDKx(NxPhysicsSDK *ptr);

	virtual						~NxPhysicsSDKx() { myptr = NULL; };
	virtual void				release();
	virtual bool				setParameter(NxParameter paramEnum, NxReal paramValue);
	virtual NxReal				getParameter(NxParameter paramEnum) const;
	virtual NxScene*			createScene(const NxSceneDesc& sceneDesc);
	virtual void				releaseScene(NxScene& scene);
	virtual NxU32				getNbScenes() const;
	virtual NxScene*			getScene(NxU32 i);
	virtual NxTriangleMesh*		createTriangleMesh(const NxStream& stream);
	virtual void				releaseTriangleMesh(NxTriangleMesh& mesh);
	virtual NxU32				getNbTriangleMeshes() const;
	virtual NxHeightField*		createHeightField(const NxHeightFieldDesc& desc);
	virtual void				releaseHeightField(NxHeightField& heightField);
	virtual NxU32				getNbHeightFields() const;
	virtual NxCCDSkeleton*		createCCDSkeleton(const NxSimpleTriangleMesh& mesh);
	virtual NxCCDSkeleton*		createCCDSkeleton(const void* memoryBuffer, NxU32 bufferSize);
	virtual void				releaseCCDSkeleton(NxCCDSkeleton& skel);
	virtual NxU32				getNbCCDSkeletons() const;
	virtual NxConvexMesh*		createConvexMesh(const NxStream& mesh);
	virtual void				releaseConvexMesh(NxConvexMesh& mesh);
	virtual NxU32				getNbConvexMeshes() const;
	virtual NxClothMesh*		createClothMesh(NxStream& stream);
	virtual void				releaseClothMesh(NxClothMesh& cloth);
	virtual NxU32				getNbClothMeshes() const;
	virtual NxClothMesh**		getClothMeshes();
	virtual NxSoftBodyMesh*		createSoftBodyMesh(NxStream& stream);
	virtual void				releaseSoftBodyMesh(NxSoftBodyMesh& softBodyMesh);
	virtual NxU32				getNbSoftBodyMeshes() const;
	virtual NxSoftBodyMesh**	getSoftBodyMeshes();
	virtual NxU32				getInternalVersion(NxU32& apiRev, NxU32& descRev, NxU32& branchId) const;
	virtual NxInterface*		getInterface(NxInterfaceType type, int versionNumber);
	virtual NxHWVersion			getHWVersion() const;
	virtual NxU32				getNbPPUs() const;
	virtual NxFoundationSDK&	getFoundationSDK() const;
};