#pragma once
//#include "../../math/Transform.h"

constexpr size_t MAX_ANIMATION_BONES = 64;

typedef DirectX::XMMATRIX JointTransforms[MAX_ANIMATION_BONES];

struct Pose
{
	JointTransforms transform;
	size_t count = 0;
};
