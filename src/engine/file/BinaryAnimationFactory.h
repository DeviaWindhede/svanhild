#pragma once

class AssetDatabase;
//struct Animation;

/*	READ/WRITE ORDER
	
	frames
		size (size_t)
		boneTransforms (size_t, [ int, Transform(XMMatrix) ])
	name (string)
	length (unsigned int)
	frameRate (float)
	duration (float)
*/

class BinaryAnimationFactory
{
//public:
//	static Animation* LoadAnimationFromFile(const std::string& aFilePath, const std::string& aFolderName, AssetDatabase* aAssetDatabase);
//	static void WriteAnimationToFile(Animation* aAnimation, const std::string& aFolderName);
//private:
//	static std::vector<Frame> ReadFramesFromFile(FILE* aFile, size_t aFrameCount);
//	static Animation* GetAnimationFromFile(const std::string& aFilePath);
};