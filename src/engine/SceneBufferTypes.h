#pragma once
#include "DirectXMath.h"

struct GPUTransform
{
	GPUTransform() : data(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0
	)
	{}

	GPUTransform(const DirectX::XMMATRIX& aMatrix) : data(
		aMatrix.r[0].m128_f32[0], aMatrix.r[1].m128_f32[0], aMatrix.r[2].m128_f32[0], aMatrix.r[3].m128_f32[0],
		aMatrix.r[0].m128_f32[1], aMatrix.r[1].m128_f32[1], aMatrix.r[2].m128_f32[1], aMatrix.r[3].m128_f32[1],
		aMatrix.r[0].m128_f32[2], aMatrix.r[1].m128_f32[2], aMatrix.r[2].m128_f32[2], aMatrix.r[3].m128_f32[2]
	) {}

	GPUTransform operator=(DirectX::XMMATRIX&& aMatrix)
	{
		data = GetDataFromMatrix(aMatrix);
		return *this;
	}

	DirectX::XMFLOAT4X3 GetDataFromMatrix(const DirectX::XMMATRIX& aMatrix)
	{
		return DirectX::XMFLOAT4X3(
			aMatrix.r[0].m128_f32[0], aMatrix.r[1].m128_f32[0], aMatrix.r[2].m128_f32[0], aMatrix.r[3].m128_f32[0],
			aMatrix.r[0].m128_f32[1], aMatrix.r[1].m128_f32[1], aMatrix.r[2].m128_f32[1], aMatrix.r[3].m128_f32[1],
			aMatrix.r[0].m128_f32[2], aMatrix.r[1].m128_f32[2], aMatrix.r[2].m128_f32[2], aMatrix.r[3].m128_f32[2]
		);
	}

	DirectX::XMFLOAT4X3 data;
};


struct FrameBufferData
{
    DirectX::XMMATRIX view{};
    DirectX::XMMATRIX projection{};
	DirectX::XMFLOAT2 viewport{};
    float nearPlane = 0;
    float farPlane	= 0;
    float time		= 0;
	UINT renderPass = 0;
    float padding[23]; // Padding so the constant buffer is 256-byte aligned.
};
static_assert((sizeof(FrameBufferData) % 256) == 0, "Constant Buffer size must be 256-byte aligned");
