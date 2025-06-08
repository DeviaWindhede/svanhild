#pragma once
#include <string>
#include <vector>

#include "mesh/Vertex.h"

struct Skeleton;
class SharedMesh;

struct MeshData
{
	size_t materialIndex = 0;
	std::string materialName = "";
	std::string name = "";
	std::vector<Vertex> vertices = {};
	std::vector<AnimatedVertex> animatedVertices = {};
	std::vector<UINT16> indices = {};
	bool isAnimated = false;
};

// TODO: remove this?
struct MeshDataPackage
{
	std::string name = "";
	Skeleton* skeleton = nullptr;
	std::vector<MeshData> meshData = {};
};

struct SharedMeshPackage
{
	std::string name = "";
	Skeleton* skeleton = nullptr;
	std::vector<SharedMesh*> meshData = {};
};