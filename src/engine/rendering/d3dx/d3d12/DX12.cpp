#include "pch.h"
#include "DX12.h"

#include "app/Win32Application.h"
#include "rendering/ShaderCompiler.h"

// static inline UINT AlignForUavCounter(UINT bufferSize)
// {
//     const UINT alignment = D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT;
//     return (bufferSize + (alignment - 1)) & ~(alignment - 1);
// }

// const UINT NumberOfMeshes = 256;
// const UINT CommandSizePerFrame = NumberOfMeshes * sizeof(DrawIndirectArgs);
// const UINT CommandBufferCounterOffset = AlignForUavCounter(CommandSizePerFrame);

DX12::DX12(UINT aWidth, UINT aHeight, bool aUseWarpDevice) :
    myFrameIndex(0),
    myViewport(0.0f, 0.0f, static_cast<float>(aWidth), static_cast<float>(aHeight)),
    myScissorRect(0, 0, static_cast<LONG>(aWidth), static_cast<LONG>(aHeight)),
    myFenceValues{},
    myRtvDescriptorSize(0),
    myFenceEvent{},
    useWarpDevice(aUseWarpDevice),
    instanceBuffer(this)
{
    ShaderCompiler::CreateInstance(*this);
}

DX12::~DX12()
{
}

// Helper function for acquiring the first available hardware adapter that supports Direct3D 12.
// If no such adapter can be found, *ppAdapter will be set to nullptr.
_Use_decl_annotations_

void DX12::GetHardwareAdapter(
    IDXGIFactory1* pFactory,
    IDXGIAdapter1** ppAdapter,
    bool requestHighPerformanceAdapter)
{
    *ppAdapter = nullptr;

    ComPtr<IDXGIAdapter1> adapter;

    ComPtr<IDXGIFactory6> factory6;
    if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
    {
        for (
            UINT adapterIndex = 0;
            SUCCEEDED(factory6->EnumAdapterByGpuPreference(
                adapterIndex,
                requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE :
                DXGI_GPU_PREFERENCE_UNSPECIFIED,
                IID_PPV_ARGS(&adapter)));
            ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                // If you want a software adapter, pass in "/warp" on the command line.
                continue;
            }

            // Check to see whether the adapter supports Direct3D 12, but don't create the
            // actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
            {
                break;
            }
        }
    }

    if (adapter.Get() == nullptr)
    {
        for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                // If you want a software adapter, pass in "/warp" on the command line.
                continue;
            }

            // Check to see whether the adapter supports Direct3D 12, but don't create the
            // actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
            {
                break;
            }
        }
    }

    *ppAdapter = adapter.Detach();
}

#define ENABLE_GPU_VALIDATION 1
#define REPORT_LIVE_OBJECTS 1

