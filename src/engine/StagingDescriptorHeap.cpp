#include "pch.h"
#include "StagingDescriptorHeap.h"
#include "DescriptorHeapHandle.h"

StagingDescriptorHeap::StagingDescriptorHeap(ID3D12Device* aDevice, D3D12_DESCRIPTOR_HEAP_TYPE aHeapType, UINT aNumDescriptors)
	:DescriptorHeap(aDevice, aHeapType, aNumDescriptors, false)
{
}

StagingDescriptorHeap::~StagingDescriptorHeap()
{
	if (activeHandleCount == 0)
		return;

	throw "Active handles were never freed";
}

DescriptorHeapHandle StagingDescriptorHeap::GetNewHeapHandle()
{
	UINT index = GetHeapIndex();

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = cpuStart;
	cpuHandle.ptr += index * descriptorSize;

	DescriptorHeapHandle result;
	result.cpuHandle = cpuHandle;
	result.heapIndex = index;

	activeHandleCount++;

	return result;
}

void StagingDescriptorHeap::FreeHeapHandle(const DescriptorHeapHandle& handle)
{
	availableDescriptors.push(handle.heapIndex);

	if (activeHandleCount == 0)
	{
		throw "Freeing heap handles when there should be none left";
	}
	activeHandleCount--;
}

UINT StagingDescriptorHeap::GetHeapIndex()
{
	if (currentIndex < maxDescriptors)
	{
		return currentIndex++;
	}

	if (availableDescriptors.size() > 0)
	{
		UINT index = availableDescriptors.front();
		availableDescriptors.pop();
		return index;
	}

	throw "Ran out of dynamic descriptor heap handles, need to increase heap size.";

}
