#pragma once

using Microsoft::WRL::ComPtr;

class IResource
{
	friend class ResourceLoader;
public:
	virtual void LoadToGPU(class DX12& aDx12)	= 0;
	virtual void OnGPULoadComplete() { isGPUInitialized = true; }
	virtual void UnloadGPU(class DX12& aDx12) { __noop; };
	UINT SrvIndex() const { return srvIndex; };
	bool GPUInitialized() const { return isGPUInitialized; }
protected:
	ComPtr<ID3D12Resource> uploadHeap;
	UINT srvIndex = 0;
	bool isGPUInitialized = false;
};