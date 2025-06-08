#pragma once
#include "Bone.h"
#include "Pose.h"

#include <string>
#include <unordered_map>

struct Skeleton
{
public:
	std::string name;
	std::unordered_map<std::string, int> boneNameToIndexMap;  // can be used to lock a transform to a bone
	std::vector<Bone> bones;

	const Bone* GetRoot() const { if (bones.empty()) return nullptr; return &bones[0]; }

	void ConvertPoseToModelSpace(const Pose& in, Pose& out) const;
	void ApplyBindPoseInverse(const Pose& in, DirectX::XMMATRIX* out) const;
	DirectX::XMMATRIX GetBoneModelSpaceTransform(const Pose& in, const std::string& aBoneName) const;
	DirectX::XMMATRIX GetBoneModelSpaceTransform(const Pose& in, const int aBoneIndex) const;
private:
	void ConvertPoseToModelSpace(const Pose& in, Pose& out, unsigned aBoneIndex, const DirectX::XMMATRIX& aParentTransform)  const;
};


//#include <string>
//#include <vector>
//#include <unordered_map>

//struct Bone
//{
//	DirectX::XMMATRIX BindPoseInverse;
//	int Parent;
//	std::vector<unsigned int> Children;
//	std::string Name;
//
//	bool operator==(const Bone& aBone) const
//	{
//		const bool A = DirectX::XMVector4Equal(BindPoseInverse.r[0], aBone.BindPoseInverse.r[0]) &&
//			DirectX::XMVector4Equal(BindPoseInverse.r[1], aBone.BindPoseInverse.r[1]) &&
//			DirectX::XMVector4Equal(BindPoseInverse.r[2], aBone.BindPoseInverse.r[2]) &&
//			DirectX::XMVector4Equal(BindPoseInverse.r[3], aBone.BindPoseInverse.r[3]);
//		const bool B = Parent == aBone.Parent;
//		const bool C = Name == aBone.Name;
//		const bool D = Children == aBone.Children;
//
//		return A && B && C && D;
//	}
//
//	Bone()
//		: BindPoseInverse{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 }, Parent(-1)
//	{
//	}
//};
//
//struct Skeleton
//{
//	std::string Name;
//
//	std::vector<Bone> Bones;
//	std::unordered_map<std::string, size_t> BoneNameToIndex;  // can be used to lock a transform to a bone
//	std::vector<std::string> BoneName;
//
//	const Bone* GetRoot() const { if (Bones.empty()) return nullptr; return &Bones[0]; }
//
//	bool operator==(const Skeleton& aSkeleton) const
//	{
//		return Bones == aSkeleton.Bones;
//	}
//
//	void ConvertPoseToModelSpace(const Pose& in, Pose& out) const;
//	void ApplyBindPoseInverse(const Pose& in, DirectX::XMMATRIX* out) const;
//private:
//	void ConvertPoseToModelSpace(const Pose& in, Pose& out, unsigned aBoneIdx, const DirectX::XMMATRIX& aParentTransform)  const;
//
//};