void DX12::Cleanup()
{
    if (myFenceEvent != nullptr && myFence != nullptr)
    {
        WaitForGPU();
    }
    ShaderCompiler::DestroyInstance();

    meshRenderer.Cleanup();
    
    if (mySwapChainWaitableObject != nullptr) { CloseHandle(mySwapChainWaitableObject); }
    if (mySwapChain)
    {
        mySwapChain->SetFullscreenState(false, nullptr);
        mySwapChain = nullptr;
    }
    if (myBundle) myBundle = nullptr;
    if (myCommandList) myCommandList = nullptr;
    if (myComputeCommandList) myComputeCommandList = nullptr;
    if (myRtvHeap) myRtvHeap = nullptr;
    if (myComputeCbvSrvUavHeap.GetHeap()) myComputeCbvSrvUavHeap = {};
    if (myRootSignature) myRootSignature = nullptr;
    if (myComputeRootSignature) myComputeRootSignature = nullptr;

    
    if (myFenceEvent != nullptr && myFence != nullptr)
    {
        myCommandQueue->Signal(myFence.Get(), ++myFenceValues[myFrameIndex]);
        if (myFence->GetCompletedValue() < myFenceValues[myFrameIndex]) {
            myFence->SetEventOnCompletion(myFenceValues[myFrameIndex], myFenceEvent);
            WaitForSingleObjectEx(myFenceEvent, INFINITE, FALSE);
        }
    }

    if (myCommandQueue) myCommandQueue = nullptr;
    if (myFence) myFence = nullptr;
    if (myFenceEvent)
    {
        CloseHandle(myFenceEvent);
        myFenceEvent = nullptr;
    }
    
    if (myComputeFenceEvent != nullptr && myComputeFence != nullptr)
    {
        myComputeCommandQueue->Signal(myComputeFence.Get(), ++myComputeFenceValues[myFrameIndex]);
        if (myComputeFence->GetCompletedValue() < myComputeFenceValues[myFrameIndex]) {
            myComputeFence->SetEventOnCompletion(myComputeFenceValues[myFrameIndex], myComputeFenceEvent);
            WaitForSingleObjectEx(myComputeFenceEvent, INFINITE, FALSE);
        }
    }
    
    if (myBundleAllocator) myBundleAllocator = nullptr;
    if (myComputeCommandQueue) myComputeCommandQueue = nullptr;
    if (myComputeFence) myComputeFence = nullptr;
    if (myCommandSignature) myCommandSignature = nullptr;
    if (myComputeFenceEvent)
    {
        CloseHandle(myComputeFenceEvent);
        myComputeFenceEvent = nullptr;
    }
    
    for (UINT i = 0; i < RenderConstants::FrameCount; i++)
    {
        if (myRenderTargets[i]) myRenderTargets[i] = nullptr;
        if (myCommandAllocator[i]) myCommandAllocator[i] = nullptr;
        if (myComputeCommandAllocator[i]) myComputeCommandAllocator[i] = nullptr;
        if (myCommandAllocator[i]) myCommandAllocator[i] = nullptr;
        if (myComputeCommandAllocator[i]) myComputeCommandAllocator[i] = nullptr;
        if (myProcessedCommandBuffers[i]) myProcessedCommandBuffers[i] = nullptr;
    }

    if (myDepthBuffer) myDepthBuffer = nullptr;
    
#if REPORT_LIVE_OBJECTS
    {
        OutputDebugStringA("========== LIVE OBJECT DETAILED REPORT BEGIN ==========\n");
        
        ComPtr<ID3D12DebugDevice> debugDevice;
        if (SUCCEEDED(myDevice->QueryInterface(IID_PPV_ARGS(&debugDevice)))) {
            debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL); // or D3D12_RLDO_SUMMARY
        }
        
        OutputDebugStringA("========== LIVE OBJECT DETAILED REPORT END ==========\n");
    }
#endif
    
    if (myDevice) { myDevice = nullptr; }
}

void DX12::LoadPipeline()
{
    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();

            // Enable additional debug layers.
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }

#if ENABLE_GPU_VALIDATION
        Microsoft::WRL::ComPtr<ID3D12Debug1> debugController1;
        if (SUCCEEDED(debugController.As(&debugController1)))
        {
            debugController1->SetEnableGPUBasedValidation(TRUE);
        }
#endif
    }
