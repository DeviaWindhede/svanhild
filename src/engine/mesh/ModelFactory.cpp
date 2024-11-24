#include "pch.h"
#include "ModelFactory.h"

#include <fbxsdk.h>
#include "FBXLoadStructs.h"
#include "Bone.h"
#include "Skeleton.h"

//#include <engine/graphics/animation/AnimationPlayer.h>
//#include "AssetDatabase.h"
//#include "BinaryReader/BinaryMeshFactory.h"
//#include "BinaryReader/BinaryAnimationFactory.h"
//#include <ShaderDatabase.h>


DirectX::XMMATRIX ModelFactory::FBXMatrixToDXMatrix(fbxsdk::FbxAMatrix & aMatrix)
{
	float matrixArray[16]
	{
		(float)aMatrix.GetColumn(0)[0], (float)aMatrix.GetColumn(0)[1], (float)aMatrix.GetColumn(0)[2], (float)aMatrix.GetColumn(0)[3],
		(float)aMatrix.GetColumn(1)[0], (float)aMatrix.GetColumn(1)[1], (float)aMatrix.GetColumn(1)[2], (float)aMatrix.GetColumn(1)[3],
		(float)aMatrix.GetColumn(2)[0], (float)aMatrix.GetColumn(2)[1], (float)aMatrix.GetColumn(2)[2], (float)aMatrix.GetColumn(2)[3],
		(float)aMatrix.GetColumn(3)[0], (float)aMatrix.GetColumn(3)[1], (float)aMatrix.GetColumn(3)[2], (float)aMatrix.GetColumn(3)[3]
	};
	return DirectX::XMMATRIX(matrixArray);
}

void ModelFactory::LoadMeshNodes(fbxsdk::FbxNode* aRootNode, std::vector<fbxsdk::FbxNode*>& outMeshNodes)
{
	for (int i = 0; i < aRootNode->GetChildCount(); i++)
	{
		fbxsdk::FbxNode* childNode = aRootNode->GetChild(i);

		// node attribute states what type of node it is. If it's null it's not a valid node.
		const FbxNodeAttribute* childAttribute = childNode->GetNodeAttribute();
		if (!childAttribute)
		{
			LoadMeshNodes(childNode, outMeshNodes);
			continue;
		}

		if (childAttribute->GetAttributeType() == FbxNodeAttribute::eMesh)
		{
			outMeshNodes.push_back(childNode);
		}

		LoadMeshNodes(childNode, outMeshNodes);
	}
}

