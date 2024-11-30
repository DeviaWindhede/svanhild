#include "pch.h"
#include "DX12.h"

#include "DXHelper.h"
#include <IWindow.h>

DX12::DX12(UINT aWidth, UINT aHeight, bool aUseWarpDevice) :
	myFrameIndex(0),
	myViewport(0.0f, 0.0f, static_cast<float>(aWidth), static_cast<float>(aHeight)),
	myScissorRect(0, 0, static_cast<LONG>(aWidth), static_cast<LONG>(aHeight)),
	myFenceValues{},
	myRtvDescriptorSize(0),
	myFenceEvent{},
	useWarpDevice(aUseWarpDevice)
{

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
				requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
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

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Width = myViewport.Width;
	swapChainDesc.Height = myViewport.Height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.Scaling = DXGI_SCALING_NONE;
	//swapChainDesc.Scaling = DXGI_SCALING_STRETCH;

	ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed(factory->CreateSwapChainForHwnd(
		myCommandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
		Win32Application::GetHwnd(),
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	));

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
			rtvHeapDesc.NumDescriptors = FrameCount;
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			ThrowIfFailed(myDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&myRtvHeap)));

			// Describe and create a constant buffer view (CBV) descriptor heap.
			// Flags indicate that this descriptor heap can be bound to the pipeline 
			// and that descriptors contained in it can be referenced by a root table.
			D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
			cbvHeapDesc.NumDescriptors = CbvCount;
			cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			ThrowIfFailed(myDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&myCbvHeap)));

			myRtvDescriptorSize = myDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

			D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
			srvHeapDesc.NumDescriptors = SrvCount;
			srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			ThrowIfFailed(myDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mySrvHeap)));

			myRtvDescriptorSize = myDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		}
	}

	// Create frame resources.
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(myRtvHeap->GetCPUDescriptorHandleForHeapStart());

		// Create a RTV for each frame.
		for (UINT n = 0; n < FrameCount; n++)
		{
			ThrowIfFailed(mySwapChain->GetBuffer(n, IID_PPV_ARGS(&myRenderTargets[n])));
			myDevice->CreateRenderTargetView(myRenderTargets[n].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, myRtvDescriptorSize);

			ThrowIfFailed(myDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&myCommandAllocator[n])));
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

		CD3DX12_DESCRIPTOR_RANGE1 ranges[1]{ {} };
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, SrvCount, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		//ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

		CD3DX12_ROOT_PARAMETER1 rootParameters[2]{ {}, {} };
		rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_VERTEX);
		rootParameters[1].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);

		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 0;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
		ThrowIfFailed(myDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&myRootSignature)));
	}

	// Depth buffer 
	{
		D3D12_HEAP_PROPERTIES dsHeapProperties;
		ZeroMemory(&dsHeapProperties, sizeof(&dsHeapProperties));

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

		myDevice->CreateDepthStencilView(myDepthBuffer.Get(), &dsvDesc, myDsvHeap->GetCPUDescriptorHandleForHeapStart());
	}

	// Create the constant buffer.
	{
		const UINT descriptorSize = myDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		const UINT constantBufferSize = sizeof(FrameBuffer);    // CB size is required to be 256-byte aligned.

		ThrowIfFailed(myDevice->CreateCommittedResource(
			&keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)),
			D3D12_HEAP_FLAG_NONE,
			&keep(CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize)),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&frameBuffer)));

		// Describe and create a constant buffer view.
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc[1] = {};
		cbvDesc[0].BufferLocation = frameBuffer->GetGPUVirtualAddress();
		cbvDesc[0].SizeInBytes = constantBufferSize;

		// Map and initialize the constant buffer. We don't unmap this until the
		// app closes. Keeping things mapped for the lifetime of the resource is okay.
		CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(frameBuffer->Map(0, &readRange, reinterpret_cast<void**>(&frameBufferCbvDataBegin)));
		memcpy(frameBufferCbvDataBegin, &frameBufferData, sizeof(frameBufferData));
	}

	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif
		ThrowIfFailed(D3DCompileFromFile(IWindow::GetEngineShaderFullPath(L"default.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
		ThrowIfFailed(D3DCompileFromFile(IWindow::GetEngineShaderFullPath(L"default.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));
	
		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 56, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = myRootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = TRUE;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL; // Test: pass if the pixel is closer to the camera
		psoDesc.DepthStencilState.StencilEnable = FALSE;  // Disable stencil testing
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;

		ThrowIfFailed(myDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&myPipelineState)));
	}

	// Create synchronization objects and wait until assets have been uploaded to the GPU.
	{
		ThrowIfFailed(myDevice->CreateFence(myFenceValues[myFrameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&myFence)));
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
	ThrowIfFailed(myDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, myCommandAllocator[myFrameIndex].Get(), myPipelineState.Get(), IID_PPV_ARGS(&myCommandList)));
}

void DX12::PrepareRender()
{
}

void DX12::ExecuteRender()
{
	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { myCommandList.Get() };
	myCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame.
	ThrowIfFailed(mySwapChain->Present(1, 0));

	MoveToNextFrame();
}

// Wait for pending GPU work to complete.
void DX12::WaitForGPU()
{
	// Schedule a Signal command in the queue.
	ThrowIfFailed(myCommandQueue->Signal(myFence.Get(), myFenceValues[myFrameIndex]));

	// Wait until the fence has been processed.
	ThrowIfFailed(myFence->SetEventOnCompletion(myFenceValues[myFrameIndex], myFenceEvent));
	WaitForSingleObjectEx(myFenceEvent, INFINITE, FALSE);

	// Increment the fence value for the current frame.
	myFenceValues[myFrameIndex]++;
}

// Prepare to render the next frame.
void DX12::MoveToNextFrame()
{
	// Schedule a Signal command in the queue.
	const UINT64 currentFenceValue = myFenceValues[myFrameIndex];
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
}
