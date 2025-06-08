#pragma once
#include "ResourceBuffer.h"

struct DrawIndirectArgsData
{
    UINT StartInstanceOffset;
    UINT MeshIndex;
};

struct DrawIndirectArgs {
    DrawIndirectArgsData data;
    D3D12_DRAW_INDEXED_ARGUMENTS drawArgs;
};

class OutputCommandBuffer : public ResourceBuffer<DrawIndirectArgs>
{
public:
    explicit OutputCommandBuffer();
    void Create(class DX12* aDx12, size_t aSize, UINT aIndexOffset);
    size_t AddItem(ComPtr<ID3D12Device>& aDevice, DrawIndirectArgs* aData, size_t aSize) override;
    void Update(ComPtr<ID3D12GraphicsCommandList>& aComputeCommandList) override;
    void Reset();

    CD3DX12_CPU_DESCRIPTOR_HANDLE uavArgsHandle;
private:
    void Create(ComPtr<ID3D12Device>& aDevice, size_t aSize) override;
    void CreateResourceViews();

    class DX12* dx12 = nullptr;
    UINT indexOffset = 0;
};
