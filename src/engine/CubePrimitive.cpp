#include "pch.h"
#include "CubePrimitive.h"
#include "mesh/Vertex.h"
#include <vector>

void CubePrimitive::InitPrimitive()
{
	float size = 1.0f;
	std::vector<Vertex> vertices{
        // front
        { { -size, -size, size },    { 0, 0, 1 }, { 1, 0 } },
        { { size, -size, size },     { 0, 0, 1 }, { 0, 0 } },
        { { size, size, size },      { 0, 0, 1 }, { 0, 1 } },
        { { -size, size, size },     { 0, 0, 1 }, { 1, 1 } },
        // back
        { { -size, -size, -size },  { 0, 0, 0.5f }, { 1, 0 } },
        { { size, -size, -size },   { 0, 0, 0.5f }, { 0, 0 } },
        { { size, size, -size },    { 0, 0, 0.5f }, { 0, 1 } },
        { { -size, size, -size },   { 0, 0, 0.5f }, { 1, 1 } },

        // left
        { { -size, -size, -size },  { 0.5f, 0, 0 }, { 0, 1 } }, // Bottom-left
        { { -size, -size, size },   { 0.5f, 0, 0 }, { 1, 1 } },  // Bottom-right
        { { -size, size, size },    { 0.5f, 0, 0 }, { 1, 0 } },   // Top-right
        { { -size, size, -size },   { 0.5f, 0, 0 }, { 0, 0 } },  // Top-left

        // right
        { { size, -size, size },    { 1, 0, 0 }, { 0, 1 } },   // Bottom-left
        { { size, -size, -size },   { 1, 0, 0 }, { 1, 1 } },  // Bottom-right
        { { size, size, -size },    { 1, 0, 0 }, { 1, 0 } },   // Top-right
        { { size, size, size },     { 1, 0, 0 }, { 0, 0 } },    // Top-left

        // top
        { { -size, size, size },    { 0, 1, 0 }, { 0, 1 } },   // Bottom-left
        { { size, size, size },     { 0, 1, 0 }, { 1, 1 } },    // Bottom-right
        { { size, size, -size },    { 0, 1, 0 }, { 1, 0 } },   // Top-right
        { { -size, size, -size },   { 0, 1, 0 }, { 0, 0 } },  // Top-left

        // bottom
        { { -size, -size, -size },  { 0, 0.5f, 0 }, { 0, 1 } }, // Bottom-left
        { { size, -size, -size },   { 0, 0.5f, 0 }, { 1, 1 } },  // Bottom-right
        { { size, -size, size },    { 0, 0.5f, 0 }, { 1, 0 } },   // Top-right
        { { -size, -size, size },   { 0, 0.5f, 0 }, { 0, 0 } },  // Top-left
    };

	std::vector<UINT16> indices{
        // Front face
        0, 1, 2,
        0, 2, 3,

        // Back face
        4, 6, 5,
        4, 7, 6,

        // Left face
        8, 9, 10,
        8, 10, 11,

        // Right face
        12, 13, 14,
        12, 14, 15,

        // Top face
        16, 17, 18,
        16, 18, 19,

        // Bottom face
        20, 21, 22,
        20, 22, 23
	};

	LoadMeshData(vertices, indices);
}
