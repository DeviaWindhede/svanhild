#include "pch.h"
#include "StagingDescriptorHeap.h"

StagingDescriptorHeap::StagingDescriptorHeap(ID3D12Device* aDevice, D3D12_DESCRIPTOR_HEAP_TYPE aHeapType, UINT aNumDescriptors)
	:DescriptorHeap(aDevice, aHeapType, aNumDescriptors, false)
{
}

StagingDescriptorHeap::~StagingDescriptorHeap()
{
	DescriptorHeap::~DescriptorHeap();
}
