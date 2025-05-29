#pragma once
#include "ResourceBuffer.h"
#include "math/AABB.h"

struct InstanceCountData
{
    AABB aabb;
    UINT instanceCount;
    UINT offset;
};

class InstanceCountBuffer final : public ResourceBuffer<InstanceCountData>
{
public:
    explicit InstanceCountBuffer(class DX12* aDx12);

    void Create(ComPtr<ID3D12Device>& aDevice, size_t aSize) override;
    
private:
    class DX12* dx12 = nullptr;
};
