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

struct InstanceData
{
	GPUTransform transform;
	//DirectX::XMFLOAT4 color;
	UINT modelIndex;
};

// Plane equation: dot(n, P) + d = 0
struct FrustumPlane {
	DirectX::XMFLOAT3 normal;
	float d;
};

struct alignas(4) FrameBufferViewport
{
	DirectX::XMFLOAT2 bounds{};
	float nearPlane = 0;
	float farPlane	= 0;
};

struct FrameBufferData
{
    DirectX::XMMATRIX view{};
    DirectX::XMMATRIX projection{};
	FrameBufferViewport viewport{};
    float time		= 0;
	UINT renderPass = 0;
	UINT frameIndex = 0;
	float padding	= 0;
	FrustumPlane frustumPlanes[6]{};
};
static_assert((sizeof(FrameBufferData) % 4) == 0, "must be 4-byte aligned");

struct FrameBufferConstantData
{
	FrameBufferData data;
	//float padding[(256 - sizeof(FrameBufferData)) / 4]; // Padding so the constant buffer is 256-byte aligned.
};

static_assert((sizeof(FrameBufferConstantData) % 256) == 0, "Constant Buffer size must be 256-byte aligned");
