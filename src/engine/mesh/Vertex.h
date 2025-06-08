#pragma once

struct Vertex
{
	Vector3f position;
	Vector3f color;
	Vector2f uv;
	Vector3f normal;
	Vector3f binormal;
	Vector3f tangent;
	float padding[2];
};

struct AnimatedVertex : public Vertex
{
	Vector4ui bones;
	Vector4f weights;
};

//bool operator==(const AnimatedVertex& other) const
//{
//	return memcmp(this, &other, sizeof(AnimatedVertex)) == 0;
//}

struct SimpleVertex
{
	Vector4f position;
	Vector4f color;
	Vector2f uv;
};

class VertexUtility
{
public:
	static void CalculateBinormalTangent(Vertex* vertices, const size_t& vertexCount)
	{
		size_t i;
		for (i = 0; i + 2 < vertexCount; i += 3)
		{
			Vertex& v0 = vertices[i];
			Vertex& v1 = vertices[i + 1];
			Vertex& v2 = vertices[i + 2];

			// Calculate edge vectors
			Vector3f edge1 = { v1.position.x - v0.position.x, v1.position.y - v0.position.y, v1.position.z - v0.position.z };
			Vector3f edge2 = { v2.position.x - v0.position.x, v2.position.y - v0.position.y, v2.position.z - v0.position.z };

			// Calculate delta UVs
			Vector2f deltaUV1 = { v1.uv.x - v0.uv.x, v1.uv.y - v0.uv.y };
			Vector2f deltaUV2 = { v2.uv.x - v0.uv.x, v2.uv.y - v0.uv.y };

			Vector3f tangent;
			{
				tangent.x = deltaUV2.y * edge1.x - deltaUV1.y * edge2.x;
				tangent.y = deltaUV2.y * edge1.y - deltaUV1.y * edge2.y;
				tangent.z = deltaUV2.y * edge1.z - deltaUV1.y * edge2.z;
				MathStatics::Normalize(tangent);
			}

			Vector3f binormal;
			{
				binormal.x = deltaUV1.x * edge2.x - deltaUV2.x * edge1.x;
				binormal.y = deltaUV1.x * edge2.y - deltaUV2.x * edge1.y;
				binormal.z = deltaUV1.x * edge2.z - deltaUV2.x * edge1.z;
				MathStatics::Normalize(binormal);
			}

			// Assign tangent and binormal to vertices
			v0.tangent = tangent;
			v1.tangent = tangent;
			v2.tangent = tangent;

			v0.binormal = binormal;
			v1.binormal = binormal;
			v2.binormal = binormal;
		}


		// Handle the remaining vertices (less than 3)
		if (i < vertexCount)
		{
			Vertex& v0 = vertices[i];

			if (i + 1 < vertexCount)
			{
				// Two remaining vertices
				Vertex& v1 = vertices[i + 1];

				// Calculate tangent and binormal for the remaining vertices
				Vector3f edge = { v1.position.x - v0.position.x, v1.position.y - v0.position.y, v1.position.z - v0.position.z };
				Vector2f deltaUV = { v1.uv.x - v0.uv.x, v1.uv.y - v0.uv.y };

				Vector3f tangent;
				{
					tangent.x = deltaUV.y * edge.x - deltaUV.x * edge.y;
					tangent.y = deltaUV.y * edge.y - deltaUV.x * edge.x;
					tangent.z = deltaUV.y * edge.z - deltaUV.x * edge.z;
					MathStatics::Normalize(tangent);
				}

				Vector3f binormal;
				{
					binormal.x = deltaUV.x * edge.x - deltaUV.y * edge.y;
					binormal.y = deltaUV.x * edge.y - deltaUV.y * edge.x;
					binormal.z = deltaUV.x * edge.z - deltaUV.y * edge.z;
					MathStatics::Normalize(binormal);
				}

				// Assign tangent and binormal to the remaining vertices
				v0.tangent = tangent;
				v1.tangent = tangent;

				v0.binormal = binormal;
				v1.binormal = binormal;
			}
			else
			{
				// Two remaining vertices
				Vertex& v1 = vertices[i - 1];

				// Calculate tangent and binormal for the remaining vertices
				Vector3f edge = { v0.position.x - v1.position.x, v0.position.y - v1.position.y, v0.position.z - v1.position.z };
				Vector2f deltaUV = { v0.uv.x - v1.uv.x, v0.uv.y - v1.uv.y };

				Vector3f tangent;
				{
					tangent.x = deltaUV.y * edge.x - deltaUV.x * edge.y;
					tangent.y = deltaUV.y * edge.y - deltaUV.x * edge.x;
					tangent.z = deltaUV.y * edge.z - deltaUV.x * edge.z;
					MathStatics::Normalize(tangent);
				}

				Vector3f binormal;
				{
					binormal.x = deltaUV.x * edge.x - deltaUV.y * edge.y;
					binormal.y = deltaUV.x * edge.y - deltaUV.y * edge.x;
					binormal.z = deltaUV.x * edge.z - deltaUV.y * edge.z;
					MathStatics::Normalize(binormal);
				}

				v0.tangent = tangent;
				v0.binormal = binormal;
			}
		}
	}
};
