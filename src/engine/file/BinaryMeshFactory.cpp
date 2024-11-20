#include "pch.h"
#include "BinaryMeshFactory.h"
#include <filesystem>
//
//#include <engine/Paths.h>
//#include <engine/graphics/model/Mesh.h>
//#include <engine/graphics/model/SkeletalMesh.h>
//#include <engine/graphics/Vertex.h>
//#include "engine/graphics/animation\Skeleton.h"
//#include "engine/utility/Error.h"
//
//#include "../AssetDatabase.h"
//#include "../ShaderDatabase.h"
//#include "BinaryFileUtility.h"
//#include <ModelFactory.h>
//
//
//void BinaryMeshFactory::WriteMeshToFile(MeshDataPackage& aData, std::string aFolderPath)
//{
//	std::string path = aFolderPath + BinaryFileUtility::GetModelFileName(aData.name);
//	std::ofstream file(path, std::ios::binary | std::ios::out);
//
//	if (!file.is_open()) {
//		PrintE("Mesh \"" + aData.name + "\" doesn't have a binary model file. Generating file...");
//
//		if (!std::filesystem::exists(aFolderPath)) {
//			std::filesystem::create_directory(aFolderPath);
//		}
//
//		file.open(path, std::ios::binary | std::ios::out | std::ios::trunc);
//		if (!file.is_open()) {
//			PrintE("Error creating file: " + path);
//			return;
//		}
//	}
//
//	// has skeleton
//	bool hasSkeleton = aData.skeleton != nullptr;
//	{
//		file.write(reinterpret_cast<const char*>(&hasSkeleton), sizeof(bool));
//	}
//
//	// name
//	{
//		BinaryFileUtility::WriteStringToFile(file, aData.name);
//	}
//
//	// skeleton
//	if (hasSkeleton)
//	{
//		// skeleton name
//		{
//			BinaryFileUtility::WriteStringToFile(file, aData.skeleton->name);
//		}
//
//		// skeleton boneNameToIndexMap
//		{
//			size_t boneMapSize = aData.skeleton->boneNameToIndexMap.size();
//			file.write(reinterpret_cast<const char*>(&boneMapSize), sizeof(size_t));
//
//			for (auto& pair : aData.skeleton->boneNameToIndexMap)
//			{
//				BinaryFileUtility::WriteStringToFile(file, pair.first);
//
//				file.write(reinterpret_cast<const char*>(&pair.second), sizeof(int));
//			}
//		}
//
//		// skeleton bones
//		{
//			size_t size = aData.skeleton->bones.size();
//			file.write(reinterpret_cast<const char*>(&size), sizeof(size_t));
//
//			for (size_t j = 0; j < aData.skeleton->bones.size(); j++)
//			{
//				BinaryFileUtility::WriteBoneToFile(file, aData.skeleton->bones[j]);
//			}
//		}
//	}
//
//	size_t dataSize = aData.meshData.size();
//	file.write(reinterpret_cast<const char*>(&dataSize), sizeof(size_t));
//
//	for (size_t i = 0; i < dataSize; i++)
//	{
//		bool isSkeletal = aData.skeleton != nullptr; // TODO: THIS IS INCORRECT D: FIX THIS
//		{
//			file.write(reinterpret_cast<const char*>(&isSkeletal), sizeof(bool));
//		}
//
//		// material index
//		{
//			file.write(reinterpret_cast<const char*>(&aData.meshData[i].materialIndex), sizeof(aData.meshData[i].materialIndex));
//		}
//
//		// material name
//		{
//			BinaryFileUtility::WriteStringToFile(file, aData.meshData[i].materialName);
//		}
//
//		// name
//		{
//			BinaryFileUtility::WriteStringToFile(file, aData.meshData[i].name);
//		}
//
//		// vertices
//		{
//			if (isSkeletal)
//			{
//				size_t size = aData.meshData[i].animatedVertices.size();
//				file.write(reinterpret_cast<const char*>(&size), sizeof(size_t));
//				file.write(reinterpret_cast<const char*>(aData.meshData[i].animatedVertices.data()), sizeof(AnimatedVertex) * size);
//			}
//			else
//			{
//				size_t size = aData.meshData[i].vertices.size();
//				file.write(reinterpret_cast<const char*>(&size), sizeof(size_t));
//				file.write(reinterpret_cast<const char*>(aData.meshData[i].vertices.data()), sizeof(Vertex) * size);
//			}
//		}
//
//		// indices
//		{
//			size_t size = aData.meshData[i].indices.size();
//			file.write(reinterpret_cast<const char*>(&size), sizeof(size_t));
//			file.write(reinterpret_cast<const char*>(aData.meshData[i].indices.data()), sizeof(unsigned int) * size);
//		}
//	}
//
//	file.close();
//}
//
//MeshDataPackage BinaryMeshFactory::GetMeshDataFromFile(const std::string& aFilePath)
//{
//	if (!std::filesystem::exists(aFilePath))
//	{
//		PrintW("No bestie file exists!! " + aFilePath);
//		return {};
//	}
//
//	std::ifstream file(aFilePath, std::ios::binary | std::ios::in);
//
//	if (!file.is_open()) {
//		PrintE("Error opening file for reading: " + aFilePath);
//		return {};
//	}
//
//	MeshDataPackage data;
//	data.skeleton = nullptr;
//
//	bool hasSkeleton = false;
//
//	// has skeleton
//	{
//		file.read(reinterpret_cast<char*>(&hasSkeleton), sizeof(bool));
//	}
//
//	// name
//	{
//		BinaryFileUtility::ReadStringFromFile(file, data.name);
//	}
//
//
//	// skeleton
//	if (hasSkeleton)
//	{
//		data.skeleton = new Skeleton();
//
//		// skeleton name
//		{
//			BinaryFileUtility::ReadStringFromFile(file, data.skeleton->name);
//		}
//
//		// skeleton boneNameToIndexMap
//		{
//			size_t size = 0;
//			file.read(reinterpret_cast<char*>(&size), sizeof(size_t));
//
//			for (size_t j = 0; j < size; j++)
//			{
//				std::string key;
//				BinaryFileUtility::ReadStringFromFile(file, key);
//				int value = 0;
//				file.read(reinterpret_cast<char*>(&value), sizeof(int));
//				data.skeleton->boneNameToIndexMap.insert({ key, value });
//			}
//		}
//
//		// skeleton bones
//		{
//			size_t size = 0;
//			file.read(reinterpret_cast<char*>(&size), sizeof(size_t));
//			data.skeleton->bones.resize(size);
//
//			for (size_t j = 0; j < size; j++)
//			{
//				BinaryFileUtility::ReadBoneFromFile(file, data.skeleton, data.skeleton->bones[j]);
//			}
//		}
//	}
//
//	size_t dataSize = 0;
//	file.read(reinterpret_cast<char*>(&dataSize), sizeof(size_t));
//	data.meshData.resize(dataSize);
//
//	for (size_t i = 0; i < dataSize; i++)
//	{
//		bool isSkeletal = false;
//
//		// is skeletal
//		{
//			file.read(reinterpret_cast<char*>(&isSkeletal), sizeof(bool));
//		}
//
//		// material index
//		{
//			file.read(reinterpret_cast<char*>(&data.meshData[i].materialIndex), sizeof(size_t));
//		}
//
//		// material name
//		{
//			BinaryFileUtility::ReadStringFromFile(file, data.meshData[i].materialName);
//		}
//
//		// name
//		{
//			BinaryFileUtility::ReadStringFromFile(file, data.meshData[i].name);
//		}
//
//		// vertices
//		{
//			size_t size = 0;
//			file.read(reinterpret_cast<char*>(&size), sizeof(size_t));
//
//			if (isSkeletal)
//			{
//				data.meshData[i].animatedVertices.resize(size);
//				file.read(reinterpret_cast<char*>(data.meshData[i].animatedVertices.data()), sizeof(AnimatedVertex) * size);
//			}
//			else
//			{
//				data.meshData[i].vertices.resize(size);
//				file.read(reinterpret_cast<char*>(data.meshData[i].vertices.data()), sizeof(Vertex) * size);
//			}
//		}
//
//		// indices
//		{
//			size_t size = 0;
//			file.read(reinterpret_cast<char*>(&size), sizeof(size_t));
//			data.meshData[i].indices.resize(size);
//			file.read(reinterpret_cast<char*>(data.meshData[i].indices.data()), sizeof(unsigned int) * size);
//		}
//	}
//
//	file.close();
//
//	return data;
//}
//
//SharedMeshPackage BinaryMeshFactory::LoadMeshFromFile(const std::string& aFilePath, const std::string& aFolderPath, AssetDatabase* aAssetDatabase)
//{
//	std::string path = aFolderPath + BinaryFileUtility::GetModelFileName(aAssetDatabase->GetNameFromPath(aFilePath));
//
//	MeshDataPackage meshDataContainer = GetMeshDataFromFile(path);
//
//	SharedMeshPackage package = ModelFactory::GetSharedMeshFromPackage(meshDataContainer);
//
//	package.name = meshDataContainer.name;
//
//	return package;
//}
