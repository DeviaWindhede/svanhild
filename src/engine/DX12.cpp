#include "pch.h"
#include "DX12.h"

#include "DXHelper.h"
#include <IWindow.h>
#include <ShaderCompiler.h>

DX12::DX12(UINT aWidth, UINT aHeight, bool aUseWarpDevice) :
	myFrameIndex(0),
	myViewport(0.0f, 0.0f, static_cast<float>(aWidth), static_cast<float>(aHeight)),
	myScissorRect(0, 0, static_cast<LONG>(aWidth), static_cast<LONG>(aHeight)),
	myFenceValues{},
	myRtvDescriptorSize(0),
	myFenceEvent{},
	useWarpDevice(aUseWarpDevice)
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

#define ENABLE_GPU_VALIDATION 1

void DX12::Cleanup()
{
	if (myFenceEvent != nullptr && myFence != nullptr) {
		WaitForGPU();
	}
	ShaderCompiler::DestroyInstance();

	for (UINT i = 0; i < FrameCount; i++)
	{
		if (myRenderTargets[i])
		{
			myRenderTargets[i] = nullptr;
		}
	}

	if (mySwapChainWaitableObject != nullptr) { CloseHandle(mySwapChainWaitableObject); }
	if (mySwapChain) { mySwapChain->SetFullscreenState(false, nullptr); mySwapChain = nullptr; }
	for (UINT i = 0; i < FrameCount; i++)
		if (myCommandAllocator[i]) { myCommandAllocator[i] = nullptr; }
	if (myCommandQueue) { myCommandQueue = nullptr; }
	if (myCommandList) { myCommandList = nullptr; }
	if (myRtvHeap) { myRtvHeap = nullptr; }
	if (mySrvHeap.descriptorHeap) { mySrvHeap = {}; }
	if (myFence) { myFence = nullptr; }
	if (myFenceEvent) { CloseHandle(myFenceEvent); myFenceEvent = nullptr; }
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
		if (SUCCEEDED(debugController.As(&debugController1))) {
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

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
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
		myCommandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
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
			rtvHeapDesc.NumDescriptors = FrameCount;
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			ThrowIfFailed(myDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&myRtvHeap)));
			myRtvDescriptorSize = myDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

			// Describe and create a constant buffer view (CBV) descriptor heap.
			// Flags indicate that this descriptor heap can be bound to the pipeline 
			// and that descriptors contained in it can be referenced by a root table.
			D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
			cbvHeapDesc.NumDescriptors = MAX_CBV_COUNT;
			cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			ThrowIfFailed(myDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&myCbvHeap)));

			mySrvHeap.Init(myDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, MAX_BOUND_SRV_COUNT, true);
			mySrvHeap.descriptorHeap->SetName(L"SRV_Heap");
			mySrvStagingHeap.Init(myDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, MAX_SRV_COUNT, false);
			mySrvHeap.descriptorHeap->SetName(L"SRV_Heap_Staging");

			//// SRV Staging Heap
			//{
			//	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
			//	srvHeapDesc.NumDescriptors = MAX_SRV_COUNT;
			//	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			//	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			//	ThrowIfFailed(myDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mySrvStagingHeap)));
			//}
			//// SRV Heap
			//{
			//	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
			//	srvHeapDesc.NumDescriptors = MAX_BOUND_SRV_COUNT;
			//	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			//	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			//	ThrowIfFailed(myDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mySrvHeap)));
			//}
		}

		frameBuffer.Init(*this);
	}

	// Create frame resources.
	{
		// RTV
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

		CD3DX12_DESCRIPTOR_RANGE1 ranges[1]{};
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, MAX_BOUND_SRV_COUNT, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		//ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, MAX_BOUND_SRV_COUNT, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		//ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

		CD3DX12_ROOT_PARAMETER1 rootParameters[2]{ {}, {} };
		rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_VERTEX);
		rootParameters[1].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);

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

	// Instance buffer
	{
		D3D12_HEAP_PROPERTIES defaultHeapProps = { D3D12_HEAP_TYPE_DEFAULT };
		D3D12_RESOURCE_DESC instanceBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(INSTANCE_BUFFER_SIZE);

		myDevice->CreateCommittedResource(
			&defaultHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&instanceBufferDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&instanceBuffer)
		);

		D3D12_HEAP_PROPERTIES uploadHeapProps = { D3D12_HEAP_TYPE_UPLOAD };

		myDevice->CreateCommittedResource(
			&uploadHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&instanceBufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&instanceUploadBuffer)
		);
	}

	// PSO
	{
		size_t vertexShader = 0;
		size_t pixelShader = 0;

		ShaderCompiler::CompileShader(L"default", ShaderType::Vertex, vertexShader);
		ShaderCompiler::CompileShader(L"default", ShaderType::Pixel, pixelShader);
		ShaderCompiler::CreatePSO(vertexShader, pixelShader);
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
	ThrowIfFailed(myDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, myCommandAllocator[myFrameIndex].Get(), ShaderCompiler::GetPSO(0).state.Get(), IID_PPV_ARGS(&myCommandList)));
	myCommandList->SetName(L"Main_CommandList");

	// SRV
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC nullSrvDesc = {};
		nullSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		nullSrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;  // A default format (it doesn't matter much since we won't be reading it)
		nullSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		nullSrvDesc.Texture2D.MostDetailedMip = 0;
		nullSrvDesc.Texture2D.MipLevels = 0;

		const UINT descriptorSize = myDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(mySrvHeap.cpuStart);
		CD3DX12_CPU_DESCRIPTOR_HANDLE srvStagingHandle(mySrvStagingHeap.cpuStart);
		ID3D12Resource* nullResource = nullptr;
		for (UINT n = 0; n < MAX_SRV_COUNT; n++)
		{
			if (n < MAX_BOUND_SRV_COUNT)
			{
				myDevice->CreateShaderResourceView(nullResource, &nullSrvDesc, srvHandle);
				srvHandle.Offset(1, descriptorSize);
			}
			myDevice->CreateShaderResourceView(nullResource, &nullSrvDesc, srvStagingHandle);
			srvStagingHandle.Offset(1, descriptorSize);
		}
	}

	// Close the command list and execute it to begin the initial GPU setup.
	ThrowIfFailed(myCommandList->Close());
	ID3D12CommandList* ppCommandLists[] = { myCommandList.Get() };
	myCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	WaitForGPU();
}

