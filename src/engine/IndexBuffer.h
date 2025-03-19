#pragma once
#include "ResourceBuffer.h"

class IndexBuffer final : public ResourceBuffer<UINT16>
{
public:
    explicit IndexBuffer(D3D12_RESOURCE_STATES aResourceState);
    
    void Create(const DX12* aDx12, size_t aSize) override;
    void Update(const DX12* aDx12) override;
    size_t AddItem(const DX12* aDx12, UINT16* aData, size_t aSize) override;
    void RemoveItem(const DX12* aDx12, size_t aIndex) override;
    
    D3D12_INDEX_BUFFER_VIEW ibv;
};
