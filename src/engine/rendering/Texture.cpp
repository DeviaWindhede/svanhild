#include "pch.h"
#include "Texture.h"

#include "d3dx/DXHelper.h"
#include "d3dx/d3d12/DX12.h"

Texture::~Texture()
{

}

void Texture::LoadToGPU(DX12* aDx12, ResourceBuffers*)
{
	// Create the texture.
	{
		// Describe and create a Texture2D.
		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.MipLevels = 1;
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		textureDesc.Width = TextureWidth;
		textureDesc.Height = TextureHeight;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		{
			auto properies = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
			ThrowIfFailed(aDx12->myDevice->CreateCommittedResource(
				&properies,
				D3D12_HEAP_FLAG_NONE,
				&textureDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&resource)));
		}

		const UINT64 uploadBufferSize = GetRequiredIntermediateSize(resource.Get(), 0, 1);

		{
			auto properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
			auto desc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
			// Create the GPU upload buffer.
			ThrowIfFailed(aDx12->myDevice->CreateCommittedResource(
				&properties,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&uploadHeap)));
		}

		// Copy data to the intermediate upload heap and then schedule a copy 
		// from the upload heap to the Texture2D.
		std::vector<UINT8> texture = GenerateTextureData();

		{
			D3D12_SUBRESOURCE_DATA textureData = {};
			textureData.pData = &texture[0];
			textureData.RowPitch = TextureWidth * TexturePixelSize;
			textureData.SlicePitch = textureData.RowPitch * TextureHeight;

			UpdateSubresources(aDx12->myCommandList.Get(), resource.Get(), uploadHeap.Get(), 0, 0, 1, &textureData);
			auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				resource.Get(),
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			);
			aDx12->myCommandList->ResourceBarrier(1, &barrier);
		}

		// Describe and create a SRV for the texture.
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = textureDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		
		// descriptorHandle = aDx12->myTextureHeap.GetNewHeapHandle();

		aDx12->myDevice->CreateShaderResourceView(resource.Get(), &srvDesc, aDx12->myGraphicsCbvSrvUavHeap.GetStaticCPUHandle(
			0, static_cast<UINT>(GraphicsHeapSpaces::Textures))
		);
	}
}

void Texture::OnGPULoadComplete(class DX12* aDx12, struct ResourceBuffers* aBuffers)
{
	IResource::OnGPULoadComplete(aDx12, aBuffers);
	uploadHeap = nullptr;
}

void Texture::UnloadCPU(DX12* aDx12, struct ResourceBuffers*)
{
	// TODO
	//aDx12->myTextureHeap.FreeHeapHandle(descriptorHandle);
}

bool Texture::Bind(UINT aSlot, DX12* aDx12)
{
	if (!GPUInitialized())
		return false;
/*
	D3D12_CPU_DESCRIPTOR_HANDLE srcHandle = descriptorHandle.cpuHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE destHandle = aDx12->mySrvHeap.cpuStart;

	UINT descriptorSize = aDx12->myDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	destHandle.ptr = aDx12->mySrvHeap.cpuStart.ptr + aSlot * descriptorSize;

	// TODO: Fix bindless textures so we dont need to use this whack code
	aDx12->myDevice->CopyDescriptorsSimple(1, destHandle, srcHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	*/
	return true;
}

std::vector<UINT8> Texture::GenerateTextureData()
{
	const UINT rowPitch = TextureWidth * TexturePixelSize;
	const UINT cellPitch = rowPitch >> 3;        // The width of a cell in the checkboard texture.
	const UINT cellHeight = TextureWidth >> 3;    // The height of a cell in the checkerboard texture.
	const UINT textureSize = rowPitch * TextureHeight;

	std::vector<UINT8> data(textureSize);
	UINT8* pData = &data[0];

	for (UINT n = 0; n < textureSize; n += TexturePixelSize)
	{
		UINT x = n % rowPitch;
		UINT y = n / rowPitch;
		UINT i = x / cellPitch;
		UINT j = y / cellHeight;

		if (i % 2 == j % 2)
		{
			pData[n] = 0x00;        // R
			pData[n + 1] = 0x00;    // G
			pData[n + 2] = 0x00;    // B
			pData[n + 3] = 0xff;    // A
		}
		else
		{
			pData[n] = 0xff;        // R
			pData[n + 1] = 0xff;    // G
			pData[n + 2] = 0xff;    // B
			pData[n + 3] = 0xff;    // A
		}
	}

	return data;
}
