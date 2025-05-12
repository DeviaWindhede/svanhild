#include "pch.h"
#include "DX12.h"
#include "InstanceBuffer.h"

InstanceBuffer::InstanceBuffer(class DX12* aDx12) :
ResourceBuffer<InstanceData>(D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
dx12(aDx12),
instanceCountBuffer(aDx12),
visibleInstancesBuffer {
	ResourceBuffer<UINT>(D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
	ResourceBuffer<UINT>(D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
}
{
}

InstanceBuffer::~InstanceBuffer()
{
}

void InstanceBuffer::Create(ComPtr<ID3D12Device>& aDevice, size_t aSize)
{
	ResourceBuffer<InstanceData>::Create(aDevice, aSize, CD3DX12_RESOURCE_DESC::Buffer(aSize * sizeof(InstanceData)), L"InstanceBuffer");
	
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(dx12->myComputeCbvSrvUavHeap.GetStaticCPUHandle(static_cast<UINT>(SrvOffsets::InstanceBuffer)));

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = cpuData.capacity();
		srvDesc.Buffer.StructureByteStride = sizeof(InstanceData);
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		dx12->myDevice->CreateShaderResourceView(resource.Get(), &srvDesc, handle);
		
		instanceBufferView = {};
		instanceBufferView.BufferLocation = resource->GetGPUVirtualAddress();
		instanceBufferView.StrideInBytes = sizeof(InstanceData);
		instanceBufferView.SizeInBytes = instanceBufferView.StrideInBytes * srvDesc.Buffer.NumElements;
	}
}

void InstanceBuffer::Update(ComPtr<ID3D12GraphicsCommandList>& aCommandList)
{
	ResourceBuffer<InstanceData>::Update(aCommandList);

	
	for (UINT i = 0; i < RenderConstants::FrameCount; i++)
	{
		visibleInstancesBuffer[i].Update(aCommandList);
		if (visibleInstancesBuffer[i].wasDirty && visibleInstancesBuffer[i].gpuSize > 0)
		{
			visibleInstanceUavHandle[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(dx12->myComputeCbvSrvUavHeap.GetPerFrameCPUHandle(i, static_cast<UINT>(ComputeUavDynamicOffsets::VisibleInstanceIndices)));

			D3D12_UNORDERED_ACCESS_VIEW_DESC uavArgsDesc = {};
			uavArgsDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uavArgsDesc.Buffer.FirstElement = 0;
			uavArgsDesc.Buffer.NumElements = gpuSize;
			uavArgsDesc.Buffer.StructureByteStride = sizeof(UINT);
			uavArgsDesc.Buffer.CounterOffsetInBytes = 0;
			uavArgsDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
		
			dx12->myDevice->CreateUnorderedAccessView(visibleInstancesBuffer[i].resource.Get(), nullptr, &uavArgsDesc, visibleInstanceUavHandle[i]);
		}
	}
	instanceCountBuffer.Update(aCommandList);
}

size_t InstanceBuffer::AddItem(ComPtr<ID3D12Device>& aDevice, InstanceData* aData, size_t aSize)
{
	size_t prevSize = size;
	
	size_t index = ResourceBuffer<InstanceData>::AddItem(aDevice, aData, aSize);
	{
		UINT size = NextPowerOfTwo(visibleInstancesBuffer[0].cpuData.size() + aSize + 1);
		UINT reducedSize = size - visibleInstancesBuffer[0].cpuData.size();
		std::vector<UINT> indices;
		indices.resize(reducedSize);
		for (UINT i = 0; i < RenderConstants::FrameCount; i++)
		{
			visibleInstancesBuffer[i].dirty = true;
			visibleInstancesBuffer[i].Create(aDevice, size);
			visibleInstancesBuffer[i].AddItem(aDevice, indices.data(), reducedSize);
			visibleInstancesBuffer[i].resource->SetName(std::wstring(L"VisibleInstancesBuffer" + std::to_wstring(i)).c_str());
		}
	}
	// TEMP
	{
		InstanceCountData data;
		data.offset = prevSize;
		data.instanceCount = aSize;
		instanceCountBuffer.AddItem(aDevice, &data);
		tempModelIndex++;
	}
	return index;
}

void InstanceBuffer::RemoveItem(ComPtr<ID3D12Device>& aDevice, size_t aIndex)
{
	ResourceBuffer<InstanceData>::RemoveItem(aDevice, aIndex);
	for (UINT i = 0; i < RenderConstants::FrameCount; i++)
		visibleInstancesBuffer[i].RemoveItem(aDevice, aIndex);
}
