#pragma once

struct Animation;
class AssetDatabase;

class AnimationFactory
{
public:
	static Animation* LoadAnimation(const std::string& aFilePath, AssetDatabase* aAssetDatabase);
	static Animation* LoadAnimationFromFBX(const std::string& aFilePath);
};

