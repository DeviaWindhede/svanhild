#pragma once
#include "DXHelper.h"

using Microsoft::WRL::ComPtr;

template<typename T, size_t Size = 1>
class CbvResource
{
public:
	virtual void Init(
		ID3D12Device* aDevice,
		D3D12_HEAP_TYPE aHeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD,
		const T* aDefaultData = nullptr,
		size_t aDataSize = sizeof(T),
		bool aShouldUnmap = false
	);

	void Map(const CD3DX12_RANGE& = CD3DX12_RANGE(0, 0));
	void Unmap(const D3D12_RANGE* aWrittenRange = nullptr);

    T data[Size]{};
    ComPtr<ID3D12Resource> resource;
    UINT8* cbvDataBegin = 0;
};

template<typename T, size_t Size>
inline void CbvResource<T, Size>::Init(
	ID3D12Device* aDevice,
	D3D12_HEAP_TYPE aHeapType,
	const T* aDefaultData,
	size_t aDataSize,
	bool aShouldUnmap
)
{
	const UINT constantBufferSize = sizeof(T) * Size;    // CB size is required to be 256-byte aligned.

	ThrowIfFailed(aDevice->CreateCommittedResource(
		&keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)),
		D3D12_HEAP_FLAG_NONE,
		&keep(CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&resource)));

	NAME_D3D12_OBJECT(resource);

	if (aDefaultData)
	{
		memcpy(&data[0], aDefaultData, aDataSize);
	}

	Map();
	memcpy(cbvDataBegin, &data[0], aDataSize);

	if (aShouldUnmap)
	{
		Unmap();
	}
}

template<typename T, size_t Size>
inline void CbvResource<T, Size>::Map(const CD3DX12_RANGE& aReadRange)
{
	ThrowIfFailed(resource->Map(0, &aReadRange, reinterpret_cast<void**>(&cbvDataBegin)));
}

template<typename T, size_t Size>
inline void CbvResource<T, Size>::Unmap(const D3D12_RANGE* aWrittenRange)
{
	resource->Unmap(0, aWrittenRange);
}
