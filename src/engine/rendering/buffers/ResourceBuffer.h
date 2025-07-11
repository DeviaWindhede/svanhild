﻿#pragma once
#include "rendering/d3dx/DXHelper.h"

template<typename T>
class ResourceBuffer
{
public:
    ResourceBuffer(const ResourceBuffer& aOther) = delete;
    ResourceBuffer(ResourceBuffer&& aOther) noexcept = delete;
    ResourceBuffer& operator=(const ResourceBuffer& aOther) = delete;
    ResourceBuffer& operator=(ResourceBuffer&& aOther) noexcept = delete;

    explicit ResourceBuffer(D3D12_RESOURCE_STATES aResourceState);
    virtual ~ResourceBuffer();

    virtual void Cleanup();

    virtual void Create(ComPtr<ID3D12Device>& aDevice, size_t aSize);
    virtual void Update(ComPtr<ID3D12GraphicsCommandList>& aCommandList);
    virtual size_t AddItem(ComPtr<ID3D12Device>& aDevice, T* aData, size_t aSize = 1);
    virtual void RemoveItem(ComPtr<ID3D12Device>& aDevice, size_t aIndex);
    // virtual void ModifyItem(ComPtr<ID3D12Device>& aDevice, size_t aIndex, T* aData, size_t aSize = 1); // TODO

    const T* GetData(size_t aIndex, size_t& outCount) const;
    size_t GetCount() const { return indexOffsets.size(); }
    
    ComPtr<ID3D12Resource> resource = nullptr;
    ComPtr<ID3D12Resource> uploadHeap = nullptr;
    
    std::vector<T> cpuData{};
    
    size_t size = 0;
    size_t gpuSize = 0;
    
    D3D12_RESOURCE_STATES defaultResourceState;
    bool dirty = false;
    bool wasDirty = false;
protected:
    virtual void Create(ComPtr<ID3D12Device>& aDevice, size_t aSize, D3D12_RESOURCE_DESC aDesc, LPCWSTR aName);
    
    std::vector<size_t> indexOffsets{};
};

template <typename T>
ResourceBuffer<T>::ResourceBuffer(D3D12_RESOURCE_STATES aResourceState) : defaultResourceState(aResourceState)
{
}

template <typename T>
ResourceBuffer<T>::~ResourceBuffer()
{
    Cleanup();
}

template <typename T>
void ResourceBuffer<T>::Cleanup()
{
    cpuData.clear();
    size = 0;
    dirty = false;
    resource = nullptr;
    uploadHeap = nullptr;
}

template <typename T>
void ResourceBuffer<T>::Create(ComPtr<ID3D12Device>& aDevice, size_t aSize)
{
    Create(aDevice, aSize, CD3DX12_RESOURCE_DESC::Buffer(aSize * sizeof(T)), L"SharedBuffer_Resource");
}

template <typename T>
void ResourceBuffer<T>::Create(ComPtr<ID3D12Device>& aDevice, size_t aSize, D3D12_RESOURCE_DESC aDesc, LPCWSTR aName)
{
    gpuSize = 0;
    cpuData.resize(aSize);

    if (aSize == 0)
    {
        resource = nullptr;
        uploadHeap = nullptr;
        return;
    }
    
    dirty = true;

    D3D12_HEAP_PROPERTIES heapProps = { D3D12_HEAP_TYPE_DEFAULT };
    
    // buffer
    {
        aDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        ThrowIfFailed(aDevice->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &aDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&resource)
        ));
        resource->SetName(aName);
    }

    // upload
    {
        heapProps = { .Type= D3D12_HEAP_TYPE_UPLOAD };
        aDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        ThrowIfFailed(aDevice->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &aDesc, // Size to be optimized
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&uploadHeap)
        ));
        NAME_D3D12_OBJECT(uploadHeap);
    }

    // TODO: Copy default objects to new buffer during creation
}

template <typename T>
void ResourceBuffer<T>::Update(ComPtr<ID3D12GraphicsCommandList>& aCommandList)
{
    wasDirty = false;
    if (!dirty)
        return;
    
    size_t begin = gpuSize > size ? 0 : gpuSize;
    size_t copySize = size - gpuSize;
    // size_t begin = 0;
    // size_t copySize = size;
    
    void* data = nullptr;
    ThrowIfFailed(uploadHeap->Map(0, nullptr, &data));
    memcpy(data, cpuData.data(), sizeof(T) * copySize);
    uploadHeap->Unmap(0, nullptr);

    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            resource.Get(),
            defaultResourceState,
            D3D12_RESOURCE_STATE_COPY_DEST
        );
        aCommandList->ResourceBarrier(1, &barrier);
    }

    aCommandList->CopyBufferRegion(
        resource.Get(),
        begin * sizeof(T),
        uploadHeap.Get(),
        0,
        sizeof(T) * copySize
    );

    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            resource.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            defaultResourceState
        );
        aCommandList->ResourceBarrier(1, &barrier);
    }

    gpuSize = size;
    dirty = false;
    wasDirty = true;
}

template <typename T>
size_t ResourceBuffer<T>::AddItem(ComPtr<ID3D12Device>& aDevice, T* aData, size_t aSize)
{
    if (cpuData.capacity() < cpuData.size() + aSize)
        Create(aDevice, NextPowerOfTwo(cpuData.size() + aSize));

    char* dest = reinterpret_cast<char*>(cpuData.data()) + sizeof(T) * size;
    memcpy(dest, reinterpret_cast<char*>(aData), sizeof(T) * aSize);
    
    size_t index = size;
    size += aSize;
    dirty = true;
    indexOffsets.push_back(aSize);

    return index;
}

template <typename T>
void ResourceBuffer<T>::RemoveItem(ComPtr<ID3D12Device>& aDevice, size_t aIndex)
{
    assert(aIndex < cpuData.size());
    if (size - 1 <= cpuData.capacity() / 2)
        Create(aDevice, cpuData.capacity() / 2);

    cpuData.erase(cpuData.begin() + aIndex);
    size--;
    dirty = true;
    
    size_t valueToRemove = indexOffsets[aIndex];
    for (size_t i = aIndex; i < indexOffsets.size() - 1; i++)
    {
        indexOffsets[i] = indexOffsets[i + 1] - valueToRemove;
    }
    indexOffsets.pop_back();
}

template <typename T>
const T* ResourceBuffer<T>::GetData(size_t aIndex, size_t& outCount) const
{
    assert(indexOffsets.size() > aIndex);
    
    outCount = (aIndex == indexOffsets.size() - 1 ? cpuData.size() : indexOffsets[aIndex + 1]) - indexOffsets[aIndex];
    return &cpuData.data()[aIndex];
}
