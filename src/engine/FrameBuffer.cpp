#include "pch.h"
#include "FrameBuffer.h"
#include "DX12.h"
#include "Camera.h"

void FrameBuffer::Init(class DX12& aDx12)
{
	const UINT descriptorSize = aDx12.myDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const UINT constantBufferSize = sizeof(FrameBufferData);    // CB size is required to be 256-byte aligned.

	ThrowIfFailed(aDx12.myDevice->CreateCommittedResource(
		&keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)),
		D3D12_HEAP_FLAG_NONE,
		&keep(CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&resource)));

	// Describe and create a constant buffer view.
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc[1] = {};
	cbvDesc[0].BufferLocation = resource->GetGPUVirtualAddress();
	cbvDesc[0].SizeInBytes = constantBufferSize;

	// Map and initialize the constant buffer. We don't unmap this until the
	// app closes. Keeping things mapped for the lifetime of the resource is okay.
	CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
	ThrowIfFailed(resource->Map(0, &readRange, reinterpret_cast<void**>(&frameBufferCbvDataBegin)));
	memcpy(frameBufferCbvDataBegin, &frameBufferData, sizeof(frameBufferData));
}

void FrameBuffer::Update(class DX12& aDx12, Camera& aCamera)
{
	frameBufferData.projection = aCamera.Projection();
	frameBufferData.view = aCamera.View();
	frameBufferData.nearPlane = aCamera.nearPlane;
	frameBufferData.farPlane = aCamera.farPlane;
	frameBufferData.viewport = DirectX::XMFLOAT2(static_cast<float>(aDx12.myViewport.Width), static_cast<float>(aDx12.myViewport.Height));

	memcpy(frameBufferCbvDataBegin, &frameBufferData, sizeof(frameBufferData));
}
