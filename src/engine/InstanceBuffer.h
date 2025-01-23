#pragma once
#include "DXHelper.h"
#include <queue>
#include "SceneBufferTypes.h"

class InstanceBuffer
{
public:
    static constexpr size_t DEFAULT_INSTANCE_BUFFER_SIZE = 8192;

    InstanceBuffer() = default;
    ~InstanceBuffer();

    void Create(class DX12* aDx12, size_t aSize = DEFAULT_INSTANCE_BUFFER_SIZE, bool aShouldCopy = true);
    void Update(class DX12& aDx12, const std::vector<InstanceData>& aInstances, size_t aOffset = 0);


    void Initialize(class DX12* aDx12);
    void OnEndFrame(class DX12* aDx12);


    ComPtr<ID3D12Resource> instanceBuffer       = nullptr;
    ComPtr<ID3D12Resource> instanceUploadHeap   = nullptr;

    class InstanceData* cpuInstanceData = nullptr;

    D3D12_VERTEX_BUFFER_VIEW instanceBufferView;
private:

    std::queue<size_t> availableIndices{};
    size_t cpuSize = 0;
    size_t heapSize = 0;
    size_t uploadHeapSize = 0;
    size_t uploadOffset = 0;
};

