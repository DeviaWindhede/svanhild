#include "pch.h"
#include "AnimationFactory.h"
#include <fbxsdk.h>

//#include <fbxsdk.h>

//#include "AssetDatabase.h"
//#include "BinaryReader/BinaryAnimationFactory.h"
//
//Animation* AnimationFactory::LoadAnimation(const std::string& aFilePath, AssetDatabase* aAssetDatabase)
//{
//	Animation* anim = BinaryAnimationFactory::LoadAnimationFromFile(aFilePath, RELATIVE_CUSTOM_ANIMATION_DATA_PATH, aAssetDatabase);
//
//#ifndef _RELEASE
//	if (anim == nullptr)
//	{
//		anim = LoadAnimationFromFBX(aFilePath);
//		BinaryAnimationFactory::WriteAnimationToFile(anim, RELATIVE_CUSTOM_ANIMATION_DATA_PATH);
//		BinaryAnimationFactory::WriteAnimationToFile(anim, RELATIVE_CUSTOM_ANIMATION_RELEASE_DATA_PATH);
//	}
//#endif
//
//	return anim;
//}
//
//Animation* AnimationFactory::LoadAnimationFromFBX(const std::string& aFilePath)
//{
//	FbxManager* lSdkManager = FbxManager::Create();
//
//	FbxIOSettings* ios = FbxIOSettings::Create(lSdkManager, IOSROOT);
//	lSdkManager->SetIOSettings(ios);
//
//	FbxImporter* fbxImporter = FbxImporter::Create(lSdkManager, "");
//
//	bool lImportStatus = fbxImporter->Initialize(aFilePath.c_str(), -1, lSdkManager->GetIOSettings());
//	if (!lImportStatus)
//	{
//		return nullptr;
//	}
//
//	FbxScene* fbxScene = FbxScene::Create(fbxImporter, "Scene");
//	if (!fbxScene)
//	{
//		return nullptr;
//	}
//	if (fbxImporter->Import(fbxScene))
//	{
//		FbxAxisSystem fbxAxisSystem;
//		fbxAxisSystem = FbxAxisSystem::eDirectX;
//		fbxAxisSystem.DeepConvertScene(fbxScene);
//		Skeleton skeleton;
//
//		SharedFactoryFunctions::GetSkeleton(fbxScene->GetRootNode(), -1, skeleton);
//
//		const FbxNode* skeletonRoot = fbxScene->FindNodeByName(FbxString(skeleton.GetRoot()->fullName.c_str()));
//		const FbxVector4 fbxTranslation = skeletonRoot->GetGeometricTranslation(::FbxNode::eSourcePivot);
//		const FbxVector4 fbxRotation = skeletonRoot->GetGeometricRotation(::FbxNode::eSourcePivot);
//		const FbxVector4 fbxScale = skeletonRoot->GetGeometricScaling(::FbxNode::eSourcePivot);
//		const FbxAMatrix rootTransform = FbxAMatrix(fbxTranslation, fbxRotation, fbxScale);
//
//		const FbxAnimStack* animStack = fbxScene->GetCurrentAnimationStack();
//
//		FbxString fbxAnimStackName = animStack->GetName();
//		std::string fbxAnimationName = fbxAnimStackName.Buffer();
//
//		const FbxTakeInfo* fbxTakeInfo = fbxScene->GetTakeInfo(fbxAnimStackName);
//
//		const FbxTime::EMode fbxGlobalTimeMode = fbxScene->GetGlobalSettings().GetTimeMode();
//		const double fbxFramesPerSecond = FbxTime::GetFrameRate(fbxGlobalTimeMode);
//		const FbxTime fbxAnimStart = fbxTakeInfo->mLocalTimeSpan.GetStart();
//		const FbxTime fbxAnimEnd = fbxTakeInfo->mLocalTimeSpan.GetStop();
//
//		// The FBX animation may not be frames 0...N and can start at arbitrary places.
//		const double fbxAnimTimeStart = fbxAnimStart.GetSecondDouble();
//		const double fbxAnimTimeEnd = fbxAnimEnd.GetSecondDouble();
//		const double fbxAnimTimeDuration = fbxAnimTimeEnd - fbxAnimTimeStart;
//
//		const FbxLongLong startFrame = fbxAnimStart.GetFrameCount(fbxGlobalTimeMode);
//		const FbxLongLong endFrame = fbxAnimEnd.GetFrameCount(fbxGlobalTimeMode);
//		const FbxLongLong animTime = endFrame - startFrame + 1;
//
//		FBXAnimation animation;
//		animation.name = aFilePath;
//		animation.length = static_cast<unsigned int>(animTime);
//		animation.frameRate = static_cast<float>(fbxFramesPerSecond);
//		animation.frames.resize(animTime);
//		animation.duration = static_cast<float>(fbxAnimTimeDuration);
//
//		size_t localFrameCounter = 0;
//
//		for (FbxLongLong t = startFrame; t <= endFrame; t++)
//		{
//			FbxTime currentTime;
//			currentTime.SetFrame(t, fbxGlobalTimeMode);
//
//			animation.frames[localFrameCounter].LocalTransforms.reserve(skeleton.bones.size());
//			animation.frames[localFrameCounter].GlobalTransforms.reserve(skeleton.bones.size());
//
//			for (Bone& Bone : skeleton.bones)
//			{
//				FbxAMatrix localFrameTransform;
//				FbxAMatrix globalFrameTransform;
//
//				if (FbxNode* jointNode = fbxScene->FindNodeByName(FbxString(Bone.fullName.c_str())))
//				{
//					localFrameTransform = jointNode->EvaluateLocalTransform(currentTime);
//					globalFrameTransform = jointNode->EvaluateGlobalTransform(currentTime);
//				}
//
//				float localTransformArray[16] =
//				{
//					(float)localFrameTransform.mData[0][0], (float)localFrameTransform.mData[0][1], (float)localFrameTransform.mData[0][2], (float)localFrameTransform.mData[0][3],
//					(float)localFrameTransform.mData[1][0], (float)localFrameTransform.mData[1][1], (float)localFrameTransform.mData[1][2], (float)localFrameTransform.mData[1][3],
//					(float)localFrameTransform.mData[2][0], (float)localFrameTransform.mData[2][1], (float)localFrameTransform.mData[2][2], (float)localFrameTransform.mData[2][3],
//					(float)localFrameTransform.mData[3][0], (float)localFrameTransform.mData[3][1], (float)localFrameTransform.mData[3][2], (float)localFrameTransform.mData[3][3]
//				};
//				float globalTransformArray[16] =
//				{
//					(float)globalFrameTransform.mData[0][0], (float)globalFrameTransform.mData[0][1], (float)globalFrameTransform.mData[0][2], (float)globalFrameTransform.mData[0][3],
//					(float)globalFrameTransform.mData[1][0], (float)globalFrameTransform.mData[1][1], (float)globalFrameTransform.mData[1][2], (float)globalFrameTransform.mData[1][3],
//					(float)globalFrameTransform.mData[2][0], (float)globalFrameTransform.mData[2][1], (float)globalFrameTransform.mData[2][2], (float)globalFrameTransform.mData[2][3],
//					(float)globalFrameTransform.mData[3][0], (float)globalFrameTransform.mData[3][1], (float)globalFrameTransform.mData[3][2], (float)globalFrameTransform.mData[3][3]
//				};
//
//				DirectX::XMMATRIX LocalTransform(localTransformArray);
//				DirectX::XMMATRIX GlobalTransform(globalTransformArray);
//
//				animation.frames[localFrameCounter].LocalTransforms.insert({ Bone.name, LocalTransform });
//				animation.frames[localFrameCounter].GlobalTransforms.insert({ Bone.name, GlobalTransform });
//			}
//
//			localFrameCounter++;
//		}
//
//		Animation* returnAnimation = new Animation();
//		returnAnimation->name = AssetDatabase::GetNameFromPath(animation.name);
//		returnAnimation->length = animation.length;
//		returnAnimation->frameRate = animation.frameRate;
//		returnAnimation->frames.resize(animation.frames.size());
//		returnAnimation->duration = static_cast<float>(animation.duration);
//
//		for (size_t f = 0; f < returnAnimation->frames.size(); f++)
//		{
//			returnAnimation->frames[f].count = animation.frames[f].LocalTransforms.size();
//
//			for (const auto& [boneName, boneTransform] : animation.frames[f].LocalTransforms)
//			{
//				DirectX::XMMATRIX localMatrix = boneTransform;
//
//				returnAnimation->frames[f].transform[skeleton.boneNameToIndexMap.at(boneName)] = localMatrix;
//			}
//		}
//
//		return returnAnimation;
//	}
//
//	return nullptr;
//}
