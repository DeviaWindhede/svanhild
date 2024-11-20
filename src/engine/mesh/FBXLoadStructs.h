#pragma once
#include <DirectXMath.h>
#include <unordered_map>
#include <string>
#include <fbxsdk.h>
#include "Vertex.h"
#include <vector>

struct FBXBoneData
{
	unsigned int vertexIndex;
	unsigned int boneID;
	float weight;
};

struct FBXAnimation
{
	struct FBXFrame
	{
		// Stores Joint Name to Transform.
		std::unordered_map<std::string, DirectX::XMMATRIX> GlobalTransforms;
		// Stores Joint Name to Transform for Bone Space Transforms.
		std::unordered_map<std::string, DirectX::XMMATRIX> LocalTransforms;
		// A list of events that are triggered this frame.
		// Lets you use .find to see if it's here or not instead of
		// looping.
		std::unordered_map<std::string, bool> TriggeredEvents;

		std::unordered_map<std::string, DirectX::XMMATRIX> SocketTransforms;
	};

	// The animation frames.
	std::vector<FBXFrame> frames;

	// A list of events that exist in this animation.
	std::vector<std::string> EventNames;

	// How long this animation is in frames.
	unsigned int length;

	// The duration of this animation.
	double duration;

	// The FPS of this animation.
	float frameRate;

	std::string name;
};

struct FBXSkeleton
{
	std::string Name;

	struct FBXBone
	{
		DirectX::XMMATRIX BindPoseInverse;
		int ParentIdx = -1;
		std::string NamespaceName;
		std::string Name;
		std::vector<unsigned> Children;
	};

	struct FBXSocket
	{
		DirectX::XMMATRIX RestTransform;
		int ParentBoneIdx = -1;
		std::string Name;
		std::string NamespaceName;
	};

	std::vector<FBXBone> Bones;
	std::unordered_map<std::string, FBXSocket> Sockets;
	std::unordered_map<std::string, size_t> BoneNameToIndex;

	const FBXBone* GetRoot() const { if (!Bones.empty()) { return &Bones[0]; } return nullptr; }
};

struct FBXVertex
{
	float Position[4] = { 0,0,0,1 };
	float VertexColors[4][4]
	{
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0},
	};

	float UVs[4][2]
	{
		{0, 0},
		{0, 0},
		{0, 0},
		{0, 0}
	};

	float Normal[3] = { 0, 0, 0 };
	float Tangent[3] = { 0, 0, 0 };
	float BiNormal[3] = { 0, 0, 0 };

	unsigned int BoneIDs[4] = { 0, 0, 0, 0 };
	float BoneWeights[4] = { 0, 0, 0, 0 };

	bool operator==(const FBXVertex& other) const
	{
		return memcmp(this, &other, sizeof(FBXVertex)) == 0;
	}
};

struct VertexHash
{
	//size_t operator()(const FBXVertex& v) const;

	size_t operator()(const Vertex& v) const
	{
		size_t result = 0;
		hash_combine(result, v.position.x);
		hash_combine(result, v.position.y);
		hash_combine(result, v.position.z);
		hash_combine(result, v.color.x);
		hash_combine(result, v.color.y);
		hash_combine(result, v.color.z);
		hash_combine(result, v.uv.x);
		hash_combine(result, v.uv.y);
		hash_combine(result, v.normal.x);
		hash_combine(result, v.normal.y);
		hash_combine(result, v.normal.z);
		hash_combine(result, v.tangent.x);
		hash_combine(result, v.tangent.y);
		hash_combine(result, v.tangent.z);
		hash_combine(result, v.binormal.x);
		hash_combine(result, v.binormal.y);
		hash_combine(result, v.binormal.z);

		return result;
	}

