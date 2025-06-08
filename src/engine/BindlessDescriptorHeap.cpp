#include "pch.h"
#include "BindlessDescriptorHeap.h"

void BindlessDescriptorHeap::Init(
    ID3D12Device* aDevice,
    const std::vector<HeapSpaceDesc>& aSpaces
)
{
    totalSize = 0;
    
    for (int i = 0; i < aSpaces.size(); i++)
    {
        const HeapSpaceDesc& space = aSpaces[i];
        
        spaceOffsets.push_back({
            .staticBaseOffset = totalStaticSize,
            .perFrameBaseOffset = totalDynamicSize
        });

        UINT statics = space.numCBVs + space.numSRVs + space.numUAVs;
        UINT dynamics = space.numPerFrameCBVs + space.numPerFrameSRVs + space.numPerFrameUAVs;
        
        totalStaticSize += statics;
        totalDynamicSize += dynamics;
        totalSize += statics + dynamics * RenderConstants::FrameCount;
    }
    
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = totalSize;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    aDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));
    
    descriptorSize = aDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    
    startCPUHandle = heap->GetCPUDescriptorHandleForHeapStart();
    startGPUHandle = heap->GetGPUDescriptorHandleForHeapStart();

    InitializeNullDescriptors(aDevice);
}

void BindlessDescriptorHeap::InitializeNullDescriptors(ID3D12Device* aDevice)
{
    for (UINT i = 0; i < totalSize; ++i)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE handle = GetStaticCPUHandle(i, 0);

        D3D12_SHADER_RESOURCE_VIEW_DESC nullDesc = {};
        nullDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        nullDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        nullDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        nullDesc.Texture2D.MipLevels = 0;

        aDevice->CreateShaderResourceView(nullptr, &nullDesc, handle);
    }
    // TODO: Track open slots
}

UINT BindlessDescriptorHeap::GetStaticOffset(UINT aIndex, UINT aSpace) const
{
    UINT offset = spaceOffsets[aSpace].staticBaseOffset + aIndex;
    return offset * descriptorSize;
}

UINT BindlessDescriptorHeap::GetDynamicOffset(UINT aFrameIndex, UINT aIndex, UINT aSpace) const
{
    UINT staticOffset = totalStaticSize * descriptorSize;
    
    UINT dynamicOffset = spaceOffsets[aSpace].perFrameBaseOffset + aIndex;
    dynamicOffset += aFrameIndex * totalDynamicSize;
    dynamicOffset *= descriptorSize;
    
    return dynamicOffset + staticOffset;
}

D3D12_GPU_DESCRIPTOR_HANDLE BindlessDescriptorHeap::GetStaticGPUHandle(UINT aIndex, UINT aSpace) const
{
    D3D12_GPU_DESCRIPTOR_HANDLE handle = startGPUHandle;
    handle.ptr += GetStaticOffset(aIndex, aSpace);
    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE BindlessDescriptorHeap::GetStaticCPUHandle(UINT aIndex, UINT aSpace) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = startCPUHandle;
    handle.ptr += GetStaticOffset(aIndex, aSpace);
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE BindlessDescriptorHeap::GetPerFrameGPUHandle(UINT aFrameIndex, UINT aIndex, UINT aSpace) const
{
    D3D12_GPU_DESCRIPTOR_HANDLE handle = startGPUHandle;
    handle.ptr += GetDynamicOffset(aFrameIndex, aIndex, aSpace);
    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE BindlessDescriptorHeap::GetPerFrameCPUHandle(UINT aFrameIndex, UINT aIndex, UINT aSpace) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = startCPUHandle;
    handle.ptr += GetDynamicOffset(aFrameIndex, aIndex, aSpace);
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE BindlessDescriptorHeap::GetPerFrameGPUHandleStart(UINT aFrameIndex) const
{
    D3D12_GPU_DESCRIPTOR_HANDLE handle = startGPUHandle;
    handle.ptr += static_cast<UINT64>(totalStaticSize * descriptorSize + aFrameIndex * totalDynamicSize * descriptorSize);
    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE BindlessDescriptorHeap::GetPerFrameCPUHandleStart(UINT aFrameIndex) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = startCPUHandle;
    handle.ptr += static_cast<UINT64>(totalStaticSize * descriptorSize + aFrameIndex * totalDynamicSize * descriptorSize);
    return handle;
}
