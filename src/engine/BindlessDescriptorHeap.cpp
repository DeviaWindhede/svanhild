#include "pch.h"
#include "BindlessDescriptorHeap.h"

void BindlessDescriptorHeap::Init(
    ID3D12Device* aDevice,
    UINT aNumCBVs, UINT aNumSRVs, UINT aNumUAVs,
    UINT aNumPerFrameCBVs, UINT aNumPerFrameSRVs, UINT aNumPerFrameUAVs
)
{
    totalSize = aNumCBVs + aNumSRVs + aNumUAVs + (aNumPerFrameCBVs + aNumPerFrameSRVs + aNumPerFrameUAVs) * RenderConstants::FrameCount;
    
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = totalSize;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    aDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));
    
    descriptorSize = aDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    capacityCBVs = aNumCBVs;
    capacitySRVs = aNumSRVs;
    capacityUAVs = aNumUAVs;
    baseTotalCapacity = capacityCBVs + capacitySRVs + capacityUAVs;

    capacityPerFrameCBVs = aNumPerFrameCBVs;
    capacityPerFrameSRVs = aNumPerFrameSRVs;
    capacityPerFrameUAVs = aNumPerFrameUAVs;
    perFrameTotalCapacity = capacityPerFrameCBVs + capacityPerFrameSRVs + capacityPerFrameUAVs;
    
    startCPUHandle = heap->GetCPUDescriptorHandleForHeapStart();
    startGPUHandle = heap->GetGPUDescriptorHandleForHeapStart();

    InitializeNullDescriptors(aDevice);
}

void BindlessDescriptorHeap::InitializeNullDescriptors(ID3D12Device* aDevice)
{
    UINT start = 0;
    UINT size = capacitySRVs;
    for (UINT i = start; i < size; ++i)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE handle = GetStaticCPUHandle(i);

        D3D12_SHADER_RESOURCE_VIEW_DESC nullDesc = {};
        nullDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        nullDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        nullDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        nullDesc.Texture2D.MipLevels = 1;

        aDevice->CreateShaderResourceView(nullptr, &nullDesc, handle);
    }

    start = size;
    size += capacitySRVs;
    for (UINT i = start; i < size; ++i)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE handle = GetStaticCPUHandle(i);

        D3D12_SHADER_RESOURCE_VIEW_DESC nullDesc = {};
        nullDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        nullDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        nullDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        nullDesc.Texture2D.MipLevels = 1;

        aDevice->CreateShaderResourceView(nullptr, &nullDesc, handle);
    }

    start = size;
    size += capacityUAVs;
    for (UINT i = start; i < size; ++i)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE handle = GetStaticCPUHandle(i);

        D3D12_UNORDERED_ACCESS_VIEW_DESC nullUAVDesc = {};
        nullUAVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        nullUAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        nullUAVDesc.Texture2D.MipSlice = 0;

        aDevice->CreateUnorderedAccessView(nullptr, nullptr, &nullUAVDesc, handle);
    }
}

D3D12_GPU_DESCRIPTOR_HANDLE BindlessDescriptorHeap::GetStaticGPUHandle(UINT aIndex) const
{
    D3D12_GPU_DESCRIPTOR_HANDLE handle = startGPUHandle;
    handle.ptr += static_cast<UINT64>(aIndex) * descriptorSize;
    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE BindlessDescriptorHeap::GetStaticCPUHandle(UINT aIndex) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = startCPUHandle;
    handle.ptr += static_cast<UINT64>(aIndex) * descriptorSize;
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE BindlessDescriptorHeap::GetPerFrameGPUHandle(UINT aFrameIndex, UINT aIndex) const
{
    D3D12_GPU_DESCRIPTOR_HANDLE handle = startGPUHandle;
    handle.ptr += static_cast<UINT64>(baseTotalCapacity + aFrameIndex * perFrameTotalCapacity + aIndex) * descriptorSize;
    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE BindlessDescriptorHeap::GetPerFrameCPUHandle(UINT aFrameIndex, UINT aIndex) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = startCPUHandle;
    handle.ptr += static_cast<UINT64>(baseTotalCapacity + aFrameIndex * perFrameTotalCapacity + aIndex) * descriptorSize;
    return handle;
}
