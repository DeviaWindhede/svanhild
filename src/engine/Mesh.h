#pragma once
#include "UploadBuffer.h"
#include "IResource.h"

class Mesh : public IResource
{
public:
	Mesh() = default;
	~Mesh();

	void LoadMeshData(const std::vector<Vertex>& aVertices, const std::vector<UINT16>& aIndices);
	
	virtual void LoadToGPU(class DX12* aDx12, struct ResourceBuffers* aBuffers) override;
	virtual void OnGPULoadComplete(class DX12* aDx12, struct ResourceBuffers* aBuffers) override;
	virtual void UnloadCPU(class DX12* aDx12, struct ResourceBuffers* aBuffers) override;
	
	//Vertex* Vertices() const			{ return (Vertex*)data; }
	//UINT16*	Indices() const				{ return (UINT16*)(data + vertexCount * sizeof(Vertex)); }
	Vertex* Vertices() const			{ return vertices; }
	UINT16*	Indices() const				{ return indices; }
	size_t	VertexCount() const			{ return vertexCount; }
	size_t	IndexCount() const			{ return indexCount; }

	const D3D12_VERTEX_BUFFER_VIEW& VertexBufferView() const { return vbv; }
	const D3D12_INDEX_BUFFER_VIEW& IndexBufferView() const { return ibv; }
private:
	void Internal_UnloadCPU();
	void PerformResourceBarrier(ComPtr<ID3D12GraphicsCommandList>& aCommandList, D3D12_RESOURCE_STATES aPrevious, D3D12_RESOURCE_STATES aNewState) const;

	D3D12_VERTEX_BUFFER_VIEW vbv;
	D3D12_INDEX_BUFFER_VIEW ibv;

	//unsigned char* data	= 0;
	Vertex* vertices = 0;
	UINT16* indices = 0;
	size_t vertexCount	= 0;
	size_t indexCount	= 0;
	UINT verticesOffset = 0;
	UINT indicesOffset	= 0;
};
