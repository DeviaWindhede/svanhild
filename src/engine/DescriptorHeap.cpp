#include "pch.h"
#include "DescriptorHeap.h"

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
    if (!descriptorHeap)
        return;

    descriptorHeap->Release();
    descriptorHeap = NULL;
}
