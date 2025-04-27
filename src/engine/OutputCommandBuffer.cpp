#include "pch.h"
#include "OutputCommandBuffer.h"

#include "DX12.h"

OutputCommandBuffer::OutputCommandBuffer() : ResourceBuffer(D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
{
}

void OutputCommandBuffer::Create(class DX12* aDx12, size_t aSize, UINT aIndexOffset)
{
	dx12 = aDx12;
	indexOffset = aIndexOffset;
	
    Create(dx12->myDevice, aSize);
}

size_t OutputCommandBuffer::AddItem(ComPtr<ID3D12Device>& aDevice, DrawIndirectArgs* aData, size_t aSize)
{
	for (size_t i = 0; i < aSize; i++)
		aData[i].InstanceCount = 0;
	
    return ResourceBuffer<DrawIndirectArgs>::AddItem(aDevice, aData, aSize);
}

void OutputCommandBuffer::Update(ComPtr<ID3D12GraphicsCommandList>& aComputeCommandList)
{
    ResourceBuffer<DrawIndirectArgs>::Update(aComputeCommandList);
	
	if (wasDirty)
	{
		CreateResourceViews();
	}
}

void OutputCommandBuffer::Reset()
{
	// TODO
}

void OutputCommandBuffer::Create(ComPtr<ID3D12Device>& aDevice, size_t aSize)
{
	D3D12_RESOURCE_DESC argsDesc = {};
	argsDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	argsDesc.Width = sizeof(DrawIndirectArgs) * aSize;
	argsDesc.Height = 1;
	argsDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	argsDesc.MipLevels = 1;
	argsDesc.DepthOrArraySize = 1;
	argsDesc.SampleDesc.Count = 1;
	argsDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	
	ResourceBuffer<DrawIndirectArgs>::Create(aDevice, aSize, argsDesc);

	if (aSize == 0)
		return;
	
	resource->SetName(L"outputCommandArgs");
}

void OutputCommandBuffer::CreateResourceViews()
{
	if (gpuSize == 0)
		return;

	uavArgsHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(dx12->myComputeCbvSrvUavHeap.GetPerFrameCPUHandle(indexOffset, static_cast<UINT>(ComputeUavDynamicOffsets::CommandOutput)));

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavArgsDesc = {};
	uavArgsDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavArgsDesc.Buffer.FirstElement = 0;
	uavArgsDesc.Buffer.NumElements = gpuSize;
	uavArgsDesc.Buffer.StructureByteStride = sizeof(DrawIndirectArgs);
	uavArgsDesc.Buffer.CounterOffsetInBytes = 0;
	uavArgsDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
		
	dx12->myDevice->CreateUnorderedAccessView(resource.Get(), nullptr, &uavArgsDesc, uavArgsHandle);
}
