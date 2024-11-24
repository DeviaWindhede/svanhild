//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once

#include "IWindow.h"
#include <vector>
#include "UploadBuffer.h"

#include "Mesh.h"

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
// An example of this can be found in the class method: OnDestroy().
using Microsoft::WRL::ComPtr;

class D3D12Window : public IWindow
{
public:
    D3D12Window(UINT width, UINT height, std::wstring name);

    virtual void OnInit();
    virtual void OnUpdate();
    virtual void OnRender();
    virtual void OnDestroy();

    __forceinline void Quit() { PostQuitMessage(0); };
private:
    // In this sample we overload the meaning of FrameCount to mean both the maximum
    // number of frames that will be queued to the GPU at a time, as well as the number
    // of back buffers in the DXGI swap chain. For the majority of applications, this
    // is convenient and works well. However, there will be certain cases where an
    // application may want to queue up more frames than there are back buffers
    // available.
    // It should be noted that excessive buffering of frames dependent on user input
    // may result in noticeable latency in your app.
    static const UINT FrameCount = 2;
    static const UINT TextureWidth = 256;
    static const UINT TextureHeight = 256;
    static const UINT TexturePixelSize = 4;    // The number of bytes used to represent a pixel in the texture.

    static const UINT CbvCount = 1;
    static const UINT SrvCount = 4096;
    static const UINT UavCount = 0; // TODO: add UAV support

    //float4x4 g_view;
    //float4x4 g_projection;
    //float2 g_resolution;
    //float2 g_viewport;
    //float g_nearPlane;
    //float g_farPlane;
    //float2 g_padding;
    //float4 padding[4];

    DirectX::XMMATRIX _cameraTransform;
    DirectX::XMFLOAT3 _cameraPosition;
    float _cameraPitch;
    float _cameraYaw;

    struct SceneConstantBuffer
    {
        DirectX::XMMATRIX view;
        DirectX::XMMATRIX projection;
        DirectX::XMFLOAT2 resolution;
        DirectX::XMFLOAT2 viewport;
        float nearPlane;
        float farPlane;
        float padding0[2];
        DirectX::XMMATRIX testTransform;
        DirectX::XMFLOAT4 offset;
        float padding[4]; // Padding so the constant buffer is 256-byte aligned.
    };
    static_assert((sizeof(SceneConstantBuffer) % 256) == 0, "Constant Buffer size must be 256-byte aligned");

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

    // App resources
    ComPtr<ID3D12Resource> m_constantBuffer;
    ComPtr<ID3D12Resource> m_texture;
    SceneConstantBuffer m_constantBufferData;
    UINT8* m_pCbvDataBegin;

    // Synchronization objects
    UINT myFrameIndex;
    HANDLE myFenceEvent;
    ComPtr<ID3D12Fence> myFence;
    UINT64 myFenceValues[FrameCount];

    void LoadPipeline();
    void LoadAssets();
    void PopulateCommandList();
    void MoveToNextFrame();
    void WaitForGpu();

    Mesh myTempMesh;
    void LoadMesh(class Mesh& aMesh);
    void LoadTexture();

    std::vector<UINT8> GenerateTextureData();
};
