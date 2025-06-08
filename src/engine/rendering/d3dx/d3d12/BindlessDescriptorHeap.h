#pragma once
#include "rendering/d3dx/DXHelper.h"

class BindlessDescriptorHeap
{
public:
    struct HeapSpaceDesc
    {
        UINT numCBVs;
        UINT numSRVs;
        UINT numUAVs;
        UINT numPerFrameCBVs;
        UINT numPerFrameSRVs;
        UINT numPerFrameUAVs;
    };
    
    struct HeapOffsets
    {
        UINT staticBaseOffset = 0;
        UINT perFrameBaseOffset = 0;
    };
    
    BindlessDescriptorHeap() = default;
    void Init(
        ID3D12Device* aDevice,
        const std::vector<HeapSpaceDesc>& aSpaces
    );

    D3D12_GPU_DESCRIPTOR_HANDLE GetStaticGPUHandle(UINT aIndex, UINT aSpace) const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetStaticCPUHandle(UINT aIndex, UINT aSpace) const;
    
    D3D12_GPU_DESCRIPTOR_HANDLE GetPerFrameGPUHandle(UINT aFrameIndex, UINT aIndex, UINT aSpace) const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetPerFrameCPUHandle(UINT aFrameIndex, UINT aIndex, UINT aSpace) const;

    D3D12_GPU_DESCRIPTOR_HANDLE GetStaticGPUHandleStart(UINT aSpace = 0) const { return GetStaticGPUHandle(0, aSpace); }
    D3D12_CPU_DESCRIPTOR_HANDLE GetStaticCPUHandleStart(UINT aSpace = 0) const { return GetStaticCPUHandle(0, aSpace); }
    
    D3D12_GPU_DESCRIPTOR_HANDLE GetPerFrameGPUHandleStart(UINT aFrameIndex) const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetPerFrameCPUHandleStart(UINT aFrameIndex) const;

    ID3D12DescriptorHeap* GetHeap() const { return heap.Get(); }
private:
    // TODO: Generate handles to prevent additional overhead?

    std::vector<HeapOffsets> spaceOffsets;
    ComPtr<ID3D12DescriptorHeap> heap;
    UINT descriptorSize;

    UINT totalStaticSize;
    UINT totalDynamicSize;
    UINT totalSize;
    
    D3D12_CPU_DESCRIPTOR_HANDLE startCPUHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE startGPUHandle;

    void InitializeNullDescriptors(ID3D12Device* aDevice);
    UINT GetStaticOffset(UINT aIndex, UINT aSpace) const;
    UINT GetDynamicOffset(UINT aFrameIndex, UINT aIndex, UINT aSpace) const;
};
