#include "pch.h"
#include "SpherePrimitive.h"

void SpherePrimitive::InitPrimitive(int aResolution)
{
    std::vector<Vertex> vertices;
    std::vector<UINT16> indices;

    float radius = 0.5f;

    for (int y = 0; y <= aResolution; ++y) {
        for (int x = 0; x <= aResolution; ++x) {
            float xSegment = (float)x / (float)aResolution;
            float ySegment = (float)y / (float)aResolution;
            float xPos = radius * static_cast<float>(std::cos(xSegment * 2.0f * M_PI) * std::sin(ySegment * M_PI));
            float yPos = radius * static_cast<float>(std::cos(ySegment * M_PI));
            float zPos = radius * static_cast<float>(std::sin(xSegment * 2.0f * M_PI) * std::sin(ySegment * M_PI));

            vertices.push_back({
                { xPos, yPos, zPos },
                { xPos / radius, yPos / radius, zPos / radius },
                { xSegment, ySegment }
                });
        }
    }

    for (int y = 0; y < aResolution; ++y) {
        for (int x = 0; x < aResolution; ++x) {
            int first = (y * (aResolution + 1)) + x;
            int second = first + aResolution + 1;

            indices.push_back(first + 1);
            indices.push_back(second);
            indices.push_back(first);

            indices.push_back(first + 1);
            indices.push_back(second + 1);
            indices.push_back(second);
        }
    }

    LoadMeshData(vertices, indices);
}
