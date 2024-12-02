#pragma once
#include "DXHelper.h"
#include <SceneBufferTypes.h>
#include <queue>

using Microsoft::WRL::ComPtr;

class DX12
{
public:
    DX12(UINT aWidth, UINT aHeight, bool aUseWarpDevice);
    ~DX12();

	void LoadPipeline();
	void PrepareRender();
	void ExecuteRender();
	void WaitForGPU();
    void MoveToNextFrame();

    UINT ReserveSrvIndex();
    void ReturnSrvIndex(UINT aIndex);

    void GetHardwareAdapter(
        _In_ IDXGIFactory1* pFactory,
        _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter,
        bool requestHighPerformanceAdapter = false);

    static constexpr UINT FrameCount = 2;
    static constexpr UINT MAX_CBV_COUNT = 1;
    static constexpr UINT MAX_SRV_COUNT = 4096;
    static constexpr UINT MAX_UAV_COUNT = 0; // TODO: add UAV support

    // Pipeline objects
    CD3DX12_VIEWPORT myViewport;
    CD3DX12_RECT myScissorRect;
    ComPtr<IDXGISwapChain3> mySwapChain;
    ComPtr<ID3D12Device> myDevice;
    ComPtr<ID3D12Resource> myRenderTargets[FrameCount];
    ComPtr<ID3D12Resource> myDepthBuffer;
    ComPtr<ID3D12CommandAllocator> myCommandAllocator[FrameCount];
    ComPtr<ID3D12CommandAllocator> myBundleAllocator;
    ComPtr<ID3D12CommandQueue> myCommandQueue;
    ComPtr<ID3D12RootSignature> myRootSignature;
    ComPtr<ID3D12DescriptorHeap> myRtvHeap;
    ComPtr<ID3D12DescriptorHeap> myCbvHeap;
    ComPtr<ID3D12DescriptorHeap> mySrvHeap;
    ComPtr<ID3D12DescriptorHeap> myDsvHeap;
    ComPtr<ID3D12PipelineState> myPipelineState;
    ComPtr<ID3D12GraphicsCommandList> myCommandList;
    ComPtr<ID3D12GraphicsCommandList> myBundle;
    UINT myRtvDescriptorSize;

    // Synchronization objects
    UINT myFrameIndex;
    HANDLE myFenceEvent;
    ComPtr<ID3D12Fence> myFence;
    UINT64 myFenceValues[FrameCount];

private:
    std::queue<UINT> srvIndices{};
    bool useWarpDevice;
};

