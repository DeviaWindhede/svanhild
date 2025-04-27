#pragma once
#include "DXHelper.h"
#include <SceneBufferTypes.h>
#include "StagingDescriptorHeap.h"
#include "FrameBuffer.h"
#include <InstanceBuffer.h>

#include "BindlessDescriptorHeap.h"
#include "MeshRenderer.h"

using Microsoft::WRL::ComPtr;

enum class SrvOffsets
{
    InstanceBuffer,
    InstanceCount,
    Count
};

enum class ComputeSrvStaticOffsets
{
    InstanceBuffer,
    InstanceCount,
    Count
};

enum class ComputeUavDynamicOffsets
{
    CommandOutput,
    Count
};

enum class GraphicsRootParameters
{
    CbvSrvUav,
    FrameBuffer,
    Count
};

class DX12
{
public:
    DX12(UINT aWidth, UINT aHeight, bool aUseWarpDevice);
    ~DX12();

    void Cleanup();
	void LoadPipeline();
	void PrepareRender();
	void ExecuteRender();
    void EndRender();
	void WaitForGPU();
	void WaitForComputeGPU();
    void WaitForNextFrame();
    void MoveToNextFrame();

    void GetHardwareAdapter(
        _In_ IDXGIFactory1* pFactory,
        _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter,
        bool requestHighPerformanceAdapter = false);

    static constexpr UINT MAX_CBV_COUNT = 1;
    static constexpr UINT MAX_SRV_COUNT = 4096;
    static constexpr UINT MAX_UAV_COUNT = 0; // TODO: add UAV support

    static constexpr UINT MAX_COMPUTE_STATIC_CBV_COUNT = 0;
    static constexpr UINT MAX_COMPUTE_STATIC_SRV_COUNT = static_cast<UINT>(ComputeSrvStaticOffsets::Count);
    static constexpr UINT MAX_COMPUTE_STATIC_UAV_COUNT = 0;
    
    static constexpr UINT MAX_COMPUTE_PER_FRAME_CBV_COUNT = 0;
    static constexpr UINT MAX_COMPUTE_PER_FRAME_SRV_COUNT = 0;
    static constexpr UINT MAX_COMPUTE_PER_FRAME_UAV_COUNT = static_cast<UINT>(ComputeUavDynamicOffsets::Count);
    
    static constexpr UINT MAX_BOUND_SRV_COUNT = 64;
    static constexpr UINT INSTANCE_BUFFER_SIZE = 4096;

    static constexpr UINT CBV_SIZE = 0; // TODO: Change to 2 here
    static constexpr UINT SRV_SIZE = static_cast<UINT>(SrvOffsets::Count);
    static constexpr UINT UAV_SIZE = 0;
    static constexpr UINT CBV_SRV_UAV_SIZE = CBV_SIZE + SRV_SIZE + UAV_SIZE; //* FrameCount; // 2srv + 1uav

    // static constexpr UINT COMPUTE_CBV_SIZE = 0; // TODO: Change to 2 here
    // static constexpr UINT COMPUTE_SRV_SIZE = static_cast<UINT>(SrvOffsets::Count);
    // static constexpr UINT COMPUTE_UAV_SIZE = RenderConstants::FrameCount * static_cast<UINT>(ComputeUavOffsets::Count);
    // static constexpr UINT COMPUTE_CBV_SRV_UAV_SIZE = COMPUTE_CBV_SIZE + COMPUTE_SRV_SIZE + COMPUTE_UAV_SIZE; //* FrameCount; // 2srv + 1uav

    // Pipeline objects
    CD3DX12_VIEWPORT myViewport;
    CD3DX12_RECT myScissorRect;
    ComPtr<IDXGISwapChain3> mySwapChain;
    ComPtr<ID3D12Device> myDevice;
    ComPtr<ID3D12Resource> myRenderTargets[RenderConstants::FrameCount];
    ComPtr<ID3D12Resource> myDepthBuffer;
    
    ComPtr<ID3D12CommandAllocator> myCommandAllocator[RenderConstants::FrameCount];
    ComPtr<ID3D12CommandAllocator> myComputeCommandAllocator[RenderConstants::FrameCount];
    ComPtr<ID3D12CommandAllocator> myBundleAllocator;
    
    ComPtr<ID3D12CommandQueue> myCommandQueue;
    ComPtr<ID3D12CommandQueue> myComputeCommandQueue;

    ComPtr<ID3D12CommandSignature> myCommandSignature;

    ComPtr<ID3D12RootSignature> myRootSignature;
    ComPtr<ID3D12RootSignature> myComputeRootSignature;
    
    ComPtr<ID3D12DescriptorHeap> myRtvHeap;
    ComPtr<ID3D12DescriptorHeap> myDsvHeap;
    
    BindlessDescriptorHeap myComputeCbvSrvUavHeap;
    DescriptorHeap mySrvHeap;
    StagingDescriptorHeap mySrvStagingHeap;
    
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

    ComPtr<ID3D12Resource> myProcessedCommandBuffers[RenderConstants::FrameCount];
    ComPtr<ID3D12Resource> myProcessedCommandBufferCounterReset;

    MeshRenderer meshRenderer;
    // Synchronization objects
    UINT cbvSrvUavDescriptorSize = 0;
    UINT myFrameIndex;
    HANDLE myFenceEvent;
    HANDLE myComputeFenceEvent;
    ComPtr<ID3D12Fence> myFence;
    ComPtr<ID3D12Fence> myComputeFence;
    UINT64 myFenceValues[RenderConstants::FrameCount];
    UINT64 myComputeFenceValues[RenderConstants::FrameCount];
    bool swapChainOccluded = false;
    bool useVSync = false;
private:
    bool useWarpDevice;
};

