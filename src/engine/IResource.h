#pragma once

using Microsoft::WRL::ComPtr;

class IResource
{
	friend class ResourceLoader;
public:
	virtual ~IResource() = default;

	virtual void LoadToGPU(class DX12& aDx12) { assert(!isGPUInitialized && "Trying to GPU load multiple times!"); }
	
	virtual void OnGPULoadComplete(class DX12&)
	{
		isGPUInitialized = true;
		uploadHeap = nullptr;
	}

	virtual void UnloadGPU(class DX12& aDx12)
	{
		resource = nullptr;
		uploadHeap = nullptr;
	}

	virtual void UnloadCPU(class DX12& aDx12) { __noop; }

	UINT Index() const { return resourceIndex; }
	bool GPUInitialized() const { return isGPUInitialized; }
protected:
	ComPtr<ID3D12Resource> resource;
	ComPtr<ID3D12Resource> uploadHeap;
	UINT resourceIndex = 0;
	bool isGPUInitialized = false;
};