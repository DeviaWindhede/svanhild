#pragma once
#include "ResourceBuffer.h"
#include "mesh/Vertex.h"

class VertexBuffer final : public ResourceBuffer<Vertex>
{
public:
    explicit VertexBuffer(D3D12_RESOURCE_STATES aResourceState);

    void Create(ComPtr<ID3D12Device>& aDevice, size_t aSize) override;
    void Update(ComPtr<ID3D12GraphicsCommandList>& aCommandList) override;
    size_t AddItem(ComPtr<ID3D12Device>& aDevice, Vertex* aData, size_t aSize) override;
    void RemoveItem(ComPtr<ID3D12Device>& aDevice, size_t aIndex) override;
    
    D3D12_VERTEX_BUFFER_VIEW vbv;
};
