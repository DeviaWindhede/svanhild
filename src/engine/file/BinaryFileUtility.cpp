#include "pch.h"
#include "BinaryFileUtility.h"
#include <filesystem>

//#include <engine/graphics/Vertex.h>
//#include <engine/graphics/animation/Skeleton.h>
//#include <engine/Paths.h>

void BinaryFileUtility::ReadStringFromFile(std::ifstream& aFile, std::string& outString)
{
	size_t size = 0;
	ReadSizeTFromFile(aFile, size);

	std::vector<char> buffer(size + 1);
	aFile.read(reinterpret_cast<char*>(buffer.data()), size);
	buffer[size] = '\0';

	outString = std::string(buffer.data());
}

void BinaryFileUtility::WriteStringToFile(std::ofstream& aFile, const std::string& aString)
{
	size_t size = aString.size();
	WriteSizeTToFile(aFile, size);
	aFile.write(reinterpret_cast<const char*>(aString.c_str()), sizeof(char) * size);
}

//void BinaryFileUtility::WriteBoneToFile(std::ofstream& aFile, Bone& aBone)
//{
//	WriteStringToFile(aFile, aBone.name);
//	WriteStringToFile(aFile, aBone.fullName);
//	aFile.write(reinterpret_cast<const char*>(&aBone.parentId), sizeof(int));
//	aFile.write(reinterpret_cast<const char*>(&aBone.inverseBindPose), sizeof(DirectX::XMMATRIX));
//
//	size_t size = aBone.childrenIds.size();
//	WriteSizeTToFile(aFile, size);
//	aFile.write(reinterpret_cast<const char*>(aBone.childrenIds.data()), sizeof(unsigned int) * size);
//}

void BinaryFileUtility::WriteSizeTToFile(std::ofstream& aFile, size_t aSize)
{
	aFile.write(reinterpret_cast<const char*>(&aSize), sizeof(size_t));
}

void BinaryFileUtility::WriteUintToFile(std::ofstream& aFile, unsigned int aValue)
{
	aFile.write(reinterpret_cast<const char*>(&aValue), sizeof(unsigned int));
}

//void BinaryFileUtility::ReadBoneFromFile(std::ifstream& file, Skeleton* aSkeleton, Bone& outBone)
//{
//	Bone& b = outBone;
//
//	ReadStringFromFile(file, b.name);
//	ReadStringFromFile(file, b.fullName);
//	int id = aSkeleton->boneNameToIndexMap.at(b.name);
//	b.id = id;
//	file.read(reinterpret_cast<char*>(&b.parentId), sizeof(int));
//	file.read(reinterpret_cast<char*>(&b.inverseBindPose), sizeof(DirectX::XMMATRIX));
//
//	size_t size = 0;
//	ReadSizeTFromFile(file, size);
//
//	b.childrenIds.resize(size);
//	file.read(reinterpret_cast<char*>(b.childrenIds.data()), sizeof(unsigned int) * size);
//}

void BinaryFileUtility::ReadSizeTFromFile(std::ifstream& aFile, size_t& outValue)
{
	aFile.read(reinterpret_cast<char*>(&outValue), sizeof(size_t));
}

void BinaryFileUtility::ReadUintFromFile(std::ifstream& aFile, unsigned int& outValue)
{
	aFile.read(reinterpret_cast<char*>(&outValue), sizeof(unsigned int));
}

std::string BinaryFileUtility::GetModelFileName(const std::string& aName)
{
	return aName + ".bestie";
}

std::string BinaryFileUtility::GetAnimationFileName(const std::string& aName)
{
	return aName + ".bff";
}