#endif

    ComPtr<IDXGIFactory4> factory;
    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

    if (useWarpDevice)
    {
        ComPtr<IDXGIAdapter> warpAdapter;
        ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

        ThrowIfFailed(D3D12CreateDevice(
            warpAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&myDevice)
        ));
    }
    else
    {
        ComPtr<IDXGIAdapter1> hardwareAdapter;
        GetHardwareAdapter(factory.Get(), &hardwareAdapter);

        ThrowIfFailed(D3D12CreateDevice(
            hardwareAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&myDevice)
        ));
    }

    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(myDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&myCommandQueue)));
    NAME_D3D12_OBJECT(myCommandQueue);

    D3D12_COMMAND_QUEUE_DESC computeQueueDesc = {};
    computeQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    computeQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;

    ThrowIfFailed(myDevice->CreateCommandQueue(&computeQueueDesc, IID_PPV_ARGS(&myComputeCommandQueue)));
    NAME_D3D12_OBJECT(myComputeCommandQueue);

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = RenderConstants::FrameCount;
    swapChainDesc.Width = 0;
    swapChainDesc.Height = 0;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Scaling = DXGI_SCALING_NONE;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    //swapChainDesc.Scaling = DXGI_SCALING_STRETCH;

    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(factory->CreateSwapChainForHwnd(
        myCommandQueue.Get(), // Swap chain needs the queue so that it can force a flush on it.
        Win32Application::GetHwnd(),
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain
    ));

    swapChain->SetFullscreenState(FALSE, nullptr);

    // This sample does not support fullscreen transitions.
    ThrowIfFailed(factory->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

    ThrowIfFailed(swapChain.As(&mySwapChain));
    myFrameIndex = mySwapChain->GetCurrentBackBufferIndex();

    // Create descriptor heaps.
    {
        // CBV, SRV, UAV
        {
            // Describe and create a render target view (RTV) descriptor heap.
            D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
            rtvHeapDesc.NumDescriptors = RenderConstants::FrameCount;
            rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            ThrowIfFailed(myDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&myRtvHeap)));
            myRtvDescriptorSize = myDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

            myGraphicsCbvSrvUavHeap.Init(
                myDevice.Get(),
                {
                    {
                        MAX_GRAPHICS_STATIC_CBV_COUNT,
                        MAX_GRAPHICS_STATIC_SRV_COUNT,
                        MAX_GRAPHICS_STATIC_UAV_COUNT,
                        MAX_GRAPHICS_PER_FRAME_CBV_COUNT,
                        MAX_GRAPHICS_PER_FRAME_SRV_COUNT,
                        MAX_GRAPHICS_PER_FRAME_UAV_COUNT
                    },
                    {
                        0,
                        MAX_TEXTURE_COUNT,
                        0,
                        0,
                        0,
                        0
                    }
                }
            );
            myGraphicsCbvSrvUavHeap.GetHeap()->SetName(L"CBV_SRV_UAV_HEAP");

            myComputeCbvSrvUavHeap.Init(
                myDevice.Get(),
                {
                    {
                        MAX_GRAPHICS_STATIC_CBV_COUNT,
                        MAX_GRAPHICS_STATIC_SRV_COUNT,
                        0,
                        0,
                        0,
                        0
                    },
                    {
                        0,
                        0,
                        0,
                        MAX_COMPUTE_PER_FRAME_CBV_COUNT,
                        MAX_COMPUTE_PER_FRAME_SRV_COUNT,
                        MAX_COMPUTE_PER_FRAME_UAV_COUNT
                    }
                }
            );
            myComputeCbvSrvUavHeap.GetHeap()->SetName(L"COMPUTE_CBV_SRV_UAV_HEAP");
        }

        frameBuffer.Init(myDevice.Get());
    }

    // Create frame resources.
    {
        // RTV
        {
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(myRtvHeap->GetCPUDescriptorHandleForHeapStart());

            // Create a RTV for each frame.
            for (UINT n = 0; n < RenderConstants::FrameCount; n++)
            {
                ThrowIfFailed(mySwapChain->GetBuffer(n, IID_PPV_ARGS(&myRenderTargets[n])));
                myDevice->CreateRenderTargetView(myRenderTargets[n].Get(), nullptr, rtvHandle);
                rtvHandle.Offset(1, myRtvDescriptorSize);

                ThrowIfFailed(
                    myDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                     IID_PPV_ARGS(&myCommandAllocator[n])));
                ThrowIfFailed(myDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE,
                                                               IID_PPV_ARGS(&myComputeCommandAllocator[n])));
            }
        }
    }

    ThrowIfFailed(myDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE, IID_PPV_ARGS(&myBundleAllocator)));


    // Create a root signature consisting of a descriptor table with a single CBV.
    {
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

        // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

        if (FAILED(myDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
        {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        CD3DX12_ROOT_PARAMETER1 rootParameters[static_cast<UINT>(GraphicsRootParameters::Count)];
        {
            CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
            ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, MAX_GRAPHICS_STATIC_SRV_COUNT, 0, static_cast<UINT>(GraphicsHeapSpaces::Generic), D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);
            rootParameters[static_cast<UINT>(GraphicsRootParameters::CbvSrvUav)].InitAsDescriptorTable(_countof(ranges), ranges, D3D12_SHADER_VISIBILITY_ALL);
        }
        {
            CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
            ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, MAX_GRAPHICS_PER_FRAME_SRV_COUNT, MAX_GRAPHICS_STATIC_SRV_COUNT, static_cast<UINT>(GraphicsHeapSpaces::Generic), D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);
            rootParameters[static_cast<UINT>(GraphicsRootParameters::PerFrameCbvSrvUav)].InitAsDescriptorTable(_countof(ranges), ranges, D3D12_SHADER_VISIBILITY_ALL);
        }
            rootParameters[static_cast<UINT>(GraphicsRootParameters::FrameBuffer)].InitAsConstantBufferView(0, static_cast<UINT>(GraphicsHeapSpaces::Generic), D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_ALL);
            rootParameters[static_cast<UINT>(GraphicsRootParameters::RenderConstants)].InitAsConstants(sizeof(DrawIndirectArgsData) / sizeof(float), 1, static_cast<UINT>(GraphicsHeapSpaces::Generic), D3D12_SHADER_VISIBILITY_ALL);
        {
            CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
            ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, MAX_TEXTURE_COUNT, 0, static_cast<UINT>(GraphicsHeapSpaces::Textures), D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
            rootParameters[static_cast<UINT>(GraphicsRootParameters::Textures)].InitAsDescriptorTable(_countof(ranges), ranges, D3D12_SHADER_VISIBILITY_PIXEL);
        }
        
        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.MipLODBias = 0;
        sampler.MaxAnisotropy = 16;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        sampler.MinLOD = 0.0f;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ShaderRegister = 0;
        sampler.RegisterSpace = 0;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler,
                                   D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        if (FAILED(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error)))
        {
            if (error)
            {
                // Convert the error message from the blob to a string
                std::string errorMsg(static_cast<char*>(error->GetBufferPointer()), error->GetBufferSize());
                OutputDebugStringA(("Failed to serialize root signature: " + errorMsg).c_str());
            }
            else
            {
                OutputDebugStringA("Failed to serialize root signature, no error blob available.");
            }
        }
        ThrowIfFailed(myDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                                    IID_PPV_ARGS(&myRootSignature)));
        NAME_D3D12_OBJECT(myRootSignature);
    }

    // Compute shader root signature
    {

        CD3DX12_ROOT_PARAMETER1 rootParameters[static_cast<UINT>(ComputeRootParameters::Count)];

        {
            CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
            ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, MAX_COMPUTE_STATIC_SRV_COUNT, 0, static_cast<UINT>(ComputeHeapSpaces::Generic), D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE); // instance buffer and instance count buffer
            rootParameters[static_cast<UINT>(ComputeRootParameters::CbvSrvUavTable)].InitAsDescriptorTable(_countof(ranges), ranges);
        }
        {
            CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
            ranges[0].Init(
                D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
                MAX_COMPUTE_PER_FRAME_UAV_COUNT,
                0,
                static_cast<UINT>(ComputeHeapSpaces::ComputeData),
                D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE,
                D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
            ); // output commands buffer array
            
            rootParameters[static_cast<UINT>(ComputeRootParameters::PerFrameCbvSrvUavTable)].InitAsDescriptorTable(_countof(ranges), ranges);
        }
        rootParameters[static_cast<UINT>(ComputeRootParameters::FrameBuffer)].InitAsConstantBufferView(0, static_cast<UINT>(ComputeHeapSpaces::Generic));
        rootParameters[static_cast<UINT>(ComputeRootParameters::RootConstants)].InitAsConstants(sizeof(RootConstants) / sizeof(float), 0, static_cast<UINT>(ComputeHeapSpaces::ComputeData));
        
        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr,
                                   D3D12_ROOT_SIGNATURE_FLAG_NONE);

        ID3DBlob* signature;
        ID3DBlob* error;
        if (FAILED(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &signature, &error)))
        {
            if (error)
            {
                // Convert the error message from the blob to a string
                std::string errorMsg(static_cast<char*>(error->GetBufferPointer()), error->GetBufferSize());
                OutputDebugStringA(("Failed to serialize root signature: " + errorMsg).c_str());
            }
            else
            {
                OutputDebugStringA("Failed to serialize root signature, no error blob available.");
            }
        }
        myDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                      IID_PPV_ARGS(&myComputeRootSignature));
        NAME_D3D12_OBJECT(myComputeRootSignature);
    }

    // Create the command signature used for indirect drawing.
    {
        D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[2] = {};

        argumentDescs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
        argumentDescs[0].Constant.RootParameterIndex = static_cast<UINT>(GraphicsRootParameters::RenderConstants);
        argumentDescs[0].Constant.DestOffsetIn32BitValues = 0;
        argumentDescs[0].Constant.Num32BitValuesToSet = sizeof(DrawIndirectArgsData) / 4; // For InstanceDataOffset + MeshIndex

        argumentDescs[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
        
        D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc = {};
        commandSignatureDesc.ByteStride = sizeof(DrawIndirectArgs);
        commandSignatureDesc.NumArgumentDescs = _countof(argumentDescs);
        commandSignatureDesc.pArgumentDescs = argumentDescs;
        commandSignatureDesc.NodeMask = 0;

        ThrowIfFailed(myDevice->CreateCommandSignature(&commandSignatureDesc, myRootSignature.Get(), IID_PPV_ARGS(&myCommandSignature)));
    }

    // Depth buffer 
    {
        D3D12_HEAP_PROPERTIES dsHeapProperties;
        ZeroMemory(&dsHeapProperties, sizeof(dsHeapProperties));

        dsHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
        dsHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        dsHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        dsHeapProperties.CreationNodeMask = NULL;
        dsHeapProperties.VisibleNodeMask = NULL;

        // Describe resource
        D3D12_RESOURCE_DESC dsResDesc;
        ZeroMemory(&dsResDesc, sizeof(D3D12_RESOURCE_DESC));

        dsResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        dsResDesc.Alignment = 0;
        dsResDesc.Width = myViewport.Width;
        dsResDesc.Height = myViewport.Height;
        dsResDesc.DepthOrArraySize = 1;
        dsResDesc.MipLevels = 1;
        dsResDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsResDesc.SampleDesc.Count = 1;
        dsResDesc.SampleDesc.Quality = 0;
        dsResDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        dsResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        // Describe clear value 
        D3D12_CLEAR_VALUE clearValue;
        ZeroMemory(&clearValue, sizeof(D3D12_CLEAR_VALUE));

        clearValue.Format = DXGI_FORMAT_D32_FLOAT;
        clearValue.DepthStencil.Depth = 1.0f;
        clearValue.DepthStencil.Stencil = 0;

        myDevice->CreateCommittedResource(
            &dsHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &dsResDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clearValue,
            IID_PPV_ARGS(&myDepthBuffer)
        );

        // === Create view description
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = 0;
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.NodeMask = NULL;
        ThrowIfFailed(myDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&myDsvHeap)));

        myDevice->CreateDepthStencilView(myDepthBuffer.Get(), &dsvDesc,
                                         myDsvHeap->GetCPUDescriptorHandleForHeapStart());
    }

    // PSO
    {
        size_t vertexShader = 0;
        size_t pixelShader = 0;
        size_t computeShader = 0;

        ShaderCompiler::CompileShader(L"default", ShaderType::Vertex, vertexShader);
        ShaderCompiler::CompileShader(L"default", ShaderType::Pixel, pixelShader);
        ShaderCompiler::CompileShader(L"cull", ShaderType::Compute, computeShader);
        currentPSO = ShaderCompiler::CreatePSO(vertexShader, pixelShader);
        currentComputePSO = ShaderCompiler::CreatePSO(computeShader);
        
        ShaderCompiler::CompileShader(L"commandReset", ShaderType::Compute, computeShader);
        ShaderCompiler::CreatePSO(computeShader);
    }

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
        ThrowIfFailed(myDevice->CreateFence(myFenceValues[myFrameIndex], D3D12_FENCE_FLAG_NONE,
                                            IID_PPV_ARGS(&myFence)));
        ThrowIfFailed(myDevice->CreateFence(myFenceValues[myFrameIndex], D3D12_FENCE_FLAG_NONE,
                                            IID_PPV_ARGS(&myComputeFence)));
        myFenceValues[myFrameIndex]++;

        // Create an event handle to use for frame synchronization.
        myFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (myFenceEvent == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }


        // Wait for the command list to execute; we are reusing the same command 
        // list in our main loop but for now, we just want to wait for setup to 
        // complete before continuing.
        WaitForGPU();
    }

    // Create the command list.
    ThrowIfFailed(myDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, myCommandAllocator[myFrameIndex].Get(),
                                              ShaderCompiler::GetPSO(currentPSO).state.Get(),
                                              IID_PPV_ARGS(&myCommandList)));
    ThrowIfFailed(myDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE,
                                              myComputeCommandAllocator[myFrameIndex].Get(),
                                              ShaderCompiler::GetPSO(currentComputePSO).state.Get(),
                                              IID_PPV_ARGS(&myComputeCommandList)));
    ThrowIfFailed(myComputeCommandList->Close());

    myCommandList->SetName(L"Main_CommandList_Direct");
    myCommandList->SetName(L"Main_CommandList_Compute");

    {
        // Create shader resource views (SRV) of the constant buffers for the
        // compute shader to read from.
        //D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        ////srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        ////srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        //srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;  // A default format (it doesn't matter much since we won't be reading it)
        //srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        //srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        ////srvDesc.Buffer.NumElements = TriangleCount;
        ////srvDesc.Buffer.StructureByteStride = sizeof(SceneConstantBuffer);
        ////srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

        //ID3D12Resource* nullResource = nullptr;
        //CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle(myCbvSrvUavHeap.descriptorHeap->GetCPUDescriptorHandleForHeapStart());
        //for (UINT n = 0; n < SRV_SIZE; n++)
        //{
        //	//srvDesc.Buffer.FirstElement = frame * TriangleCount;
        //	myDevice->CreateShaderResourceView(nullResource, &srvDesc, cbvSrvHandle);
        //	cbvSrvHandle.Offset(1, cbvSrvUavDescriptorSize);
        //}


        //D3D12_RESOURCE_DESC commandBufferDesc;
        //// Create the unordered access views (UAVs) that store the results of the compute work.
        //CD3DX12_CPU_DESCRIPTOR_HANDLE processedCommandsHandle(myCbvSrvUavHeap.descriptorHeap->GetCPUDescriptorHandleForHeapStart(), 2, cbvSrvUavDescriptorSize);
        //for (UINT frame = 0; frame < FrameCount; frame++)
        //{
        //	// Allocate a buffer large enough to hold all of the indirect commands
        //	// for a single frame as well as a UAV counter.
        //	commandBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(CommandBufferCounterOffset + sizeof(UINT), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        //	ThrowIfFailed(myDevice->CreateCommittedResource(
        //		&keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
        //		D3D12_HEAP_FLAG_NONE,
        //		&commandBufferDesc,
        //		D3D12_RESOURCE_STATE_COMMON,
        //		nullptr,
        //		IID_PPV_ARGS(&myProcessedCommandBuffers[frame])));

        //	NAME_D3D12_OBJECT_INDEXED(myProcessedCommandBuffers, frame);

        //	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        //	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        //	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        //	uavDesc.Buffer.FirstElement = 0;
        //	uavDesc.Buffer.NumElements = NumberOfMeshes; // TODO
        //	uavDesc.Buffer.StructureByteStride = sizeof(IndirectCommand);
        //	uavDesc.Buffer.CounterOffsetInBytes = CommandBufferCounterOffset;
        //	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

        //	myDevice->CreateUnorderedAccessView(
        //		myProcessedCommandBuffers[frame].Get(),
        //		myProcessedCommandBuffers[frame].Get(),
        //		&uavDesc,
        //		processedCommandsHandle);

        //	processedCommandsHandle.Offset(CBV_SRV_UAV_SIZE, cbvSrvUavDescriptorSize);
        //}


        //// Allocate a buffer that can be used to reset the UAV counters and initialize
        //// it to 0.
        //ThrowIfFailed(myDevice->CreateCommittedResource(
        //	&keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)),
        //	D3D12_HEAP_FLAG_NONE,
        //	&keep(CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT))),
        //	D3D12_RESOURCE_STATE_GENERIC_READ,
        //	nullptr,
        //	IID_PPV_ARGS(&myProcessedCommandBufferCounterReset)));

        //UINT8* pMappedCounterReset = nullptr;
        //CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
        //ThrowIfFailed(myProcessedCommandBufferCounterReset->Map(0, &readRange, reinterpret_cast<void**>(&pMappedCounterReset)));
        //ZeroMemory(pMappedCounterReset, sizeof(UINT));
        //myProcessedCommandBufferCounterReset->Unmap(0, nullptr);
    }

    // instanceBuffer.Create(this);
    meshRenderer.Create(this);

    // Close the command list and execute it to begin the initial GPU setup.
    ThrowIfFailed(myCommandList->Close());
    ID3D12CommandList* ppCommandLists[] = {myCommandList.Get()};
    myCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
    WaitForGPU();
}

