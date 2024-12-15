#include "pch.h"
#include "ResourceLoader.h"
#include "IResource.h"
#include "DXHelper.h"

ResourceLoader::ResourceLoader(DX12& aDx12) :
	dx12(aDx12)
{
}

void ResourceLoader::Update()
{
	if (resources.size() == 0)
		return;

	PrepareLoad();

	for (IResource* resource : resources)
	{
		resource->LoadToGPU(dx12);
	}

	ExitLoad();

	for (size_t i = 0; i < resources.size(); i++)
	{
		IResource* resource = resources[i];
		resource->OnGPULoadComplete();
		if (activeCallbacks.contains(i))
			std::any_cast<OnLoadedCallback>(activeCallbacks[i])(resource);
	}

	for (IResource* resource : resources)
	{
		resource->OnGPULoadComplete();
	}

	resources.clear();
}

void ResourceLoader::PrepareLoad()
{
	ThrowIfFailed(dx12.myCommandAllocator[dx12.myFrameIndex]->Reset());
	ThrowIfFailed(dx12.myCommandList->Reset(dx12.myCommandAllocator[dx12.myFrameIndex].Get(), dx12.myPipelineState.Get()));
}

void ResourceLoader::ExitLoad()
{
	ThrowIfFailed(dx12.myCommandList->Close());

	// Execute the command list
	ID3D12CommandList* commandLists[] = { dx12.myCommandList.Get() };
	dx12.myCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	dx12.WaitForGPU();
}
