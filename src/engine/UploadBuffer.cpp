#include "pch.h"
#include "UploadBuffer.h"

HRESULT UploadBuffer::Initialize(ComPtr<ID3D12Device> aDevice, size_t aSize)
{
    HRESULT hr = aDevice->CreateCommittedResource(
        &keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)),
        D3D12_HEAP_FLAG_NONE,
        &keep(CD3DX12_RESOURCE_DESC::Buffer(aSize)),
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&m_spUploadBuffer));

    if (SUCCEEDED(hr))
    {
        void* pData;
        //
        // No CPU reads will be done from the resource.
        //
        CD3DX12_RANGE readRange(0, 0);
        m_spUploadBuffer->Map(0, &readRange, &pData);
        m_pDataCur = m_pDataBegin = reinterpret_cast<UINT8*>(pData);
        m_pDataEnd = m_pDataBegin + aSize;
    }
    return hr;
}

HRESULT UploadBuffer::SetDataToUploadBuffer(
    const void* pData,
    UINT bytesPerData,
    UINT dataCount,
    UINT alignment,
    UINT& byteOffset
)
{
    SIZE_T byteSize = bytesPerData * dataCount;
    HRESULT hr = SuballocateFromBuffer(byteSize, alignment);
    if (SUCCEEDED(hr))
    {
        byteOffset = UINT(m_pDataCur - m_pDataBegin);
        memcpy(m_pDataCur, pData, byteSize);
        m_pDataCur += byteSize;
    }
    return hr;
}