void DX12::PrepareRender()
{
    ThrowIfFailed(myCommandAllocator[myFrameIndex]->Reset());
    ThrowIfFailed(myCommandList->Reset(myCommandAllocator[myFrameIndex].Get(),
                                       ShaderCompiler::GetPSO(currentPSO).state.Get()));

    {
        // WaitForComputeGPU();
        ThrowIfFailed(myComputeCommandAllocator[myFrameIndex]->Reset());
        ThrowIfFailed(myComputeCommandList->Reset(myComputeCommandAllocator[myFrameIndex].Get(),
                                                  ShaderCompiler::GetPSO(currentComputePSO).state.Get()));

        if (currentComputePSO < SIZE_T_MAX && meshRenderer.IsReady())
        {
            myComputeCommandList->SetComputeRootSignature(myComputeRootSignature.Get());

            ID3D12DescriptorHeap* descriptorHeaps[] = { myComputeCbvSrvUavHeap.GetHeap() };
            myComputeCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
            myComputeCommandList->SetComputeRootDescriptorTable(static_cast<UINT>(ComputeRootParameters::CbvSrvUavTable), myComputeCbvSrvUavHeap.GetStaticGPUHandleStart());
            myComputeCommandList->SetComputeRootDescriptorTable(static_cast<UINT>(ComputeRootParameters::PerFrameCbvSrvUavTable), myComputeCbvSrvUavHeap.GetPerFrameGPUHandleStart(myFrameIndex));
            myComputeCommandList->SetComputeRootConstantBufferView(static_cast<UINT>(ComputeRootParameters::FrameBuffer), frameBuffer.resource->GetGPUVirtualAddress());

            meshRenderer.Dispatch();
        }
        ThrowIfFailed(myComputeCommandList->Close());
        
        ID3D12CommandList* ppCommandLists[] = {myComputeCommandList.Get()};
        myComputeCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
    
        myComputeCommandQueue->Signal(myComputeFence.Get(), myComputeFenceValues[myFrameIndex]);
    }

    {
        myCommandList->SetGraphicsRootSignature(myRootSignature.Get());

        ID3D12DescriptorHeap* ppHeaps[] = {myGraphicsCbvSrvUavHeap.GetHeap()};
        myCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

        myCommandList->SetGraphicsRootDescriptorTable(static_cast<UINT>(GraphicsRootParameters::CbvSrvUav), myGraphicsCbvSrvUavHeap.GetStaticGPUHandleStart());
        myCommandList->SetGraphicsRootDescriptorTable(static_cast<UINT>(GraphicsRootParameters::PerFrameCbvSrvUav), myGraphicsCbvSrvUavHeap.GetPerFrameGPUHandleStart(myFrameIndex));
        myCommandList->SetGraphicsRootDescriptorTable(static_cast<UINT>(GraphicsRootParameters::Textures), myGraphicsCbvSrvUavHeap.GetStaticGPUHandle(0, 1));
        myCommandList->SetGraphicsRootConstantBufferView(static_cast<UINT>(GraphicsRootParameters::FrameBuffer), frameBuffer.resource->GetGPUVirtualAddress());

        myCommandList->RSSetViewports(1, &myViewport);
        myCommandList->RSSetScissorRects(1, &myScissorRect);

        // Indicate that the back buffer will be used as a render target.
        {
            CD3DX12_RESOURCE_BARRIER barriers[1]
            {
                CD3DX12_RESOURCE_BARRIER::Transition(
                    myRenderTargets[myFrameIndex].Get(),
                    D3D12_RESOURCE_STATE_PRESENT,
                    D3D12_RESOURCE_STATE_RENDER_TARGET
                )
            };
            
            myCommandList->ResourceBarrier(_countof(barriers), barriers);
        }

        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = myDsvHeap->GetCPUDescriptorHandleForHeapStart();
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(myRtvHeap->GetCPUDescriptorHandleForHeapStart(), myFrameIndex,
                                                myRtvDescriptorSize);
        myCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

        myCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, NULL);

        // Record commands.
        const float clearColor[] = {0.0f, 0.2f, 0.4f, 1.0f};
        myCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

        // Execute the commands stored in the bundle.
        //dx12.myCommandList->ExecuteBundle(dx12.myBundle.Get());

        myCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }
}

