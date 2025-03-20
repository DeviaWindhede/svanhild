#include "pch.h"
#include "DX12.h"
#include "InstanceBuffer.h"

InstanceBuffer::~InstanceBuffer()
{
	delete[] cpuInstanceData;
	cpuInstanceData = nullptr;
}

void InstanceBuffer::Create(DX12* aDx12, size_t aSize, bool aShouldCopy)
{
	InstanceData* newCpuHeap = new InstanceData[aSize];
	if (cpuInstanceData)
	{
		size_t copySize = heapSize;
		if (aSize > heapSize)
		{
			for (size_t i = aSize - heapSize; i < aSize; i++)
			{
				availableIndices.push(i);
			}
		}
		else
			copySize = aSize;
		
		if (aShouldCopy)
			memcpy(newCpuHeap, cpuInstanceData, heapSize < aSize ? heapSize : aSize);
		
		delete[] cpuInstanceData;
		cpuInstanceData = nullptr;
	}
	cpuInstanceData = newCpuHeap;
	heapSize = aSize;

	D3D12_HEAP_PROPERTIES heapProps = { D3D12_HEAP_TYPE_DEFAULT };
	D3D12_RESOURCE_DESC instanceBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(heapSize * sizeof(InstanceData));

	// instance buffer
	{
		// buffer
		{
			instanceBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

			aDx12->myDevice->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&instanceBufferDesc,
				D3D12_RESOURCE_STATE_COMMON,
				nullptr,
				IID_PPV_ARGS(&instanceBuffer)
			);
			NAME_D3D12_OBJECT(instanceBuffer);

			instanceBufferView = {};
			instanceBufferView.BufferLocation = instanceBuffer->GetGPUVirtualAddress();
			instanceBufferView.StrideInBytes = sizeof(InstanceData);
			instanceBufferView.SizeInBytes = instanceBufferView.StrideInBytes * heapSize;
		}

		// upload
		{
			heapProps = { D3D12_HEAP_TYPE_UPLOAD };
			instanceBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			aDx12->myDevice->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&instanceBufferDesc, // Size to be optimized
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&instanceUploadHeap)
			);
			NAME_D3D12_OBJECT(instanceUploadHeap);
		}

		//// SRV
		//{
		//	heapProps = { D3D12_HEAP_TYPE_DEFAULT };

		//	D3D12_RESOURCE_DESC visibleDesc = instanceBufferDesc;
		//	visibleDesc.Width = sizeof(UINT) * heapSize; // instance indices
		//	visibleDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		//	aDx12->myDevice->CreateCommittedResource(
		//		&heapProps,
		//		D3D12_HEAP_FLAG_NONE,
		//		&visibleDesc,
		//		D3D12_RESOURCE_STATE_GENERIC_READ,
		//		nullptr,
		//		IID_PPV_ARGS(&shaderVisibleBuffer)
		//	);
		//	NAME_D3D12_OBJECT(shaderVisibleBuffer);
		//}
	}





	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(aDx12->myComputeCbvSrvUavHeap.cpuStart);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = heapSize;
		srvDesc.Buffer.StructureByteStride = sizeof(InstanceData);
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		aDx12->myDevice->CreateShaderResourceView(instanceBuffer.Get(), &srvDesc, handle);

		//handle.Offset(1, aDx12->cbvSrvUavDescriptorSize);

		//D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		//uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		//uavDesc.Buffer.FirstElement = 0;
		//uavDesc.Buffer.NumElements = heapSize;
		//uavDesc.Buffer.StructureByteStride = sizeof(UINT);
		//aDx12->myDevice->CreateUnorderedAccessView(shaderVisibleBuffer.Get(), nullptr, &uavDesc, handle);
		//uavHandle = handle;

		//handle.Offset(1, aDx12->cbvSrvUavDescriptorSize);
		//
		//D3D12_UNORDERED_ACCESS_VIEW_DESC uavArgsDesc = {};
		//uavArgsDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		//uavArgsDesc.Buffer.FirstElement = 0;
		//uavArgsDesc.Buffer.NumElements = heapSize;
		//uavArgsDesc.Buffer.StructureByteStride = sizeof(IndirectCommand);
		//aDx12->myDevice->CreateUnorderedAccessView(indirectArgsBuffer.Get(), nullptr, &uavArgsDesc, handle);
		//uavArgsHandle = handle;
	}

}


void InstanceBuffer::Update(DX12& aDx12, const std::vector<InstanceData>& aInstances, size_t aOffset)
{
	if (aInstances.size() > heapSize)
	{
		instanceBuffer.Reset();
		Create(&aDx12, NextPowerOfTwo(aInstances.size()));
	}

	void* data = nullptr;
	ThrowIfFailed(instanceUploadHeap->Map(0, nullptr, &data));
	data = static_cast<void*>(uploadOffset * sizeof(InstanceData) + static_cast<char*>(data));
	memcpy(data, aInstances.data(), sizeof(InstanceData) * aInstances.size());
	instanceUploadHeap->Unmap(0, nullptr);

	aDx12.myCommandList->CopyBufferRegion(
		instanceBuffer.Get(),
		aOffset * sizeof(InstanceData),  // Update with proper offset for chunk
		instanceUploadHeap.Get(),
		uploadOffset * sizeof(InstanceData),
		sizeof(InstanceData) * aInstances.size()
	);

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		instanceBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
	);
	aDx12.myCommandList->ResourceBarrier(1, &barrier);

	uploadOffset += aInstances.size();
}

void InstanceBuffer::Initialize(DX12* aDx12)
{
	// TODO: Add intermediate upload buffer to keep static objects always loaded
}

void InstanceBuffer::OnEndFrame(DX12* aDx12)
{
	cpuSize = uploadOffset;
	uploadOffset = 0;
}
