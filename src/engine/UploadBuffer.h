#pragma once

#include "DXHelper.h"
#include <mesh/Vertex.h>

class UploadBuffer
{
public:
	ComPtr<ID3D12Resource> m_spUploadBuffer;
	UINT8* m_pDataBegin = nullptr;    // starting position of upload buffer
	UINT8* m_pDataCur = nullptr;      // current position of upload buffer
	UINT8* m_pDataEnd = nullptr;      // ending position of upload buffer

	HRESULT Initialize(ComPtr<ID3D12Device> aDevice, size_t aSize);

	HRESULT SetDataToUploadBuffer(
		const void* pData,
		UINT bytesPerData,
		UINT dataCount,
		UINT alignment,
		UINT& byteOffset
	);

	template<typename T>
	HRESULT SetConstants(
		T aConstants[],
		UINT aAlignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT,
		UINT aOffset = 0
	)
	{
		return SetDataToUploadBuffer(
			aConstants, sizeof(T), sizeof(aConstants) / sizeof(T),
			aAlignment,
			aOffset
		);
	}

	HRESULT SetVertices(Vertex aVertices[], UINT aOffset = 0)
	{
		return SetConstants<Vertex>(aVertices, sizeof(Vertex), aOffset);
	}

	HRESULT SetIndices(UINT aIndices[], UINT aOffset = 0)
	{
		return SetConstants<UINT>(aIndices, sizeof(UINT), aOffset);
	}

	UINT Align(UINT uLocation, UINT uAlign)
	{
		if ((0 == uAlign) || (uAlign & (uAlign - 1)))
		{
			throw "non-pow2 alignment";
		}

		return ((uLocation + (uAlign - 1)) & ~(uAlign - 1));
	}

	HRESULT SuballocateFromBuffer(SIZE_T uSize, UINT uAlign)
	{
		m_pDataCur = reinterpret_cast<UINT8*>(
			Align(reinterpret_cast<SIZE_T>(m_pDataCur), uAlign)
		);

		return (m_pDataCur + uSize > m_pDataEnd) ? E_INVALIDARG : S_OK;
	}
	//HRESULT Add()
};

