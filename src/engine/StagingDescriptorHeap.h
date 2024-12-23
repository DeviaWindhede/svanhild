#pragma once
#include "DescriptorHeap.h"
#include <queue>

class StagingDescriptorHeap : public DescriptorHeap
{
public:
    StagingDescriptorHeap() = default;
    StagingDescriptorHeap(ID3D12Device* aDevice, D3D12_DESCRIPTOR_HEAP_TYPE aHeapType, UINT aNumDescriptors);
    ~StagingDescriptorHeap() final;

    class DescriptorHeapHandle GetNewHeapHandle();
    void FreeHeapHandle(const DescriptorHeapHandle& handle);

private:
    UINT GetHeapIndex();

    std::queue<UINT> availableDescriptors{};
    UINT currentIndex = 0;
    UINT activeHandleCount = 0;
};
