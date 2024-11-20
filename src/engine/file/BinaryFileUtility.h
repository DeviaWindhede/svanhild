#pragma once
#include <fstream>

struct Skeleton;
struct Bone;

class BinaryFileUtility
{
public:
	static std::string GetModelFileName(const std::string& aName);
	static std::string GetAnimationFileName(const std::string& aName);

	static void WriteStringToFile(std::ofstream& aFile, const std::string& aString);
	static void WriteBoneToFile(std::ofstream& aFile, Bone& aBone);
	static void WriteSizeTToFile(std::ofstream& aFile, size_t aSize);
	static void WriteUintToFile(std::ofstream& aFile, unsigned int aValue);

	template<typename T>
	static void WriteToFile(std::ofstream& aFile, const T& aValue);

	static void ReadStringFromFile(std::ifstream& aFile, std::string& outString);
	static void ReadBoneFromFile(std::ifstream& file, Skeleton* aSkeleton, Bone& outBone);
	static void ReadSizeTFromFile(std::ifstream& aFile, size_t& outValue);
	static void ReadUintFromFile(std::ifstream& aFile, unsigned int& outValue);
};

template<typename T>
inline void BinaryFileUtility::WriteToFile(std::ofstream& aFile, const T& aValue)
{
	aFile.write(reinterpret_cast<const char*>(&aValue), sizeof(T));
}