void DX12::PrepareRender()
{
}

void DX12::ExecuteRender()
{
	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { myCommandList.Get() };
	myCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	HRESULT hr = mySwapChain->Present((UINT)useVSync, DXGI_PRESENT_ALLOW_TEARING);
	ThrowIfFailed(hr);
	swapChainOccluded = hr == DXGI_STATUS_OCCLUDED;

	MoveToNextFrame();
}

// Wait for pending GPU work to complete.
void DX12::WaitForGPU()
{
	//ThrowIfFailed(myCommandQueue->Signal(myFence.Get(), ++myFenceValues[myFrameIndex]));

	//if (myFence->GetCompletedValue() < myFenceValues[myFrameIndex])
	//{
	//	ThrowIfFailed(myFence->SetEventOnCompletion(myFenceValues[myFrameIndex], myFenceEvent));
	//	WaitForSingleObjectEx(myFenceEvent, INFINITE, FALSE);
	//}

	ThrowIfFailed(myCommandQueue->Signal(myFence.Get(), myFenceValues[myFrameIndex]));
	
	ThrowIfFailed(myFence->SetEventOnCompletion(myFenceValues[myFrameIndex], myFenceEvent));
	WaitForSingleObjectEx(myFenceEvent, INFINITE, FALSE);
	
	myFenceValues[myFrameIndex]++;
}

void DX12::WaitForNextFrame()
{
	UINT nextFrameIndex = myFrameIndex + 1;
	myFrameIndex = nextFrameIndex;

	HANDLE waitableObjects[] = { mySwapChainWaitableObject, nullptr };
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
