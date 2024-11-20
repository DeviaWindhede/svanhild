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

#include "pch.h"
#include "D3D12Window.h"
#include "InputManager.h"

#include "mesh/Vertex.h"
#include <algorithm>

D3D12Window::D3D12Window(UINT width, UINT height, std::wstring name) :
	IWindow(width, height, name),
	myFrameIndex(0),
	myViewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
	myScissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
	myFenceValues{},
	myRtvDescriptorSize(0),
	myFenceEvent{},
	myVertexBufferView{},
	m_pCbvDataBegin{},
	_cameraPitch(0.0f),
	_cameraYaw(0.0f),
	_cameraPosition{ 0.0f, 0.0f, 0.0f },
	m_constantBufferData{}
{
	_cameraTransform = DirectX::XMMatrixTranslation(0.0f, 0.0f, -10.0f);
}

void D3D12Window::OnInit()
{
	LoadPipeline();
	LoadAssets();
}

// Load the rendering pipeline dependencies.
void D3D12Window::LoadPipeline()
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

	if (myUseWarpDevice)
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
	swapChainDesc.Width = myWidth;
	swapChainDesc.Height = myHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;

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
}


#include "mesh/ModelFactory.h"
#include "StringHelper.h"

// Load the sample assets.
void D3D12Window::LoadAssets()
{
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


	// Create the pipeline state, which includes compiling and loading shaders.
	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif

		ThrowIfFailed(D3DCompileFromFile(GetEngineShaderFullPath(L"default.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
		ThrowIfFailed(D3DCompileFromFile(GetEngineShaderFullPath(L"default.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

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
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;

		ThrowIfFailed(myDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&myPipelineState)));
	}

	// Create the command list.
	ThrowIfFailed(myDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, myCommandAllocator[myFrameIndex].Get(), myPipelineState.Get(), IID_PPV_ARGS(&myCommandList)));

	// Command lists are created in the recording state, but there is nothing
	// to record yet. The main loop expects it to be closed, so close it now.
	//ThrowIfFailed(myCommandList->Close());

	// Create the vertex buffer.
	{
		// Define the geometry for a triangle.
		//Vertex triangleVertices[] =
		//{
		//	{ { 0.0f, 0.25f * myAspectRatio, 0.0f }, {}, { 0.5f, 0.0f }, {}, {}, {} },
		//	{ { 0.25f, -0.25f * myAspectRatio, 0.0f }, {}, { 1.0f, 1.0f }, {}, {}, {} },
		//	{ { -0.25f, -0.25f * myAspectRatio, 0.0f }, {}, { 0.0f, 1.0f }, {}, {}, {} }
		//};

		auto package = ModelFactory::LoadMeshFromFBX(StringHelper::ws2s(GetAssetFullPath(L"TGE.fbx")));

		_tempMeshVertexCount = package.meshData[0].vertices.size();
		const UINT vertexBufferSize = sizeof(Vertex) * _tempMeshVertexCount;

		// Note: using upload heaps to transfer static data like vert buffers is not 
		// recommended. Every time the GPU needs it, the upload heap will be marshalled 
		// over. Please read up on Default Heap usage. An upload heap is used here for 
		// code simplicity and because there are very few verts to actually transfer.

		CD3DX12_HEAP_PROPERTIES properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
		ThrowIfFailed(myDevice->CreateCommittedResource(
			&properties,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&myVertexBuffer)));

		// Copy the triangle data to the vertex buffer.
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(myVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, package.meshData[0].vertices.data(), vertexBufferSize);
		myVertexBuffer->Unmap(0, nullptr);

		// Initialize the vertex buffer view.
		myVertexBufferView.BufferLocation = myVertexBuffer->GetGPUVirtualAddress();
		myVertexBufferView.StrideInBytes = sizeof(Vertex);
		myVertexBufferView.SizeInBytes = vertexBufferSize;
	}


	// Note: ComPtr's are CPU objects but this resource needs to stay in scope until
	// the command list that references it has finished executing on the GPU.
	// We will flush the GPU at the end of this method to ensure the resource is not
	// prematurely destroyed.
	ComPtr<ID3D12Resource> textureUploadHeap;

	// Create the texture.
	{
		// Describe and create a Texture2D.
		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.MipLevels = 1;
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		textureDesc.Width = TextureWidth;
		textureDesc.Height = TextureHeight;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		{
			auto properies = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
			ThrowIfFailed(myDevice->CreateCommittedResource(
				&properies,
				D3D12_HEAP_FLAG_NONE,
				&textureDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&m_texture)));
		}

		const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_texture.Get(), 0, 1);

		{
			auto properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
			auto desc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
			// Create the GPU upload buffer.
			ThrowIfFailed(myDevice->CreateCommittedResource(
				&properties,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&textureUploadHeap)));
		}

		// Copy data to the intermediate upload heap and then schedule a copy 
		// from the upload heap to the Texture2D.
		std::vector<UINT8> texture = GenerateTextureData();

		{
			D3D12_SUBRESOURCE_DATA textureData = {};
			textureData.pData = &texture[0];
			textureData.RowPitch = TextureWidth * TexturePixelSize;
			textureData.SlicePitch = textureData.RowPitch * TextureHeight;

			UpdateSubresources(myCommandList.Get(), m_texture.Get(), textureUploadHeap.Get(), 0, 0, 1, &textureData);
			auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				m_texture.Get(),
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			);
			myCommandList->ResourceBarrier(1, &barrier);
		}

		// Describe and create a SRV for the texture.
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = textureDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		const UINT descriptorSize = myDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(mySrvHeap->GetCPUDescriptorHandleForHeapStart());
		for (size_t i = 0; i < SrvCount; i++)
		{
			myDevice->CreateShaderResourceView(m_texture.Get(), &srvDesc, srvHandle);
			srvHandle.Offset(1, descriptorSize);
		}
	}

	// Create the constant buffer.
	{
		const UINT descriptorSize = myDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		const UINT constantBufferSize = sizeof(SceneConstantBuffer);    // CB size is required to be 256-byte aligned.

		ThrowIfFailed(myDevice->CreateCommittedResource(
			&keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)),
			D3D12_HEAP_FLAG_NONE,
			&keep(CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize)),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_constantBuffer)));

		// Describe and create a constant buffer view.
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc[1] = {};
		cbvDesc[0].BufferLocation = m_constantBuffer->GetGPUVirtualAddress();
		cbvDesc[0].SizeInBytes = constantBufferSize;

		//CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(mySrvHeap->GetCPUDescriptorHandleForHeapStart());
		//CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(cbvHandle, CbvCount, descriptorSize);
		//CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle(srvHandle, SrvCount, myDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

		// Create CBVs
		//for (int i = 0; i < CbvCount; i++) {
		//	myDevice->CreateConstantBufferView(&cbvDesc[i], cbvHandle);
		//	cbvHandle.Offset(1, descriptorSize);
		//}

		// Create SRVs
		//for (int i = 0; i < SrvCount; i++) {
		//    myDevice->CreateShaderResourceView(srvResource[i], &srvDesc[i], srvHandle);
		//    srvHandle.Offset(1, myDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
		//}

		// Create UAVs
		//for (int i = 0; i < UavCount; i++) {
		//    myDevice->CreateUnorderedAccessView(uavResource[i], nullptr, &uavDesc[i], uavHandle);
		//    uavHandle.Offset(1, myDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
		//}


		// Map and initialize the constant buffer. We don't unmap this until the
		// app closes. Keeping things mapped for the lifetime of the resource is okay.
		CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pCbvDataBegin)));
		memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));
	}

	// Close the command list and execute it to begin the initial GPU setup.
	ThrowIfFailed(myCommandList->Close());
	ID3D12CommandList* ppCommandLists[] = { myCommandList.Get() };
	myCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Create and record the bundle.
	{
		ThrowIfFailed(myDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, myBundleAllocator.Get(), myPipelineState.Get(), IID_PPV_ARGS(&myBundle)));
		myBundle->SetGraphicsRootSignature(myRootSignature.Get());

		myBundle->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());
		myBundle->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		myBundle->IASetVertexBuffers(0, 1, &myVertexBufferView);
		myBundle->DrawInstanced(3, 1, 0, 0);
		ThrowIfFailed(myBundle->Close());
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
		WaitForGpu();
	}
}