/*

SharedMeshPackage ModelFactory::LoadMesh(const std::string& aFilePath, AssetDatabase* aAssetDatabase)
{
	SharedMeshPackage package = BinaryMeshFactory::LoadMeshFromFile(aFilePath, RELATIVE_CUSTOM_MESH_DATA_PATH, aAssetDatabase);

#ifndef _RELEASE
	if (package.meshData.size() == 0)
	{
		auto meshPackage = LoadMeshFromFBX(aFilePath);

		assert(meshPackage.meshData.size() > 0 && "No mesh data found in FBX file");

		BinaryMeshFactory::WriteMeshToFile(meshPackage, RELATIVE_CUSTOM_MESH_DATA_PATH);
		package = BinaryMeshFactory::LoadMeshFromFile(aFilePath, RELATIVE_CUSTOM_MESH_DATA_PATH, aAssetDatabase);
	}
#endif

	return package;
}

void ModelFactory::SetMaterial(const std::string& aMaterialName, SharedMesh* aSharedMesh)
{
	aSharedMesh->SetMaterialName(aMaterialName);
	TextureCollection textureCollection;

	if (!AssetDatabase::DoesTextureExist(aMaterialName))
	{
		// TODO: Missing texture currently skips Forward rendering
		aSharedMesh->SetPixelShader(ShaderDatabase::GetPixelShader(PsType::MissingTextureDeferred));
		return;
	}
	textureCollection = AssetDatabase::GetTextures(AssetDatabase::GetTextureIndex(aMaterialName));

	aSharedMesh->SetTextures(textureCollection);
}


SharedMeshPackage ModelFactory::GetSharedMeshFromPackage(MeshDataPackage& aMeshDataPackage, bool aSetMaterial)
{
	SharedMeshPackage package;
	package.name = aMeshDataPackage.name;

	bool skeletonFound = aMeshDataPackage.skeleton != nullptr;
	for (size_t i = 0; i < aMeshDataPackage.meshData.size(); i++)
	{
		auto& data = aMeshDataPackage.meshData[i];

		SharedMesh* loadMesh = nullptr;
		if (skeletonFound)
		{
			loadMesh = new SkeletalMesh();
			auto skMesh = reinterpret_cast<SkeletalMesh*>(loadMesh);
			if (!skMesh->Initialize(
				data.animatedVertices.data(),
				data.animatedVertices.size(),
				data.indices.data(),
				data.indices.size()
			))
			{
				delete loadMesh;
				loadMesh = nullptr;
				return {};
			}

			skMesh->SetSkeleton(*aMeshDataPackage.skeleton);
			package.skeleton = aMeshDataPackage.skeleton;
			loadMesh->SetVertexShader(ShaderDatabase::GetVertexShader(VsType::SkinnedPBR));
		}
		else
		{
			loadMesh = new Mesh();
			if (!reinterpret_cast<Mesh*>(loadMesh)->Initialize(
				data.vertices.data(),
				data.vertices.size(),
				data.indices.data(),
				data.indices.size()
			))
			{
				delete loadMesh;
				loadMesh = nullptr;
				return {};
			}
			loadMesh->SetVertexShader(ShaderDatabase::GetVertexShader(VsType::DefaultPBRInstanced));
		}

		loadMesh->SetFileName(aMeshDataPackage.name);
		loadMesh->SetPixelShader(ShaderDatabase::GetPixelShader(PsType::GBuffer));

		if (aSetMaterial) { SetMaterial(data.materialName, loadMesh); }

		package.meshData.push_back(loadMesh);
	}

	return package;
}
*/


