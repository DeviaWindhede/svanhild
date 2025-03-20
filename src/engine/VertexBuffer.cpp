#include "pch.h"
#include "VertexBuffer.h"

VertexBuffer::VertexBuffer(D3D12_RESOURCE_STATES aResourceState) : ResourceBuffer(aResourceState)
{
    vbv.StrideInBytes = sizeof(Vertex);
}

void VertexBuffer::Create(ComPtr<ID3D12Device>& aDevice, size_t aSize)
{
    ResourceBuffer<Vertex>::Create(aDevice, aSize);
    
    vbv.SizeInBytes = static_cast<UINT>(sizeof(Vertex) * gpuSize);
    vbv.BufferLocation = resource->GetGPUVirtualAddress();
}

void VertexBuffer::Update(ComPtr<ID3D12GraphicsCommandList>& aCommandList)
{
    ResourceBuffer<Vertex>::Update(aCommandList);

    if (!resource)
        return;
    
    vbv.SizeInBytes = static_cast<UINT>(sizeof(Vertex) * gpuSize);
    vbv.BufferLocation = resource->GetGPUVirtualAddress();
}

size_t VertexBuffer::AddItem(ComPtr<ID3D12Device>& aDevice, Vertex* aData, size_t aSize)
{
    size_t index = ResourceBuffer::AddItem(aDevice, aData, aSize);
    return index;
}

void VertexBuffer::RemoveItem(ComPtr<ID3D12Device>& aDevice, size_t aIndex)
{
    ResourceBuffer::RemoveItem(aDevice, aIndex);
}
