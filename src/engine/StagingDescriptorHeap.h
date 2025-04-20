#pragma once
#include "DescriptorHeap.h"

class StagingDescriptorHeap : public DescriptorHeap
{
public:
    StagingDescriptorHeap() = default;
    StagingDescriptorHeap(ID3D12Device* aDevice, D3D12_DESCRIPTOR_HEAP_TYPE aHeapType, UINT aNumDescriptors);
    ~StagingDescriptorHeap() final override;
};
