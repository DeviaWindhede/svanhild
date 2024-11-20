#pragma once

class AssetDatabase;
class SharedMesh;
struct Skeleton;

/*	READ/WRITE ORDER
	
	hasSkeleton
	name
	(skeleton)
		name (string)
		boneNameToIndexMap (size_t, [ string, size_t ] * size_t)
		bones (size_t, Bone * size_t)
	mesh count
		isSkeletal
		materialIndex
		materialName
		name
		vertices/animatedVertices (size_t, Vertex/AnimatedVertex * size_t)
		indices (size_t, unsigned int * size_t)
*/

class BinaryMeshFactory
{
//public:
//	static SharedMeshPackage LoadMeshFromFile(const std::string& aFilePath, const std::string& aFolderPath, AssetDatabase* aAssetDatabase);
//	static void WriteMeshToFile(MeshDataPackage& aData, std::string aFolderPath);
//
//private:
//	static MeshDataPackage GetMeshDataFromFile(const std::string& filePath);
//
//	static void SetMaterial(
//		std::string aMaterialName,
//		SharedMesh* aSharedMesh,
//		AssetDatabase* aAssetDatabase
//	);
};
