#include "pch.h"
#include "VertexBuffer.h"

VertexBuffer::VertexBuffer(D3D12_RESOURCE_STATES aResourceState) : ResourceBuffer(aResourceState)
{
    vbv.StrideInBytes = sizeof(Vertex);
}

void VertexBuffer::Create(const DX12* aDx12, size_t aSize)
{
    ResourceBuffer<Vertex>::Create(aDx12, aSize);
    
    vbv.SizeInBytes = static_cast<UINT>(sizeof(Vertex) * gpuSize);
    vbv.BufferLocation = resource->GetGPUVirtualAddress();
}

void VertexBuffer::Update(const DX12* aDx12)
{
    ResourceBuffer<Vertex>::Update(aDx12);

    if (!resource)
        return;
    
    vbv.SizeInBytes = static_cast<UINT>(sizeof(Vertex) * gpuSize);
    vbv.BufferLocation = resource->GetGPUVirtualAddress();
}

size_t VertexBuffer::AddItem(const DX12* aDx12, Vertex* aData, size_t aSize)
{
    size_t index = ResourceBuffer::AddItem(aDx12, aData, aSize);
    return index;
}

void VertexBuffer::RemoveItem(const DX12* aDx12, size_t aIndex)
{
    ResourceBuffer::RemoveItem(aDx12, aIndex);
}
