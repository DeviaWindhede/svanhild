#pragma once
#include <string>
#include "FactoryStructs.h"

class AssetDatabase;
class SharedMesh;
class Mesh;
class SkeletalMesh;
class AnimationPlayer;

namespace fbxsdk
{
	class FbxNode;
	class FbxScene;
	class FbxAMatrix;
	class FbxVector4;
	template <class Type>
	class FbxLayerElementTemplate;
};

class ModelFactory
{
public:
	static SharedMeshPackage LoadMesh(const std::string& aFilePath, AssetDatabase* aAssetDatabase);
	static SharedMeshPackage GetSharedMeshFromPackage(MeshDataPackage& aMeshDataPackage, bool aSetMaterial = true);
	
	static MeshDataPackage LoadMeshFromFBX(const std::string& aFilePath);
private:
	static bool GetSkeleton(
		fbxsdk::FbxNode* aRootNode,
		int someParentIndex,
		Skeleton& outSkeleton
	);


	static DirectX::XMMATRIX FBXMatrixToDXMatrix(fbxsdk::FbxAMatrix& aMatrix);

	static void LoadMeshNodes(
		fbxsdk::FbxNode* aRootNode,
		std::vector<fbxsdk::FbxNode*>& outMeshNodes
	);

	static bool GetMeshDataFromScene(
		fbxsdk::FbxScene* aScene,
		fbxsdk::FbxNode* aMeshNode,
		Skeleton* aSkeleton,
		std::vector<MeshData>& outMeshData
	);

	static void SetDataFromFBXElement(
		fbxsdk::FbxLayerElementTemplate<fbxsdk::FbxVector4>* anElement,
		int aFbxContolPointIdx,
		int aPolygonIdx,
		Vector3f& outData
	);

	static void SetSkeletonName(
		Skeleton& aSkeleton/*,
		const std::string& aFileName*/
	);

	static void SetMaterial(const std::string& aMaterialName, SharedMesh* aSharedMesh);

};

