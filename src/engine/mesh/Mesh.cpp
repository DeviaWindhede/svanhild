#include "pch.h"
#include "Mesh.h"
#include <vector>

#include "rendering/ResourceLoader.h"
#include "rendering/d3dx/d3d12/DX12.h"

Mesh::~Mesh()
{
	Internal_UnloadCPU();
}

void Mesh::Internal_UnloadCPU()
{
	delete[] vertices;
	vertices = nullptr;
	delete[] indices;
	indices = nullptr;
}

void Mesh::PerformResourceBarrier(
	ComPtr<ID3D12GraphicsCommandList>& aCommandList,
	D3D12_RESOURCE_STATES aPreviousState,
	D3D12_RESOURCE_STATES aNewState
) const
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = resource.Get();
	barrier.Transition.StateBefore = aPreviousState; // This is the default state for most resources
	barrier.Transition.StateAfter = aNewState;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	aCommandList->ResourceBarrier(1, &barrier);
}

void Mesh::LoadToGPU(class DX12* aDx12, struct ResourceBuffers* aBuffers)
{
	verticesIndex = aBuffers->vertexBuffer.AddItem(aDx12->myDevice, vertices, vertexCount);
	indicesIndex = aBuffers->indexBuffer.AddItem(aDx12->myDevice, indices, indexCount);

	// TODO: Update views on removal
	// TODO: fix view buffer location to account for index
	
	// vertex buffer view
	{
		vbv = {};
		vbv.SizeInBytes = static_cast<UINT>(sizeof(Vertex) * VertexCount());
		vbv.BufferLocation = aBuffers->vertexBuffer.resource->GetGPUVirtualAddress() + vbv.SizeInBytes;
		vbv.StrideInBytes = sizeof(Vertex);
	}
	
	// index buffer view
	{
		ibv = {};
		ibv.SizeInBytes = static_cast<UINT>(sizeof(UINT16) * IndexCount());
		vbv.BufferLocation = aBuffers->indexBuffer.resource->GetGPUVirtualAddress() + vbv.SizeInBytes;
		ibv.Format = DXGI_FORMAT_R16_UINT; // DXGI_FORMAT_R32_UINT
	}

	IResource::LoadToGPU(aDx12, aBuffers);
	
	
	// size_t vertexSize = sizeof(Vertex) * vertexCount;
	// size_t indexSize = sizeof(UINT16) * indexCount;
	// size_t bufferSize = vertexSize + indexSize;
	//
	// aDx12->myDevice->CreateCommittedResource(
	// 	&keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
	// 	D3D12_HEAP_FLAG_NONE,
	// 	&keep(CD3DX12_RESOURCE_DESC::Buffer(bufferSize)),
	// 	D3D12_RESOURCE_STATE_COMMON,
	// 	nullptr,
	// 	IID_PPV_ARGS(&resource)
	// );
	// NAME_D3D12_OBJECT(resource);
	//
	// {
	// 	D3D12_HEAP_PROPERTIES heapProperties = {};
	// 	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	//
	// 	D3D12_RESOURCE_DESC resourceDesc = {};
	// 	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	// 	resourceDesc.Width = bufferSize;
	// 	resourceDesc.Height = 1;
	// 	resourceDesc.DepthOrArraySize = 1;
	// 	resourceDesc.MipLevels = 1;
	// 	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	// 	resourceDesc.SampleDesc.Count = 1;
	// 	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	//
	//
	// 	HRESULT hr = aDx12->myDevice->CreateCommittedResource(
	// 		&heapProperties,
	// 		D3D12_HEAP_FLAG_NONE,
	// 		&resourceDesc,
	// 		D3D12_RESOURCE_STATE_GENERIC_READ,  // The upload heap state is generic read
	// 		nullptr,
	// 		IID_PPV_ARGS(&uploadHeap)
	// 	);
	//
	// 	if (!uploadHeap)
	// 		throw  std::runtime_error("Upload heap creation failed.");
	//
	// 	NAME_D3D12_OBJECT(uploadHeap);
	//
	// 	void* mappedData = nullptr;
	// 	ThrowIfFailed(uploadHeap->Map(0, nullptr, &mappedData));
	//
	// 	memcpy(mappedData, Vertices(), vertexSize);
	// 	memcpy(static_cast<char*>(mappedData) + vertexSize, Indices(), indexSize);
	//
	// 	uploadHeap->Unmap(0, nullptr);
	// }
	//
	// PerformResourceBarrier(aDx12->myCommandList, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	//
	// aDx12->myCommandList->CopyBufferRegion(resource.Get(), 0, uploadHeap.Get(), 0, bufferSize);
	//
	// PerformResourceBarrier(aDx12->myCommandList, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	//
}

void Mesh::OnGPULoadComplete(class DX12* aDx12, struct ResourceBuffers* aBuffers)
{
	IResource::OnGPULoadComplete(aDx12, aBuffers);
//#ifndef _DEBUG
	UnloadCPU(aDx12, aBuffers);
//#endif
}

void Mesh::UnloadCPU(class DX12* aDx12, struct ResourceBuffers* aBuffers)
{
	Internal_UnloadCPU();
}

void Mesh::LoadMeshData(const std::vector<Vertex>& aVertices, const std::vector<UINT16>& aIndices)
{
	//if (data != nullptr)
	if (vertices != nullptr || indices != nullptr)
	{
		Internal_UnloadCPU();
	}

	vertexCount = aVertices.size();
	indexCount = static_cast<UINT16>(aIndices.size());

	vertices = new Vertex[vertexCount];
	indices = new UINT16[indexCount];
	memcpy(vertices, aVertices.data(), vertexCount * sizeof(Vertex));
	memcpy(indices, aIndices.data(), indexCount * sizeof(UINT16));
	
	for (UINT i = 0; i < aVertices.size(); i++)
	{
		if (aVertices[i].position.x > bounds.max.x)
			bounds.max.x = aVertices[i].position.x;
		if (aVertices[i].position.y > bounds.max.y)
			bounds.max.y = aVertices[i].position.y;
		if (aVertices[i].position.z > bounds.max.z)
			bounds.max.z = aVertices[i].position.z;
		if (aVertices[i].position.x < bounds.min.x)
			bounds.min.x = aVertices[i].position.x;
		if (aVertices[i].position.y < bounds.min.y)
			bounds.min.y = aVertices[i].position.y;
		if (aVertices[i].position.z < bounds.min.z)
			bounds.min.z = aVertices[i].position.z;
	}
	//size_t totalSize = sizeof(Vertex) * vertexCount + sizeof(UINT) * indexCount;
	//data = new unsigned char[totalSize]; // TODO: Custom allocator for all asset content

	//memcpy(data, aVertices.data(), vertexCount * sizeof(Vertex));
	//memcpy((data + vertexCount * sizeof(Vertex)), aIndices.data(), indexCount * sizeof(UINT16));

}
