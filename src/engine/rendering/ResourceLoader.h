#pragma once

#include <vector>
#include <functional>

#include "buffers/IndexBuffer.h"
#include "buffers/VertexBuffer.h"

class IFunction {
public:
	virtual ~IFunction() = default;
	virtual void execute(class IResource*) const = 0;
};

template <typename Ret>
class OnLoadedCallback : public IFunction {
public:
	typedef std::function<void(Ret*)> Callback;

	OnLoadedCallback(Callback func) : func_(func) {}

	void execute(class IResource* aOwner) const override {
		func_((Ret*)aOwner);
	}

private:
	std::function<void(Ret*)> func_;
};

struct PendingResource
{
	class IResource* resource = nullptr;
	IFunction* callback = nullptr;
};

struct ModelData
{
	size_t vertexBaseLocation = 0;
	size_t indexBaseLocation = 0;
};

struct ResourceBuffers
{
	VertexBuffer vertexBuffer;
	IndexBuffer indexBuffer;

	ResourceBuffers() :
	vertexBuffer(D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
	indexBuffer(D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_INDEX_BUFFER)
	{
		
	}
};

class ResourceLoader
{
public:
	ResourceLoader(class DX12* aDx12);
	~ResourceLoader();

	void BeginFrame();
	void OnRender();

	// TODO: Add proper mesh and texture loaders at integrate this function
	template<typename T = class IResource>
	void LoadResource(T* aResource, OnLoadedCallback<T>::Callback aOnLoadedCallback = nullptr)
	{ 
		resourcesToLoad.push_back(PendingResource{
			aResource,
			aOnLoadedCallback ? new OnLoadedCallback<T>(aOnLoadedCallback) : nullptr
		});
	}

	ResourceBuffers& GetBuffers() { return buffers; }
private:
	void PrepareLoad();
	void ExitLoad();

	std::vector<ModelData> modelMap;
	std::vector<PendingResource> resourcesToLoad;
	std::vector<class IResource*> activeResources;
	UINT resourceCounter = 0;
	ResourceBuffers buffers{};
	// TODO: Custom heap allocator

	DX12* dx12;
};

