#include "pch.h"
#include "FrameBuffer.h"
#include "app/ApplicationTimer.h"
#include "math/Camera.h"
#include "rendering/d3dx/d3d12/DX12.h"

void FrameBuffer::Init(ID3D12Device* aDevice, D3D12_HEAP_TYPE aHeapType, const FrameBufferConstantData* aDefaultData, size_t aDataSize, bool aShouldUnmap)
{
	CbvResource<FrameBufferConstantData>::Init(aDevice, aHeapType, aDefaultData, aDataSize, aShouldUnmap);
	resource->SetName(L"FrameBuffer");
}

void FrameBuffer::Update(class DX12& aDx12, Camera& aCamera, ApplicationTimer& aTimer)
{
	data[0].data.view = aCamera.View();
	data[0].data.projection = aCamera.Projection();
	data[0].data.viewport.bounds = DirectX::XMFLOAT2(static_cast<float>(aDx12.myViewport.Width), static_cast<float>(aDx12.myViewport.Height));
	data[0].data.viewport.nearPlane = aCamera.nearPlane;
	data[0].data.viewport.farPlane = aCamera.farPlane;
	data[0].data.time = aTimer.GetTotalTime();
	data[0].data.frameIndex = aDx12.myFrameIndex;

	ExtractFrustumPlanes(data[0].data.frustumPlanes, data[0].data.view, true);

	memcpy(cbvDataBegin, &data, sizeof(data));
}

void FrameBuffer::ExtractFrustumPlanes(FrustumPlane outPlanes[6], const DirectX::XMMATRIX& viewProj, bool normalize)
{
	// Combine view and projection
	DirectX::XMMATRIX m = viewProj;

	// Row-major elements of the matrix
	DirectX::XMVECTOR r0 = m.r[0]; // X
	DirectX::XMVECTOR r1 = m.r[1]; // Y
	DirectX::XMVECTOR r2 = m.r[2]; // Z
	DirectX::XMVECTOR r3 = m.r[3]; // W

	// Left:     row 4 + row 1
	DirectX::XMVECTOR leftPlane   = DirectX::XMVectorAdd(r3, r0);
	// Right:    row 4 - row 1
	DirectX::XMVECTOR rightPlane  = DirectX::XMVectorSubtract(r3, r0);
	// Bottom:   row 4 + row 2
	DirectX::XMVECTOR bottomPlane = DirectX::XMVectorAdd(r3, r1);
	// Top:      row 4 - row 2
	DirectX::XMVECTOR topPlane    = DirectX::XMVectorSubtract(r3, r1);
	// Near:     row 4 + row 3
	DirectX::XMVECTOR nearPlane   = DirectX::XMVectorAdd(r3, r2);
	// Far:      row 4 - row 3
	DirectX::XMVECTOR farPlane    = DirectX::XMVectorSubtract(r3, r2);

	DirectX::XMVECTOR planes[6] = {
		leftPlane, rightPlane,
		bottomPlane, topPlane,
		nearPlane, farPlane
	};

	for (int i = 0; i < 6; ++i)
	{
		DirectX::XMVECTOR p = planes[i];
		if (normalize)
		{
			// Normalize the plane equation
			DirectX::XMVECTOR n = DirectX::XMVector3Normalize(DirectX::XMVectorSet(p.m128_f32[0], p.m128_f32[1], p.m128_f32[2], 0.0f));
			float d = p.m128_f32[3] / DirectX::XMVectorGetX(DirectX::XMVector3Length(n));
			XMStoreFloat3(&outPlanes[i].normal, n);
			outPlanes[i].d = d;
		}
		else
		{
			XMStoreFloat3(&outPlanes[i].normal, p);
			outPlanes[i].d = p.m128_f32[3];
		}
	}
}
