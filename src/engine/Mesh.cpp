#include "pch.h"
#include "Mesh.h"
#include "DX12.h"
#include <vector>

Mesh::~Mesh()
{
	Reset();
}

void Mesh::ResetUploadBuffer()
{
	//buffer.Reset();
	//buffer = {};
}

void Mesh::PerformResourceBarrier(
	ComPtr<ID3D12GraphicsCommandList>& aCommandList,
	D3D12_RESOURCE_STATES aPreviousState,
	D3D12_RESOURCE_STATES aNewState
) const
{
	assert(initializedBuffers);

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = resourceBuffer.Get();
	barrier.Transition.StateBefore = aPreviousState; // This is the default state for most resources
	barrier.Transition.StateAfter = aNewState;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	aCommandList->ResourceBarrier(1, &barrier);
}

void Mesh::Reset()
{
	ResetUploadBuffer();
	delete[] vertices;
	vertices = nullptr;
	delete[] indices;
	indices = nullptr;
	//delete[] data;
	//data = nullptr;
}

void Mesh::LoadToGPU(class DX12& aDx12)
{
	assert(!initializedBuffers && "Trying to initialize buffer multiple times!");

	size_t vertexSize = sizeof(Vertex) * vertexCount;
	size_t indexSize = sizeof(UINT16) * indexCount;
	size_t bufferSize = vertexSize + indexSize;

	aDx12.myDevice->CreateCommittedResource(
		&keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
		D3D12_HEAP_FLAG_NONE,
		&keep(CD3DX12_RESOURCE_DESC::Buffer(bufferSize)),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&resourceBuffer)
	);

	{
		D3D12_HEAP_PROPERTIES heapProperties = {};
		heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = bufferSize;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;


		HRESULT hr = aDx12.myDevice->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,  // The upload heap state is generic read
			nullptr,
			IID_PPV_ARGS(&uploadHeap)
		);

		if (!uploadHeap)
			throw  std::runtime_error("Upload heap creation failed.");


		void* mappedData = nullptr;
		ThrowIfFailed(uploadHeap->Map(0, nullptr, &mappedData));

		memcpy(mappedData, Vertices(), vertexSize);
		memcpy(static_cast<char*>(mappedData) + vertexSize, Indices(), indexSize);

		uploadHeap->Unmap(0, nullptr);
	}
	initializedBuffers = true;

	PerformResourceBarrier(aDx12.myCommandList, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

	aDx12.myCommandList->CopyBufferRegion(resourceBuffer.Get(), 0, uploadHeap.Get(), 0, bufferSize);

	PerformResourceBarrier(aDx12.myCommandList, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	// vertex buffer view
	{
		vbv = {};
		vbv.BufferLocation = resourceBuffer->GetGPUVirtualAddress();
		vbv.StrideInBytes = sizeof(Vertex);
		vbv.SizeInBytes = static_cast<UINT>(sizeof(Vertex) * VertexCount());
	}

	// index buffer view
	{
		ibv = {};
		ibv.BufferLocation = resourceBuffer->GetGPUVirtualAddress() + vbv.SizeInBytes;
		ibv.SizeInBytes = static_cast<UINT>(sizeof(UINT16) * IndexCount());
		ibv.Format = DXGI_FORMAT_R16_UINT; // DXGI_FORMAT_R32_UINT
	}
}

void Mesh::OnGPULoadComplete()
{
	IResource::OnGPULoadComplete();
	uploadHeap = nullptr;
}

void Mesh::LoadMeshData(const std::vector<Vertex>& aVertices, const std::vector<UINT16>& aIndices)
{
	//if (data != nullptr)
	if (vertices != nullptr || indices != nullptr)
	{
		Reset();
	}

	vertexCount = aVertices.size();
	indexCount = static_cast<UINT16>(aIndices.size());

	vertices = new Vertex[vertexCount];
	indices = new UINT16[indexCount];
	memcpy(vertices, aVertices.data(), vertexCount * sizeof(Vertex));
	memcpy(indices, aIndices.data(), indexCount * sizeof(UINT16));

	//size_t totalSize = sizeof(Vertex) * vertexCount + sizeof(UINT) * indexCount;
	//data = new unsigned char[totalSize]; // TODO: Custom allocator for all asset content

	//memcpy(data, aVertices.data(), vertexCount * sizeof(Vertex));
	//memcpy((data + vertexCount * sizeof(Vertex)), aIndices.data(), indexCount * sizeof(UINT16));

}
