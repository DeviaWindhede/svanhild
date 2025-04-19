#include "pch.h"
#include "FrameBuffer.h"
#include "DX12.h"
#include "Camera.h"
#include "ApplicationTimer.h"

void FrameBuffer::Init(ID3D12Device* aDevice, D3D12_HEAP_TYPE aHeapType, const FrameBufferConstantData* aDefaultData, size_t aDataSize, bool aShouldUnmap)
{
	CbvResource<FrameBufferConstantData>::Init(aDevice, aHeapType, aDefaultData, aDataSize, aShouldUnmap);
}

void FrameBuffer::Update(class DX12& aDx12, Camera& aCamera, ApplicationTimer& aTimer)
{
	data[0].data.projection = aCamera.Projection();
	data[0].data.view = aCamera.View();
	data[0].data.nearPlane = aCamera.nearPlane;
	data[0].data.farPlane = aCamera.farPlane;
	data[0].data.viewport = DirectX::XMFLOAT2(static_cast<float>(aDx12.myViewport.Width), static_cast<float>(aDx12.myViewport.Height));
	data[0].data.time = aTimer.GetTotalTime();
	data[0].data.frameIndex = aDx12.myFrameIndex;

	memcpy(cbvDataBegin, &data, sizeof(data));
}
