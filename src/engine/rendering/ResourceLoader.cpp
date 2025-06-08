#include "pch.h"
#include "ResourceLoader.h"
#include "ShaderCompiler.h"
#include "mesh/IResource.h"

ResourceLoader::ResourceLoader(DX12* aDx12) :
	dx12(aDx12), buffers()
{
}

ResourceLoader::~ResourceLoader()
{
	for (IResource* resource : activeResources)
	{
		resource->UnloadGPU(dx12, &buffers);
		resource->UnloadCPU(dx12, &buffers);
		delete resource;
	}
	activeResources.clear();
}

void ResourceLoader::BeginFrame()
{
	if (resourcesToLoad.size() == 0)
		return;

	PrepareLoad();

	for (PendingResource& resource : resourcesToLoad)
	{
		resource.resource->LoadToGPU(dx12, &buffers);
	}

	ExitLoad();
	
	for (PendingResource& pending : resourcesToLoad)
	{
		IResource* resource = pending.resource;
		resource->resourceIndex = resourceCounter++;
		resource->OnGPULoadComplete(dx12, &buffers);

		activeResources.push_back(resource);

		if (pending.callback)
		{
			pending.callback->execute(resource);
			delete pending.callback;
			pending.callback = nullptr;
		}
			//pending.callback(resource);
	}

	resourcesToLoad.clear();
}

void ResourceLoader::OnRender()
{
	buffers.vertexBuffer.Update(dx12->myCommandList);
	buffers.indexBuffer.Update(dx12->myCommandList);
}

void ResourceLoader::PrepareLoad()
{
	ThrowIfFailed(dx12->myCommandAllocator[dx12->myFrameIndex]->Reset());
	ThrowIfFailed(dx12->myCommandList->Reset(dx12->myCommandAllocator[dx12->myFrameIndex].Get(), ShaderCompiler::GetPSO(dx12->currentPSO).state.Get()));
}

void ResourceLoader::ExitLoad()
{
	ThrowIfFailed(dx12->myCommandList->Close());

	// Execute the command list
	ID3D12CommandList* commandLists[] = { dx12->myCommandList.Get() };
	dx12->myCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	dx12->WaitForGPU();
}