void DX12::ExecuteRender()
{
    meshRenderer.ExecuteIndirectRender();
    
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        myRenderTargets[myFrameIndex].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT);
    
    myCommandList->ResourceBarrier(1, &barrier);
        
    ThrowIfFailed(myCommandList->Close());

    if (currentComputePSO < SIZE_T_MAX)
    {
        // Execute the rendering work only when the compute work is complete.
        myCommandQueue->Wait(myComputeFence.Get(), myComputeFenceValues[myFrameIndex]);
    }
    
    // Execute the command list.
    ID3D12CommandList* ppCommandLists[] = {myCommandList.Get()};
    myCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
}

void DX12::EndRender()
{
    HRESULT hr = mySwapChain->Present(static_cast<UINT>(useVSync), static_cast<UINT>(!useVSync) * DXGI_PRESENT_ALLOW_TEARING);
    ThrowIfFailed(hr);
    swapChainOccluded = hr == DXGI_STATUS_OCCLUDED;

    meshRenderer.OnEndFrame();
    // instanceBuffer.OnEndFrame(this);
    MoveToNextFrame();
}

// Wait for pending GPU work to complete.
void DX12::WaitForGPU()
{
    ThrowIfFailed(myCommandQueue->Signal(myFence.Get(), myFenceValues[myFrameIndex]));

    ThrowIfFailed(myFence->SetEventOnCompletion(myFenceValues[myFrameIndex], myFenceEvent));
    WaitForSingleObjectEx(myFenceEvent, INFINITE, FALSE);

    myFenceValues[myFrameIndex]++;
}

