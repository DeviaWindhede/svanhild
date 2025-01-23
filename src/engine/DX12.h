#pragma once
#include "DXHelper.h"
#include <SceneBufferTypes.h>
#include "StagingDescriptorHeap.h"
#include "FrameBuffer.h"

using Microsoft::WRL::ComPtr;

class DX12
{
public:
    DX12(UINT aWidth, UINT aHeight, bool aUseWarpDevice);
    ~DX12();

    void Cleanup();
	void LoadPipeline();
	void PrepareRender();
	void ExecuteRender();
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

    // Pipeline objects
    CD3DX12_VIEWPORT myViewport;
    CD3DX12_RECT myScissorRect;
    ComPtr<IDXGISwapChain3> mySwapChain;
    ComPtr<ID3D12Device> myDevice;
    ComPtr<ID3D12Resource> myRenderTargets[FrameCount];
    ComPtr<ID3D12Resource> myDepthBuffer;
    ComPtr<ID3D12Resource> instanceBuffer;
    ComPtr<ID3D12Resource> instanceUploadBuffer;
    ComPtr<ID3D12CommandAllocator> myCommandAllocator[FrameCount];
    ComPtr<ID3D12CommandAllocator> myBundleAllocator;
    ComPtr<ID3D12CommandQueue> myCommandQueue;
    ComPtr<ID3D12RootSignature> myRootSignature;
    ComPtr<ID3D12DescriptorHeap> myRtvHeap;
    ComPtr<ID3D12DescriptorHeap> myCbvHeap;
    DescriptorHeap mySrvHeap;
    StagingDescriptorHeap mySrvStagingHeap;
    ComPtr<ID3D12DescriptorHeap> myDsvHeap;
    ComPtr<ID3D12GraphicsCommandList> myCommandList;
    ComPtr<ID3D12GraphicsCommandList> myBundle;

    FrameBuffer frameBuffer;
    HANDLE mySwapChainWaitableObject = nullptr;
    UINT myRtvDescriptorSize;
    size_t currentPSO;
    size_t currentComputePSO;


    // Synchronization objects
    UINT myFrameIndex;
    HANDLE myFenceEvent;
    ComPtr<ID3D12Fence> myFence;
    UINT64 myFenceValues[FrameCount];
    bool swapChainOccluded = false;
    bool useVSync = false;
private:
    bool useWarpDevice;
};

