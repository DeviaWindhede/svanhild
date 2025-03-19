#pragma once
#include "DXHelper.h"
#include <SceneBufferTypes.h>
#include "StagingDescriptorHeap.h"
#include "FrameBuffer.h"
#include <InstanceBuffer.h>

#include "MeshRenderer.h"

using Microsoft::WRL::ComPtr;

class DX12
{
public:
    // TODO: MOVE TO MATH
    inline static size_t NextPowerOfTwo(size_t aValue)
    {
        if (aValue <= 1)
            return 1;
        unsigned long index;
        _BitScanReverse64(&index, aValue - 1);
        return 1ull << (index + 1);
    }
    
    DX12(UINT aWidth, UINT aHeight, bool aUseWarpDevice);
    ~DX12();

    void Cleanup();
	void LoadPipeline();
	void PrepareRender();
	void ExecuteRender();
    void EndRender();
	void WaitForGPU();
    void WaitForNextFrame();
    void MoveToNextFrame();

    void GetHardwareAdapter(
        _In_ IDXGIFactory1* pFactory,
        _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter,
        bool requestHighPerformanceAdapter = false);

    static constexpr UINT FrameCount = 2;
    static constexpr UINT MAX_CBV_COUNT = 1;
    static constexpr UINT MAX_SRV_COUNT = 4096;
    static constexpr UINT MAX_UAV_COUNT = 0; // TODO: add UAV support
    static constexpr UINT MAX_BOUND_SRV_COUNT = 64;
    static constexpr UINT INSTANCE_BUFFER_SIZE = 4096;

    static constexpr UINT CBV_SIZE = 0; // TODO: Change to 2 here
    static constexpr UINT SRV_SIZE = 2;
    static constexpr UINT UAV_SIZE = 1;
    static constexpr UINT CBV_SRV_UAV_SIZE = CBV_SIZE + SRV_SIZE + UAV_SIZE; //* FrameCount; // 2srv + 1uav

    // Pipeline objects
    CD3DX12_VIEWPORT myViewport;
    CD3DX12_RECT myScissorRect;
    ComPtr<IDXGISwapChain3> mySwapChain;
    ComPtr<ID3D12Device> myDevice;
    ComPtr<ID3D12Resource> myRenderTargets[FrameCount];
    ComPtr<ID3D12Resource> myDepthBuffer;
    ComPtr<ID3D12CommandAllocator> myCommandAllocator[FrameCount];
    ComPtr<ID3D12CommandAllocator> myComputeCommandAllocator[FrameCount];
    ComPtr<ID3D12CommandAllocator> myBundleAllocator;
    ComPtr<ID3D12CommandQueue> myCommandQueue;
    ComPtr<ID3D12CommandQueue> myComputeCommandQueue;
    ComPtr<ID3D12CommandSignature> myCommandSignature;
    ComPtr<ID3D12RootSignature> myRootSignature;
    ComPtr<ID3D12RootSignature> myComputeRootSignature;
    ComPtr<ID3D12DescriptorHeap> myRtvHeap;
    DescriptorHeap myComputeCbvSrvUavHeap;
    DescriptorHeap mySrvHeap;
    StagingDescriptorHeap mySrvStagingHeap;
    ComPtr<ID3D12DescriptorHeap> myDsvHeap;
    ComPtr<ID3D12GraphicsCommandList> myCommandList;
    ComPtr<ID3D12GraphicsCommandList> myComputeCommandList;
    ComPtr<ID3D12GraphicsCommandList> myBundle;

    // GpuResources resources;
    InstanceBuffer instanceBuffer;
    FrameBuffer frameBuffer;
    HANDLE mySwapChainWaitableObject = nullptr;
    UINT myRtvDescriptorSize;
    size_t currentPSO = SIZE_T_MAX;
    size_t currentComputePSO = SIZE_T_MAX;

    ComPtr<ID3D12Resource> myProcessedCommandBuffers[FrameCount];
    ComPtr<ID3D12Resource> myProcessedCommandBufferCounterReset;

    MeshRenderer meshRenderer;
    // Synchronization objects
    UINT cbvSrvUavDescriptorSize = 0;
    UINT myFrameIndex;
    HANDLE myFenceEvent;
    ComPtr<ID3D12Fence> myFence;
    ComPtr<ID3D12Fence> myComputeFence;
    UINT64 myFenceValues[FrameCount];
    bool swapChainOccluded = false;
    bool useVSync = false;
private:
    bool useWarpDevice;
};

