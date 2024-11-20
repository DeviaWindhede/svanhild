#include "pch.h"
#include "BinaryAnimationFactory.h"
//#include <engine/graphics/model/Mesh.h>
//#include <engine/graphics/model/SkeletalMesh.h>
//#include <vector>
//
//#include <filesystem>
//#include "engine/graphics/animation\Skeleton.h"
//#include "engine/utility/Error.h"
//#include "../AssetDatabase.h"
//
//#include <engine/Paths.h>
//#include "BinaryFileUtility.h"
//
//Animation* BinaryAnimationFactory::LoadAnimationFromFile(const std::string& aFilePath, const std::string& aFolderName, AssetDatabase* aAssetDatabase)
//{
//	std::string path = aFolderName + BinaryFileUtility::GetAnimationFileName(aAssetDatabase->GetNameFromPath(aFilePath));
//
//	return GetAnimationFromFile(path);
//}
//
//void BinaryAnimationFactory::WriteAnimationToFile(Animation* aAnimation, const std::string& aFolderName)
//{
//	std::string path = aFolderName + BinaryFileUtility::GetAnimationFileName(aAnimation->name);
//	std::ofstream file(path, std::ios::binary | std::ios::out);
//
//	if (!file.is_open()) {
//		PrintE("Animation \"" + aAnimation->name + "\" doesn't have a binary animation file. Generating file...");
//
//		if (!std::filesystem::exists(aFolderName)) {
//			std::filesystem::create_directory(aFolderName);
//		}
//
//		file.open(path, std::ios::binary | std::ios::out | std::ios::trunc);
//		if (!file.is_open()) {
//			PrintE("Error creating file: " + path);
//			return;
//		}
//	}
//
//	// frames
//	{
//		size_t containerSize = aAnimation->frames.size();
//		file.write(reinterpret_cast<const char*>(&containerSize), sizeof(size_t));
//
//		for (size_t i = 0; i < containerSize; i++)
//		{
//			size_t size = MAX_ANIMATION_BONES;
//			file.write(reinterpret_cast<const char*>(&size), sizeof(size_t));
//
//			for (size_t j = 0; j < size; ++j)
//			{
//				// Write transform
//				DirectX::XMMATRIX m = aAnimation->frames[i].transform[j].GetMatrix();
//				file.write(reinterpret_cast<const char*>(&m), sizeof(DirectX::XMMATRIX));
//			}
//		}
//	}
//
//	// name
//	{
//		BinaryFileUtility::WriteStringToFile(file, aAnimation->name);
//	}
//
//	// length
//	{
//		file.write(reinterpret_cast<const char*>(&aAnimation->length), sizeof(unsigned int));
//	}
//
//	// frame rate
//	{
//		file.write(reinterpret_cast<const char*>(&aAnimation->frameRate), sizeof(float));
//	}
//
//	// duration
//	{
//		file.write(reinterpret_cast<const char*>(&aAnimation->duration), sizeof(float));
//	}
//}
//
//Animation* BinaryAnimationFactory::GetAnimationFromFile(const std::string& aFilePath)
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
//	Animation* anim = new Animation();
//
//	// frames
//	{
//		size_t containerSize = 0;
//		file.read(reinterpret_cast<char*>(&containerSize), sizeof(size_t));
//		anim->frames.resize(containerSize);
//
//		for (size_t i = 0; i < containerSize; i++)
//		{
//			// frame
//			Frame frame{};
//
//			size_t size = 0;
//			file.read(reinterpret_cast<char*>(&size), sizeof(size_t));
//
//			for (size_t j = 0; j < size; j++)
//			{
//				// Read transform
//				DirectX::XMMATRIX m{};
//				file.read(reinterpret_cast<char*>(&m), sizeof(DirectX::XMMATRIX));
//				
//				frame.transform[j] = Transform(m);
//			}
//			anim->frames[i] = frame;
//		}
//	}
//
//	// name
//	{
//		BinaryFileUtility::ReadStringFromFile(file, anim->name);
//	}
//
//	// length
//	{
//		file.read(reinterpret_cast<char*>(&anim->length), sizeof(unsigned int));
//	}
//
//	// frame rate
//	{
//		file.read(reinterpret_cast<char*>(&anim->frameRate), sizeof(float));
//	}
//
//	// duration
//	{
//		file.read(reinterpret_cast<char*>(&anim->duration), sizeof(float));
//	}
//
//	return anim;
//}
