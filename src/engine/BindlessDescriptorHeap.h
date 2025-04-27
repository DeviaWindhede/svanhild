#pragma once
#include "DXHelper.h"
#include "RenderConstants.h"

class BindlessDescriptorHeap
{
public:
    BindlessDescriptorHeap() = default;
    void Init(
        ID3D12Device* aDevice,
        UINT aNumCBVs, UINT aNumSRVs, UINT aNumUAVs,
        UINT aNumPerFrameCBVs, UINT aNumPerFrameSRVs, UINT aNumPerFrameUAVs
    );

    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const { return GetStaticCPUHandle(0); }
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const { return GetStaticGPUHandle(0); }
    
    D3D12_GPU_DESCRIPTOR_HANDLE GetStaticGPUHandle(UINT aIndex) const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetStaticCPUHandle(UINT aIndex) const;
    
    D3D12_GPU_DESCRIPTOR_HANDLE GetPerFrameGPUHandle(UINT aFrameIndex, UINT aIndex) const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetPerFrameCPUHandle(UINT aFrameIndex, UINT aIndex) const;

    ID3D12DescriptorHeap* GetHeap() const { return heap.Get(); }
private:
    // TODO: Generate handles to prevent additional overhead?
    
    ComPtr<ID3D12DescriptorHeap> heap;
    UINT descriptorSize;

    UINT totalSize;
    UINT baseTotalCapacity;
    UINT perFrameTotalCapacity;
    
    UINT capacityCBVs;
    UINT capacitySRVs;
    UINT capacityUAVs;
    
    UINT capacityPerFrameCBVs;
    UINT capacityPerFrameSRVs;
    UINT capacityPerFrameUAVs;
    
    D3D12_CPU_DESCRIPTOR_HANDLE startCPUHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE startGPUHandle;

    void InitializeNullDescriptors(ID3D12Device* aDevice);
};
