#pragma once
#include <string>
#include <vector>
#include <DirectXMath.h>

struct Bone
{
	std::string name;
	std::string fullName;
	int id;
	int parentId;
	DirectX::XMMATRIX inverseBindPose;
	std::vector<unsigned int> childrenIds; // must be last in struct to support .buddy files

	Bone(
		const std::string& aName,
		const int& aId,
		const int& aParentId,
		const DirectX::XMMATRIX& aInverseBindPose,
		const std::vector<unsigned int>& aChildrenIds
	)
	{
		name = aName;
		id = aId;
		parentId = aParentId;
		inverseBindPose = aInverseBindPose;
		childrenIds = aChildrenIds;
	}

	Bone()
	{
		id = -1;
		parentId = -1;
		name = "";
		childrenIds = {};
		inverseBindPose = DirectX::XMMatrixIdentity();
	}
};
