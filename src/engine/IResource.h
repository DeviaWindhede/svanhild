#pragma once

using Microsoft::WRL::ComPtr;

class IResource
{
	friend class ResourceLoader;
public:
	virtual void LoadToGPU(class DX12& aDx12) { assert(!isGPUInitialized && "Trying to GPU load multiple times!"); }
	
	virtual void OnGPULoadComplete()
	{ 
		isGPUInitialized = true;
		uploadHeap = nullptr;
	}

	virtual void UnloadGPU(class DX12& aDx12)
	{
		resource = nullptr;
		uploadHeap = nullptr;
	}

	virtual void UnloadCPU() { __noop; }

	UINT SrvIndex() const { return srvIndex; };
	bool GPUInitialized() const { return isGPUInitialized; }
protected:
	ComPtr<ID3D12Resource> resource;
	ComPtr<ID3D12Resource> uploadHeap;
	UINT srvIndex = 0;
	bool isGPUInitialized = false;
};