bool ModelFactory::GetMeshDataFromScene(
	fbxsdk::FbxScene* aScene,
	fbxsdk::FbxNode* aMeshNode,
	Skeleton* aSkeleton,
	std::vector<MeshData>& outMeshData
)
{
	const bool isSkeletalMesh = aSkeleton != nullptr;

	const unsigned int numMeshMaterials = aMeshNode->GetMaterialCount();
	outMeshData.reserve(numMeshMaterials);

	std::unordered_multimap<unsigned int, std::pair<size_t, float>> controlPointToBonesAndWeights;

	FbxMesh* fbxMesh = aMeshNode->GetMesh();

	if (isSkeletalMesh)
	{
		const FbxVector4 fbxTranslation = aMeshNode->GetGeometricTranslation(FbxNode::eSourcePivot);
		const FbxVector4 fbxRotation = aMeshNode->GetGeometricRotation(FbxNode::eSourcePivot);
		const FbxVector4 fbxScale = aMeshNode->GetGeometricScaling(FbxNode::eSourcePivot);
		const fbxsdk::FbxAMatrix rootTransform = FbxAMatrix(fbxTranslation, fbxRotation, fbxScale);

		// deformers are skins or blend shapes, currently we will only support skins
		for (int i = 0; i < fbxMesh->GetDeformerCount(); i++)
		{
			FbxSkin* fbxSkin = reinterpret_cast<FbxSkin*>(fbxMesh->GetDeformer(i, FbxDeformer::EDeformerType::eSkin));

			if (!fbxSkin)
			{
				//PrintW("Deformer is not a skin! We only support skins for now:( [ModelFactory.cpp]");
				continue;
			}

			for (int clusterIndex = 0; clusterIndex < fbxSkin->GetClusterCount(); clusterIndex++)
			{
				FbxCluster* fbxCluster = fbxSkin->GetCluster(clusterIndex);

				fbxsdk::FbxAMatrix meshBindTransform;
				fbxCluster->GetTransformMatrix(meshBindTransform);

				fbxsdk::FbxAMatrix localToWorldTransform;
				fbxCluster->GetTransformLinkMatrix(localToWorldTransform);

				fbxsdk::FbxAMatrix bindPoseInverseTransform = localToWorldTransform.Inverse() * meshBindTransform * rootTransform;
				// idk why but without transposing the inverse transform it's very VERY wrong
				bindPoseInverseTransform = bindPoseInverseTransform.Transpose();

				size_t jointIndex = 0;
				{
					std::string jointName = fbxCluster->GetLink()->GetName();
					if (size_t namePos = jointName.find_last_of(':'); namePos != std::string::npos)
					{
						jointName = jointName.substr(namePos + 1);
					}
					jointIndex = aSkeleton->boneNameToIndexMap[jointName];
				}

				aSkeleton->bones[jointIndex].inverseBindPose = FBXMatrixToDXMatrix(bindPoseInverseTransform);

				for (int j = 0; j < fbxCluster->GetControlPointIndicesCount(); j++)
				{
					const unsigned int controlPointIndex = fbxCluster->GetControlPointIndices()[j];
					const float weight = static_cast<float>(fbxCluster->GetControlPointWeights()[j]);
					controlPointToBonesAndWeights.insert({ controlPointIndex, { jointIndex, weight } });
				}
			}
		}
	}

	FbxAMatrix tempmatrix = aMeshNode->EvaluateGlobalTransform();
	FbxTransform tempxform = aMeshNode->GetTransform();

	// If we have materials we may need to cut this mesh up into multiple.
	const bool bHasMaterials = aMeshNode->GetMaterialCount() != 0;
	FbxSurfaceMaterial* currentSceneMaterial = nullptr;

	// We need to load per material index since each mesh can only have one material when rendering.
	// This means that if it has more than one they will be separated into two meshes.
	for (int meshMaterialIndex = 0; meshMaterialIndex < aMeshNode->GetMaterialCount() || meshMaterialIndex == 0; meshMaterialIndex++)
	{
		MeshData currentMeshData = {};
		currentMeshData.name = aMeshNode->GetName();

		if (bHasMaterials)
		{
			for (int sceneMaterialIndex = 0; sceneMaterialIndex < aScene->GetMaterialCount(); sceneMaterialIndex++)
			{
				FbxSurfaceMaterial* sceneMaterial = aScene->GetMaterial(sceneMaterialIndex);
				FbxSurfaceMaterial* meshNodeMaterial = aMeshNode->GetMaterial(meshMaterialIndex);
				if (sceneMaterial == meshNodeMaterial)
				{
					currentSceneMaterial = sceneMaterial;
					currentMeshData.materialIndex = sceneMaterialIndex;
					currentMeshData.materialName = meshNodeMaterial->GetName();
					break;
				}
			}
		}
		else
		{
			currentMeshData.materialIndex = 0;
		}

		FbxLayerElementMaterial* fbxElementMaterial = fbxMesh->GetElementMaterial();

		const int fbxMeshPolygonCount = fbxMesh->GetPolygonCount();
		std::unordered_map<size_t, unsigned int> duplicateVerticesMap;

		if (isSkeletalMesh)
		{
			currentMeshData.animatedVertices.reserve(static_cast<size_t>(fbxMeshPolygonCount) * 3ULL);
			duplicateVerticesMap.reserve(currentMeshData.animatedVertices.capacity());
		}
		else
		{
			currentMeshData.vertices.reserve(static_cast<size_t>(fbxMeshPolygonCount) * 3ULL);
			duplicateVerticesMap.reserve(currentMeshData.vertices.capacity());
		}
		currentMeshData.indices.reserve(currentMeshData.indices.capacity());
		currentMeshData.isAnimated = isSkeletalMesh;

		UINT16 indexCounter = 0;
		for (int p = 0; p < fbxMeshPolygonCount; p++)
		{
			if (bHasMaterials)
			{
				// This is the index of the materials in the mesh element array.
				// It doesn't always correspond to the scene material list since the first
				// material here might be material n in the scene.
				const int IndexAtP = fbxElementMaterial->GetIndexArray().GetAt(p);
				FbxSurfaceMaterial* polygonMaterial = aMeshNode->GetMaterial(IndexAtP);
				if (currentSceneMaterial != polygonMaterial)
				{
					continue;
				}
			}

			for (int vertexIndex = 0; vertexIndex < 3; vertexIndex++)
			{
				const int fbxControlPtIndex = fbxMesh->GetPolygonVertex(p, vertexIndex);
				const int polygonIndex = p * 3 + vertexIndex;

				Vertex vertex;
				AnimatedVertex animatedVertex;

				Vertex* v = isSkeletalMesh ? &animatedVertex : &vertex;

				// Position
				{
					const FbxVector4 fbxVxPos = fbxMesh->GetControlPointAt(fbxControlPtIndex);
					v->position.x = static_cast<float>(fbxVxPos[0]);
					v->position.y = static_cast<float>(fbxVxPos[1]);
					v->position.z = static_cast<float>(fbxVxPos[2]);
					//v->position.w = static_cast<float>(fbxVxPos[3]);
				}

				// UVs
				{
					FbxVector2 fbxVxUVs[4];
					const int fbxNumUVs = fbxMesh->GetElementUVCount();

					const int fbxTextureUVIndex = fbxMesh->GetTextureUVIndex(p, vertexIndex);
					for (int uv = 0; uv < fbxNumUVs && uv < 4; uv++)
					{
						FbxGeometryElementUV* fbxUvElement = fbxMesh->GetElementUV(uv);
						fbxVxUVs[uv] = fbxUvElement->GetDirectArray().GetAt(fbxTextureUVIndex);
					}

					v->uv.x = static_cast<float>(fbxVxUVs[0][0]);
					v->uv.y = 1 - static_cast<float>(fbxVxUVs[0][1]);
				}

				// Normals
				{
					SetDataFromFBXElement(fbxMesh->GetElementNormal(0), fbxControlPtIndex, polygonIndex, v->normal);
					SetDataFromFBXElement(fbxMesh->GetElementTangent(0), fbxControlPtIndex, polygonIndex, v->tangent);
					SetDataFromFBXElement(fbxMesh->GetElementBinormal(0), fbxControlPtIndex, polygonIndex, v->binormal);
				}

				// Vertex Colors
				{
					v->color.x = 1.0f;
					v->color.y = 1.0f;
					v->color.z = 1.0f;
					//v->color.w = 1.0f;

					{
						FbxColor fbxColors[4];
						const int fbxNumVxColorChannels = fbxMesh->GetElementVertexColorCount();
						for (int col = 0; col < fbxNumVxColorChannels && col < 4; col++)
						{
							FbxGeometryElementVertexColor* colElement = fbxMesh->GetElementVertexColor(col);

							int index = fbxControlPtIndex;
							FbxLayerElement::EReferenceMode referenceMode = fbxMesh->GetElementNormal(0)->GetReferenceMode();
							if (colElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
							{
								referenceMode = colElement->GetReferenceMode();
								index = polygonIndex;
							}

							if (referenceMode == FbxGeometryElement::eIndexToDirect)
							{
								index = colElement->GetIndexArray().GetAt(index);
							}

							fbxColors[col] = colElement->GetDirectArray().GetAt(index);
						}

						// TODO: multiple color channels?
						if (fbxNumVxColorChannels > 0)
						{
							v->color.x = static_cast<float>(fbxColors[0][0]);
							v->color.y = static_cast<float>(fbxColors[0][1]);
							v->color.z = static_cast<float>(fbxColors[0][2]);
							//v->color.w = static_cast<float>(fbxColors[0][3]);
						}
					}
				}

				// Bones
				if (isSkeletalMesh)
				{
					unsigned int boneIndices[] = { 0, 0, 0, 0 };
					float weights[] = { 0, 0, 0, 0 };

					if (aSkeleton->GetRoot())
					{
						// Since we're making a bit of a complex iteration we need to define the iterator.
						// It's a lot less to type that way.
						typedef std::unordered_multimap<unsigned int, std::pair<size_t, float>>::iterator MMIter;

						// Then we use equal range to get all the data stored for this specific control point.
						std::pair<MMIter, MMIter> values = controlPointToBonesAndWeights.equal_range(fbxControlPtIndex);

						// This idx is to loop on the 4 indices of ID and Weight.
						int idx = 0;
						for (MMIter it = values.first; it != values.second && idx < 4; ++it)
						{
							std::pair<size_t, float> BoneAndWeight = it->second;
							boneIndices[idx] = static_cast<unsigned>(BoneAndWeight.first);
							weights[idx] = BoneAndWeight.second;
							idx++;
						}
					}

					reinterpret_cast<AnimatedVertex*>(v)->bones.x = boneIndices[0];
					reinterpret_cast<AnimatedVertex*>(v)->bones.y = boneIndices[1];
					reinterpret_cast<AnimatedVertex*>(v)->bones.z = boneIndices[2];
					reinterpret_cast<AnimatedVertex*>(v)->bones.w = boneIndices[3];

					reinterpret_cast<AnimatedVertex*>(v)->weights.x = weights[0];
					reinterpret_cast<AnimatedVertex*>(v)->weights.y = weights[1];
					reinterpret_cast<AnimatedVertex*>(v)->weights.z = weights[2];
					reinterpret_cast<AnimatedVertex*>(v)->weights.w = weights[3];
				}

				{
					// control points may contain duplicate vertices
					VertexHash Hasher;
					size_t hash = isSkeletalMesh ? Hasher(*reinterpret_cast<AnimatedVertex*>(v)) : Hasher(*v);
					if (duplicateVerticesMap.find(hash) == duplicateVerticesMap.end())
					{
						duplicateVerticesMap[hash] = indexCounter;
						if (isSkeletalMesh)
						{
							currentMeshData.animatedVertices.push_back(*reinterpret_cast<AnimatedVertex*>(v));
						}
						else
						{
							currentMeshData.vertices.push_back(*v);
						}
						currentMeshData.indices.push_back(indexCounter++);
					}
					else
					{
						currentMeshData.indices.push_back(duplicateVerticesMap[hash]);
					}
				}
			}
		}

		if (currentMeshData.vertices.size() + currentMeshData.animatedVertices.size() > 0)
		{
			outMeshData.push_back(currentMeshData);
		}

		duplicateVerticesMap.clear();

	}

	controlPointToBonesAndWeights.clear();

	return true;
}

void ModelFactory::SetDataFromFBXElement(
	fbxsdk::FbxLayerElementTemplate<fbxsdk::FbxVector4>* aElement,
	int aFbxContolPointIdx,
	int aPolygonIdx,
	Vector3f& outData
)
{
	int index = aElement->GetMappingMode() == FbxGeometryElement::eByControlPoint ? aFbxContolPointIdx : aPolygonIdx;

	assert(aElement->GetReferenceMode() != FbxGeometryElement::EReferenceMode::eIndex && "Invalid reference mode");

	if (aElement->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
	{
		index = aElement->GetIndexArray().GetAt(index);
	}

	outData.x = static_cast<float>(aElement->GetDirectArray().GetAt(index).mData[0]);
	outData.y = static_cast<float>(aElement->GetDirectArray().GetAt(index).mData[1]);
	outData.z = static_cast<float>(aElement->GetDirectArray().GetAt(index).mData[2]);
}


void ModelFactory::SetSkeletonName(Skeleton& aSkeleton)
{
	const Bone* rootBone = aSkeleton.GetRoot();
	if (!rootBone) { return; }

	const std::string rootBoneName = rootBone->fullName;
	const size_t pos = rootBoneName.find_first_of(':');
	aSkeleton.name = rootBoneName.substr(0, pos);

	//std::string name = aFileName;
	//std::replace(name.begin(), name.end(), '/', '\\');
	//auto slashPos = aFileName.find_last_of('\\');
	//if (slashPos != std::string::npos)
	//{
	//	name = name.substr(slashPos + 1, name.length() - (slashPos - 1));
	//}

	//name = name.substr(0, name.length());

	//aSkeleton.name = name;
}

// TODO: LOD groups?
MeshDataPackage ModelFactory::LoadMeshFromFBX(const std::string& aFilePath)
{
	FbxManager* lSdkManager = FbxManager::Create();

	FbxIOSettings* ios = FbxIOSettings::Create(lSdkManager, "OC");
	lSdkManager->SetIOSettings(ios);

	FbxImporter* fbxImporter = FbxImporter::Create(lSdkManager, "");

	if (!fbxImporter->Initialize(aFilePath.c_str(), -1, lSdkManager->GetIOSettings())) { return {}; }

	FbxScene* scene = FbxScene::Create(fbxImporter, "Scene");
	if (!scene) { return {}; }

	std::string fileName = aFilePath.substr(0, aFilePath.find_last_of("."));

	// TODO: Combine mulitple meshes into one class?
	if (!fbxImporter->Import(scene)) { return {}; }

	scene->ConnectMaterials();
	FbxGeometryConverter* fbxConverter = new FbxGeometryConverter(lSdkManager);

	if (!fbxConverter->Triangulate(scene, true, true))
	{
		// triangulate using the legacy triangulator, surely it works this time :))
		//PrintW("Trianguation does not work!! Trying legacy triangulator... [ModelFactory.cpp]");
		if (!fbxConverter->Triangulate(scene, true, false))
		{
			throw;
		}
	}

	std::vector<FbxNode*> meshNodes;
	LoadMeshNodes(scene->GetRootNode(), meshNodes);

	// debug since this should not happen
#ifndef _RELEASE
	for (FbxNode* meshNode : meshNodes)
	{
		FbxMesh* fbxMesh = meshNode->GetMesh();

		const int numUvLayers = fbxMesh->GetUVLayerCount();

		assert(numUvLayers > 0 && "No UV layers present in FBX file");

		if (fbxMesh->GetElementBinormalCount() > 0 && fbxMesh->GetElementTangentCount() > 0) { continue; }

		bool result = fbxMesh->GenerateTangentsData(0, false, false);

		const FbxStatus& status = fbxImporter->GetStatus();
		assert(result && "No UV layers present in FBX file" && status.GetErrorString());
	}
#endif

	FbxAxisSystem fbxAxisSystem;
	fbxAxisSystem = FbxAxisSystem::eDirectX;
	fbxAxisSystem.DeepConvertScene(scene);

	Skeleton* skeleton = new Skeleton();

	MeshDataPackage package;
	//package.name = AssetDatabase::GetNameFromPath(fileName);
	package.skeleton = nullptr;

	bool skeletonFound = GetSkeleton(scene->GetRootNode(), -1, *skeleton);
	if (skeletonFound)
	{
		package.skeleton = skeleton;
		SetSkeletonName(*skeleton);
	}
	else
	{
		delete skeleton;
		skeleton = nullptr;
	}

	for (FbxNode* meshNode : meshNodes)
	{
		GetMeshDataFromScene(scene, meshNode, skeleton, package.meshData);
	}

	if (scene->GetMaterialCount() > 0)
	{
		for (int j = 0; j < static_cast<int>(package.meshData.size()); j++)
		{
			std::string name = package.meshData[j].materialName;
			if (name.rfind("m_", 0) == 0)
			{
				name.erase(name.begin(), name.begin() + 2);
			}

			package.meshData[j].materialName = name;
		}
	}

	scene->Destroy(false);
	fbxImporter->Destroy();

	return package;
}

bool ModelFactory::GetSkeleton(
	FbxNode* aRootNode,
	int someParentIndex,
	Skeleton& outSkeleton
)
{
	int index = someParentIndex;

	FbxNodeAttribute* nodeAttribute = aRootNode->GetNodeAttribute();
	if (nodeAttribute)
	{
		if (nodeAttribute->GetAttributeType() == FbxNodeAttribute::eSkeleton)
		{
			// We're a bone, process as normal
			//FBXSkeleton::FBXBone bone;
			Bone bone;
			bone.fullName = aRootNode->GetName();
			bone.parentId = someParentIndex;

			bone.name = bone.fullName;
			if (const size_t pos = bone.name.find_last_of(':'); pos != std::string::npos)
			{
				bone.name = bone.name.substr(pos + 1);
			}
			outSkeleton.bones.push_back(bone);

			index = static_cast<int>(outSkeleton.bones.size() - 1ULL);
			bone.id = index;

			if (outSkeleton.boneNameToIndexMap.find(bone.name) != outSkeleton.boneNameToIndexMap.end())
			{
				std::throw_with_nested(std::runtime_error("Found more than one bone with the same name [" + bone.fullName + "]!"));
			}

			outSkeleton.boneNameToIndexMap.insert({ bone.name, index });

			if (someParentIndex >= 0)
			{
				outSkeleton.bones[someParentIndex].childrenIds.push_back(index);
			}
		}
	}

	for (int i = 0; i < aRootNode->GetChildCount(); i++)
	{
		GetSkeleton(aRootNode->GetChild(i), index, outSkeleton);
	}

	return !outSkeleton.bones.empty();
}
