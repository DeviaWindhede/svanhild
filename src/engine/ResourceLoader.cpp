#include "pch.h"
#include "ResourceLoader.h"
#include "DXHelper.h"

void ResourceLoader::LoadResources()
{
	if (resources.size() == 0)
		return;

	PrepareLoad();

	for (IResource* resource : resources)
	{
		resource->LoadToGPU();
	}

	ExitLoad();

	resources.clear();
}

void ResourceLoader::PrepareLoad()
{
	//ThrowIfFailed(myCommandAllocator[myFrameIndex]->Reset());
	//ThrowIfFailed(myCommandList->Reset(myCommandAllocator[myFrameIndex].Get(), myPipelineState.Get()));
}

void ResourceLoader::ExitLoad()
{
//	ThrowIfFailed(myCommandList->Close());
//
//	// Execute the command list
//	ID3D12CommandList* commandLists[] = { myCommandList.Get() };
//	myCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
//
//	WaitForGpu();
}
