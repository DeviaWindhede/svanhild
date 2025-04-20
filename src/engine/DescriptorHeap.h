#pragma once
#include "DXHelper.h"
#include <queue>

class DescriptorHeap
{
public:
    DescriptorHeap() = default;
    DescriptorHeap(ID3D12Device* aDevice, D3D12_DESCRIPTOR_HEAP_TYPE aHeapType, UINT aNumDescriptors, bool aIsReferencedByShader);
    virtual ~DescriptorHeap();

    void Init(ID3D12Device* aDevice, D3D12_DESCRIPTOR_HEAP_TYPE aHeapType, UINT aNumDescriptors, bool aIsReferencedByShader);

    class DescriptorHeapHandle GetNewHeapHandle();
    void FreeHeapHandle(const DescriptorHeapHandle& handle);

    ID3D12DescriptorHeap* descriptorHeap;
    D3D12_DESCRIPTOR_HEAP_TYPE heapType;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuStart;
    D3D12_GPU_DESCRIPTOR_HANDLE gpuStart;
    UINT maxDescriptors;
    UINT descriptorSize;
protected:
    bool isReferencedByShader;
private:
    UINT GetHeapIndex();

    std::queue<UINT> availableDescriptors{};
    UINT currentIndex = 0;
    UINT activeHandleCount = 0;
};
