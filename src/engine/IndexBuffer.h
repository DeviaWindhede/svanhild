#pragma once
#include "ResourceBuffer.h"

class IndexBuffer final : public ResourceBuffer<UINT16>
{
public:
    explicit IndexBuffer(D3D12_RESOURCE_STATES aResourceState);
    
    void Create(ComPtr<ID3D12Device>& aDevice, size_t aSize) override;
    void Update(ComPtr<ID3D12GraphicsCommandList>& aDx12) override;
    size_t AddItem(ComPtr<ID3D12Device>& aDevice, UINT16* aData, size_t aSize) override;
    void RemoveItem(ComPtr<ID3D12Device>& aDevice, size_t aIndex) override;
    
    D3D12_INDEX_BUFFER_VIEW ibv;
};
