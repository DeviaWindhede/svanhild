#pragma once
#include "ResourceBuffer.h"
#include "mesh/Vertex.h"

class VertexBuffer final : public ResourceBuffer<Vertex>
{
public:
    explicit VertexBuffer(D3D12_RESOURCE_STATES aResourceState);

    void Create(const DX12* aDx12, size_t aSize) override;
    void Update(const DX12* aDx12) override;
    size_t AddItem(const DX12* aDx12, Vertex* aData, size_t aSize) override;
    void RemoveItem(const DX12* aDx12, size_t aIndex) override;
    
    D3D12_VERTEX_BUFFER_VIEW vbv;
};
