#include "pch.h"
#include "IndexBuffer.h"

IndexBuffer::IndexBuffer(D3D12_RESOURCE_STATES aResourceState) : ResourceBuffer(aResourceState)
{
    ibv.Format = DXGI_FORMAT_R16_UINT; // DXGI_FORMAT_R32_UINT
}

void IndexBuffer::Create(const DX12* aDx12, size_t aSize)
{
    ResourceBuffer<unsigned short>::Create(aDx12, aSize);
    
    ibv.SizeInBytes = static_cast<UINT>(sizeof(UINT16) * gpuSize);
    ibv.BufferLocation = resource->GetGPUVirtualAddress();
}

void IndexBuffer::Update(const DX12* aDx12)
{
    ResourceBuffer<unsigned short>::Update(aDx12);
    
    if (!resource)
        return;
    
    ibv.SizeInBytes = static_cast<UINT>(sizeof(UINT16) * gpuSize);
    ibv.BufferLocation = resource->GetGPUVirtualAddress();
}

size_t IndexBuffer::AddItem(const DX12* aDx12, UINT16* aData, size_t aSize)
{
    size_t index = ResourceBuffer::AddItem(aDx12, aData, aSize);
    return index;
}

void IndexBuffer::RemoveItem(const DX12* aDx12, size_t aIndex)
{
    ResourceBuffer::RemoveItem(aDx12, aIndex);
}
