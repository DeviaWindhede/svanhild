#include "pch.h"
#include "IndexBuffer.h"

IndexBuffer::IndexBuffer(D3D12_RESOURCE_STATES aResourceState) : ResourceBuffer(aResourceState)
{
    ibv.Format = DXGI_FORMAT_R16_UINT; // DXGI_FORMAT_R32_UINT
}

void IndexBuffer::Create(ComPtr<ID3D12Device>& aDevice, size_t aSize)
{
    ResourceBuffer<unsigned short>::Create(aDevice, aSize);
    
    ibv.SizeInBytes = static_cast<UINT>(sizeof(UINT16) * gpuSize);
    ibv.BufferLocation = resource->GetGPUVirtualAddress();
}

void IndexBuffer::Update(ComPtr<ID3D12GraphicsCommandList>& aCommandList)
{
    ResourceBuffer<unsigned short>::Update(aCommandList);
    
    if (!resource)
        return;
    
    ibv.SizeInBytes = static_cast<UINT>(sizeof(UINT16) * gpuSize);
    ibv.BufferLocation = resource->GetGPUVirtualAddress();
}

size_t IndexBuffer::AddItem(ComPtr<ID3D12Device>& aDevice, UINT16* aData, size_t aSize)
{
    size_t index = ResourceBuffer::AddItem(aDevice, aData, aSize);
    return index;
}

void IndexBuffer::RemoveItem(ComPtr<ID3D12Device>& aDevice, size_t aIndex)
{
    ResourceBuffer::RemoveItem(aDevice, aIndex);
}
