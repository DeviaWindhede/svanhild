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

	D3D12_HEAP_PROPERTIES defaultHeapProps = { D3D12_HEAP_TYPE_DEFAULT };
	D3D12_RESOURCE_DESC instanceBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(heapSize * sizeof(InstanceData));

	aDx12->myDevice->CreateCommittedResource(
		&defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&instanceBufferDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&instanceBuffer)
	);

	D3D12_HEAP_PROPERTIES uploadHeapProps = { D3D12_HEAP_TYPE_UPLOAD };

	aDx12->myDevice->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&instanceBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&instanceUploadHeap)
	);

	instanceBufferView = {};
	instanceBufferView.BufferLocation = instanceBuffer->GetGPUVirtualAddress();
	instanceBufferView.StrideInBytes = sizeof(InstanceData);
	instanceBufferView.SizeInBytes = instanceBufferView.StrideInBytes * heapSize;

}

// TODO: MOVE TO MATH
inline static size_t NextPowerOfTwo(size_t aValue)
{
	if (aValue <= 1)
		return 1;
	unsigned long index;
	_BitScanReverse64(&index, aValue - 1);
	return 1ull << (index + 1);
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
	data = (void*)(uploadOffset * sizeof(InstanceData) + (char*)data);
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
	uploadOffset = 0;
}