// Generate a simple black and white checkerboard texture.
std::vector<UINT8> D3D12Window::GenerateTextureData()
{
	const UINT rowPitch = TextureWidth * TexturePixelSize;
	const UINT cellPitch = rowPitch >> 3;        // The width of a cell in the checkboard texture.
	const UINT cellHeight = TextureWidth >> 3;    // The height of a cell in the checkerboard texture.
	const UINT textureSize = rowPitch * TextureHeight;

	std::vector<UINT8> data(textureSize);
	UINT8* pData = &data[0];

	for (UINT n = 0; n < textureSize; n += TexturePixelSize)
	{
		UINT x = n % rowPitch;
		UINT y = n / rowPitch;
		UINT i = x / cellPitch;
		UINT j = y / cellHeight;

		if (i % 2 == j % 2)
		{
			pData[n] = 0x00;        // R
			pData[n + 1] = 0x00;    // G
			pData[n + 2] = 0x00;    // B
			pData[n + 3] = 0xff;    // A
		}
		else
		{
			pData[n] = 0xff;        // R
			pData[n + 1] = 0xff;    // G
			pData[n + 2] = 0xff;    // B
			pData[n + 3] = 0xff;    // A
		}
	}

	return data;
}

// Update frame-based values.
void D3D12Window::OnUpdate()
{
	const float translationSpeed = 0.005f;
	const float offsetBounds = 1.f;

	//cameraTransform = DirectX::XMMatrixMultiply(cameraTransform, DirectX::XMMatrixRotationY(3.14f * _timer.GetDeltaTime()));

	m_constantBufferData.projection = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(110.0f), myAspectRatio, 0.1f, 1000000.0f);
	m_constantBufferData.view = DirectX::XMMatrixInverse(nullptr, _cameraTransform);
	m_constantBufferData.farPlane = 1000000.0f;
	m_constantBufferData.nearPlane = 0.1f;
	m_constantBufferData.resolution = DirectX::XMFLOAT2(static_cast<float>(myWidth), static_cast<float>(myHeight));
	m_constantBufferData.viewport = DirectX::XMFLOAT2(static_cast<float>(myWidth), static_cast<float>(myHeight));


	{
		auto* im = InputManager::GetInstance();

		if (im->IsKeyHeld(VK_ESCAPE) && im->IsKeyHeld(VK_SHIFT))
		{
			Quit();
		}

		{
			float xDelta = 0.0f;
			float yDelta = 0.0f;
			if (im->IsKeyHeld(VK_LEFT))
			{
				yDelta += -1.0f * _timer.GetDeltaTime();
			}
			if (im->IsKeyHeld(VK_RIGHT))
			{
				yDelta += 1.0f * _timer.GetDeltaTime();
			}

			if (im->IsKeyHeld(VK_UP))
			{
				xDelta += -1.0f * _timer.GetDeltaTime();
			}
			if (im->IsKeyHeld(VK_DOWN))
			{
				xDelta += 1.0f * _timer.GetDeltaTime();
			}

			_cameraPitch += xDelta;
			_cameraYaw += yDelta;

			_cameraPitch = std::clamp(_cameraPitch, -DirectX::XM_PIDIV2, DirectX::XM_PIDIV2);
			_cameraYaw = std::fmod(_cameraYaw, DirectX::XM_2PI);
		}

		{
			float multiplier = im->IsKeyHeld(VK_SHIFT) ? 10.0f : 1.0f;

			Vector3f moveDirection = {};

			if (im->IsKeyHeld('W'))
			{
				moveDirection.z += 1.0f * _timer.GetDeltaTime();
			}
			if (im->IsKeyHeld('S'))
			{
				moveDirection.z += -1.0f * _timer.GetDeltaTime();
			}
			if (im->IsKeyHeld('A'))
			{
				moveDirection.x += -1.0f * _timer.GetDeltaTime();
			}
			if (im->IsKeyHeld('D'))
			{
				moveDirection.x += 1.0f * _timer.GetDeltaTime();
			}
			if (im->IsKeyHeld('E'))
			{
				moveDirection.y += 1.0f * _timer.GetDeltaTime();
			}
			if (im->IsKeyHeld('Q'))
			{
				moveDirection.y += -1.0f * _timer.GetDeltaTime();
			}

			

			Vector3f right = { _cameraTransform.r[0].m128_f32[0], _cameraTransform.r[0].m128_f32[1], _cameraTransform.r[0].m128_f32[2] };
			Vector3f up = { _cameraTransform.r[1].m128_f32[0], _cameraTransform.r[1].m128_f32[1], _cameraTransform.r[1].m128_f32[2] };
			Vector3f forward = { _cameraTransform.r[2].m128_f32[0], _cameraTransform.r[2].m128_f32[1], _cameraTransform.r[2].m128_f32[2] };

			Vector3f finalMoveDirection = { 0, 0, 0 };

			finalMoveDirection = {
				finalMoveDirection.x + right.x * moveDirection.x * multiplier,
				finalMoveDirection.y + right.y * moveDirection.x * multiplier,
				finalMoveDirection.z + right.z * moveDirection.x * multiplier
			};
			finalMoveDirection = {
				finalMoveDirection.x + up.x * moveDirection.y * multiplier,
				finalMoveDirection.y + up.y * moveDirection.y * multiplier,
				finalMoveDirection.z + up.z * moveDirection.y * multiplier
			};
			finalMoveDirection = {
				finalMoveDirection.x + forward.x * moveDirection.z * multiplier,
				finalMoveDirection.y + forward.y * moveDirection.z * multiplier,
				finalMoveDirection.z + forward.z * moveDirection.z * multiplier
			};

			_cameraPosition = {
				_cameraPosition.x + finalMoveDirection.x,
				_cameraPosition.y + finalMoveDirection.y,
				_cameraPosition.z + finalMoveDirection.z
			};
		}

		_cameraTransform = DirectX::XMMatrixRotationRollPitchYaw(_cameraPitch, _cameraYaw, 0);

		_cameraTransform = DirectX::XMMatrixMultiply(
			_cameraTransform,
			DirectX::XMMatrixTranslation(_cameraPosition.x, _cameraPosition.y, _cameraPosition.z)
		);
	}

	{
		auto S = DirectX::XMMatrixScaling(10.0f, 10.0f, 10.0f);
		//auto R = DirectX::XMMatrixRotationY(std::sin(_timer.GetTotalTime()));
		auto R = DirectX::XMMatrixRotationY(0);
		auto T = DirectX::XMMatrixTranslation(0, 0, 10);
		m_constantBufferData.testTransform = S * R * T;

		m_constantBufferData.offset.x += translationSpeed;
		if (m_constantBufferData.offset.x > offsetBounds)
		{
			m_constantBufferData.offset.x = -offsetBounds;
		}
	}

	memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));
}

