#pragma once
#include "DXHelper.h"
#include <queue>

#include "InstanceCountBuffer.h"
#include "ResourceBuffer.h"
#include "SceneBufferTypes.h"


class InstanceBuffer : public ResourceBuffer<InstanceData>
{
public:
    static constexpr size_t DEFAULT_INSTANCE_BUFFER_SIZE = 8192;

    explicit InstanceBuffer(class DX12* aDx12);
    ~InstanceBuffer();

    virtual void Create(ComPtr<ID3D12Device>& aDevice, size_t aSize) override;
    virtual void Update(ComPtr<ID3D12GraphicsCommandList>& aCommandList) override;
    virtual size_t AddItem(ComPtr<ID3D12Device>& aDevice, InstanceData* aData, size_t aSize = 1) override;
    virtual void RemoveItem(ComPtr<ID3D12Device>& aDevice, size_t aIndex) override;

    InstanceCountBuffer instanceCountBuffer;
    D3D12_VERTEX_BUFFER_VIEW instanceBufferView;
private:
    size_t tempModelIndex = 0;
    class DX12* dx12 = nullptr;
};

