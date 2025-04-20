#include "pch.h"
#include "DescriptorHeap.h"

#include "DescriptorHeapHandle.h"

DescriptorHeap::DescriptorHeap(ID3D12Device* aDevice, D3D12_DESCRIPTOR_HEAP_TYPE aHeapType, UINT aNumDescriptors, bool aIsReferencedByShader)
{
    Init(aDevice, aHeapType, aNumDescriptors, aIsReferencedByShader);
}

void DescriptorHeap::Init(ID3D12Device* aDevice, D3D12_DESCRIPTOR_HEAP_TYPE aHeapType, UINT aNumDescriptors, bool aIsReferencedByShader)
{
    heapType = aHeapType;
    maxDescriptors = aNumDescriptors;
    isReferencedByShader = aIsReferencedByShader;

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
    heapDesc.NumDescriptors = maxDescriptors;
    heapDesc.Type = heapType;
    heapDesc.Flags = isReferencedByShader ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heapDesc.NodeMask = 0;

    ThrowIfFailed(aDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&descriptorHeap)));

    cpuStart = descriptorHeap->GetCPUDescriptorHandleForHeapStart();

    if (isReferencedByShader)
    {
        gpuStart = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
    }

    descriptorSize = aDevice->GetDescriptorHandleIncrementSize(heapType);
}

DescriptorHeap::~DescriptorHeap()
{
    if (descriptorHeap)
    {
        descriptorHeap->Release();
        descriptorHeap = NULL;
    }

    if (activeHandleCount == 0)
        return;

    throw "Active handles were never freed";
}

DescriptorHeapHandle DescriptorHeap::GetNewHeapHandle()
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

void DescriptorHeap::FreeHeapHandle(const DescriptorHeapHandle& handle)
{
    availableDescriptors.push(handle.heapIndex);

    if (activeHandleCount == 0)
    {
        throw "Freeing heap handles when there should be none left";
    }
    activeHandleCount--;
}

UINT DescriptorHeap::GetHeapIndex()
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