// Render the scene.
void D3D12Window::OnRender()
{
	// Record all the commands we need to render the scene into the command list.
	PopulateCommandList();

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { myCommandList.Get() };
	myCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame.
	ThrowIfFailed(mySwapChain->Present(1, 0));

	MoveToNextFrame();
}

void D3D12Window::OnDestroy()
{
	// Ensure that the GPU is no longer referencing resources that are about to be
	// cleaned up by the destructor.
	WaitForGpu();

	CloseHandle(myFenceEvent);
}

void D3D12Window::PopulateCommandList()
{
	// Command list allocators can only be reset when the associated 
	// command lists have finished execution on the GPU; apps should use 
	// fences to determine GPU execution progress.
	ThrowIfFailed(myCommandAllocator[myFrameIndex]->Reset());

	// However, when ExecuteCommandList() is called on a particular command 
	// list, that command list can then be reset at any time and must be before 
	// re-recording.
	ThrowIfFailed(myCommandList->Reset(myCommandAllocator[myFrameIndex].Get(), myPipelineState.Get()));

	// Set necessary state.
	myCommandList->SetGraphicsRootSignature(myRootSignature.Get());

	ID3D12DescriptorHeap* ppHeaps[] = { mySrvHeap.Get() };
	myCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	myCommandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());
	myCommandList->SetGraphicsRootDescriptorTable(1, mySrvHeap->GetGPUDescriptorHandleForHeapStart());
	myCommandList->RSSetViewports(1, &myViewport);
	myCommandList->RSSetScissorRects(1, &myScissorRect);

	// Indicate that the back buffer will be used as a render target.
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			myRenderTargets[myFrameIndex].Get(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
		myCommandList->ResourceBarrier(1, &barrier);
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(myRtvHeap->GetCPUDescriptorHandleForHeapStart(), myFrameIndex, myRtvDescriptorSize);
	myCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// Record commands.
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	myCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// Execute the commands stored in the bundle.
	//myCommandList->ExecuteBundle(myBundle.Get());

	myCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	myCommandList->IASetVertexBuffers(0, 1, &myVertexBufferView);
	myCommandList->DrawInstanced(_tempMeshVertexCount, 1, 0, 0);

	// Indicate that the back buffer will now be used to present.
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			myRenderTargets[myFrameIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT);
		myCommandList->ResourceBarrier(1, &barrier);
	}

	ThrowIfFailed(myCommandList->Close());
}

// Wait for pending GPU work to complete.
void D3D12Window::WaitForGpu()
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
void D3D12Window::MoveToNextFrame()
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
