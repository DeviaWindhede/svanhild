#pragma once
#include <DirectXMath.h>

typedef DirectX::XMFLOAT2 Vector2f;
typedef DirectX::XMFLOAT3 Vector3f;
typedef DirectX::XMFLOAT4 Vector4f;
typedef DirectX::XMUINT4 Vector4ui;

typedef DirectX::XMFLOAT4 Color4f;
typedef DirectX::XMFLOAT4X4 Matrix4f;


class MathStatics
{
public:
	static inline void Normalize(Vector2f Vector)
	{
		float length = static_cast<float>(sqrt(Vector.x * Vector.x + Vector.y * Vector.y));
		Vector.x /= length;
		Vector.y /= length;
	};

	static inline void Normalize(Vector3f& Vector)
	{
		float length = static_cast<float>(sqrt(Vector.x * Vector.x + Vector.y * Vector.y + Vector.z * Vector.z));
		Vector.x /= length;
		Vector.y /= length;
		Vector.z /= length;
	};

	static inline void Normalize(Vector4f& Vector)
	{
		float length = static_cast<float>(sqrt(Vector.x * Vector.x + Vector.y * Vector.y + Vector.z * Vector.z));
		Vector.x /= length;
		Vector.y /= length;
		Vector.z /= length;
	};
};
