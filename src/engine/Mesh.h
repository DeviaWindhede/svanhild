#pragma once
#include "UploadBuffer.h"
#include "IResource.h"

class Mesh : public IResource
{
public:
	Mesh() = default;
	~Mesh();

	virtual void LoadToGPU(class DX12& aDx12) override;
	virtual void OnGPULoadComplete() override;
	void LoadMeshData(const std::vector<Vertex>& aVertices, const std::vector<UINT16>& aIndices);
	
	void ResetUploadBuffer();

	//FORCEINLINE Vertex* Vertices() const			{ return (Vertex*)data; }
	//FORCEINLINE UINT16*	Indices() const				{ return (UINT16*)(data + vertexCount * sizeof(Vertex)); }
	FORCEINLINE Vertex* Vertices() const			{ return vertices; }
	FORCEINLINE UINT16*	Indices() const				{ return indices; }
	FORCEINLINE size_t	VertexCount() const			{ return vertexCount; }
	FORCEINLINE size_t	IndexCount() const			{ return indexCount; }
	FORCEINLINE bool	InitializedBuffer() const	{ return initializedBuffers; }

	FORCEINLINE const D3D12_VERTEX_BUFFER_VIEW& VertexBufferView() const { return vbv; }
	FORCEINLINE const D3D12_INDEX_BUFFER_VIEW& IndexBufferView() const { return ibv; }

	//UploadBuffer buffer{};

	ComPtr<ID3D12Resource> resourceBuffer;
private:
	void PerformResourceBarrier(ComPtr<ID3D12GraphicsCommandList>& aCommandList, D3D12_RESOURCE_STATES aPrevious, D3D12_RESOURCE_STATES aNewState) const;

	void Reset();


	D3D12_VERTEX_BUFFER_VIEW vbv;
	D3D12_INDEX_BUFFER_VIEW ibv;

	//unsigned char* data	= 0;
	Vertex* vertices = 0;
	UINT16* indices = 0;
	size_t vertexCount	= 0;
	size_t indexCount	= 0;
	UINT verticesOffset = 0;
	UINT indicesOffset	= 0;
	bool initializedBuffers	= false;
};