void DX12::WaitForComputeGPU()
{
    ThrowIfFailed(myComputeCommandQueue->Signal(myComputeFence.Get(), myComputeFenceValues[myFrameIndex]));

    ThrowIfFailed(myComputeFence->SetEventOnCompletion(myComputeFenceValues[myFrameIndex], myComputeFenceEvent));
    WaitForSingleObjectEx(myComputeFenceEvent, INFINITE, FALSE);

    myComputeFenceValues[myFrameIndex]++;
}

void DX12::WaitForNextFrame()
{
    UINT nextFrameIndex = myFrameIndex + 1;
    myFrameIndex = nextFrameIndex;

    HANDLE waitableObjects[] = {mySwapChainWaitableObject, nullptr};
    DWORD numWaitableObjects = 1;

    UINT64 fenceValue = myFenceValues[myFrameIndex];
    if (fenceValue != 0) // means no fence was signaled
    {
        myFence->SetEventOnCompletion(fenceValue, myFenceEvent);
        waitableObjects[1] = myFenceEvent;
        numWaitableObjects = 2;
    }

    WaitForMultipleObjects(numWaitableObjects, waitableObjects, TRUE, INFINITE);
}

// Prepare to render the next frame.
void DX12::MoveToNextFrame()
{
    // Schedule a Signal command in the queue.
    UINT64 currentFenceValue = myFenceValues[myFrameIndex];
    ThrowIfFailed(myCommandQueue->Signal(myFence.Get(), currentFenceValue));

    // Update the frame index.
    myFrameIndex = mySwapChain->GetCurrentBackBufferIndex();

    // If the next frame is not ready to be rendered yet, wait until it is ready.
    if (myFence->GetCompletedValue() < myFenceValues[myFrameIndex])
    {
        ThrowIfFailed(myFence->SetEventOnCompletion(myFenceValues[myFrameIndex], myFenceEvent));
        WaitForSingleObjectEx(myFenceEvent, INFINITE, FALSE);
    }

    // Set the fence value for the next frame.
    myFenceValues[myFrameIndex] = currentFenceValue + 1;

    
    if (currentComputePSO < SIZE_T_MAX)
    {
        currentFenceValue = myComputeFenceValues[myFrameIndex];
        ThrowIfFailed(myComputeCommandQueue->Signal(myFence.Get(), currentFenceValue));

        // If the next frame is not ready to be rendered yet, wait until it is ready.
        if (myComputeFence->GetCompletedValue() < myComputeFenceValues[myFrameIndex])
        {
            ThrowIfFailed(myComputeFence->SetEventOnCompletion(myComputeFenceValues[myFrameIndex], myComputeFenceEvent));
            WaitForSingleObjectEx(myComputeFenceEvent, INFINITE, FALSE);
        }

        // Set the fence value for the next frame.
        myComputeFenceValues[myFrameIndex] = currentFenceValue + 1;
    }
}