	size_t operator()(const AnimatedVertex& v) const
	{
		size_t result = 0;
		hash_combine(result, v.position.x);
		hash_combine(result, v.position.y);
		hash_combine(result, v.position.z);
		hash_combine(result, v.color.x);
		hash_combine(result, v.color.y);
		hash_combine(result, v.color.z);
		hash_combine(result, v.uv.x);
		hash_combine(result, v.uv.y);
		hash_combine(result, v.normal.x);
		hash_combine(result, v.normal.y);
		hash_combine(result, v.normal.z);
		hash_combine(result, v.tangent.x);
		hash_combine(result, v.tangent.y);
		hash_combine(result, v.tangent.z);
		hash_combine(result, v.binormal.x);
		hash_combine(result, v.binormal.y);
		hash_combine(result, v.binormal.z);

		hash_combine(result, v.bones.x);
		hash_combine(result, v.bones.y);
		hash_combine(result, v.bones.z);
		hash_combine(result, v.bones.w);
		hash_combine(result, v.weights.x);
		hash_combine(result, v.weights.y);
		hash_combine(result, v.weights.z);
		hash_combine(result, v.weights.w);

		return result;
	}

	size_t operator()(const FBXVertex& v) const
	{
		size_t result = 0;
		hash_combine(result, v.Position[0]);
		hash_combine(result, v.Position[1]);
		hash_combine(result, v.Position[2]);
		hash_combine(result, v.VertexColors[0][0]);
		hash_combine(result, v.VertexColors[0][1]);
		hash_combine(result, v.VertexColors[0][2]);
		hash_combine(result, v.VertexColors[0][3]);
		hash_combine(result, v.VertexColors[1][0]);
		hash_combine(result, v.VertexColors[1][1]);
		hash_combine(result, v.VertexColors[1][2]);
		hash_combine(result, v.VertexColors[1][3]);
		hash_combine(result, v.VertexColors[2][0]);
		hash_combine(result, v.VertexColors[2][1]);
		hash_combine(result, v.VertexColors[2][2]);
		hash_combine(result, v.VertexColors[2][3]);
		hash_combine(result, v.VertexColors[3][0]);
		hash_combine(result, v.VertexColors[3][1]);
		hash_combine(result, v.VertexColors[3][2]);
		hash_combine(result, v.VertexColors[3][3]);
		hash_combine(result, v.UVs[0][0]);
		hash_combine(result, v.UVs[0][1]);
		hash_combine(result, v.UVs[1][0]);
		hash_combine(result, v.UVs[1][1]);
		hash_combine(result, v.UVs[2][0]);
		hash_combine(result, v.UVs[2][1]);
		hash_combine(result, v.UVs[3][0]);
		hash_combine(result, v.UVs[3][1]);
		hash_combine(result, v.Normal[0]);
		hash_combine(result, v.Normal[1]);
		hash_combine(result, v.Normal[2]);
		hash_combine(result, v.Tangent[0]);
		hash_combine(result, v.Tangent[1]);
		hash_combine(result, v.Tangent[2]);
		hash_combine(result, v.BiNormal[0]);
		hash_combine(result, v.BiNormal[1]);
		hash_combine(result, v.BiNormal[2]);
		// This MAY break things and requires testing.
		hash_combine(result, v.BoneIDs[0]);
		hash_combine(result, v.BoneIDs[1]);
		hash_combine(result, v.BoneIDs[2]);
		hash_combine(result, v.BoneIDs[3]);
		hash_combine(result, v.BoneWeights[0]);
		hash_combine(result, v.BoneWeights[1]);
		hash_combine(result, v.BoneWeights[2]);
		hash_combine(result, v.BoneWeights[3]);

		return result;
	}
private:

	template<typename T>
	inline void hash_combine(size_t& s, const T& v) const
	{
		std::hash<T> h;
		s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
	}
};

struct FBXMesh
{
	struct Element
	{
		std::vector<FBXVertex> Vertices;
		std::vector<unsigned int> Indices;

		unsigned int MaterialIndex;
		std::string MeshName;
	};

	struct LODGroup
	{
		struct LODLevel
		{
			unsigned int Level;
			float Distance;
			std::vector<Element> Elements;
		};

		std::vector<LODLevel> Levels;
	};

	FBXSkeleton Skeleton;

	std::vector<Element> Elements;
	//std::vector<Material> Materials;
	std::vector<LODGroup> LODGroups;

	std::string Name;
	std::vector<std::string> MaterialName;

	__forceinline bool IsValid() const
	{
		return (!Elements.empty() || !LODGroups.empty());
	}
};