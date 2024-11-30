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
	dx12(width, height, myUseWarpDevice),
	_cameraPitch(0.0f),
	_cameraYaw(0.0f),
	_cameraPosition{ 0.0f, 0.0f, 0.0f }
{
	_cameraTransform = DirectX::XMMatrixTranslation(0.0f, 0.0f, -10.0f);
}

void D3D12Window::OnInit()
{
	InputManager::CreateInstance();

	dx12.LoadPipeline();
	LoadAssets();
}

#include "mesh/ModelFactory.h"
#include "StringHelper.h"

// Load the sample assets.
void D3D12Window::LoadAssets()
{
	// Create and record the bundle.
	{
		ThrowIfFailed(dx12.myDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, dx12.myBundleAllocator.Get(), dx12.myPipelineState.Get(), IID_PPV_ARGS(&dx12.myBundle)));
		dx12.myBundle->SetGraphicsRootSignature(dx12.myRootSignature.Get());

		dx12.myBundle->SetGraphicsRootConstantBufferView(0, dx12.frameBuffer->GetGPUVirtualAddress());
		dx12.myBundle->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		dx12.myBundle->IASetVertexBuffers(0, 1, &myTempMesh.VertexBufferView());
		dx12.myBundle->IASetIndexBuffer(&myTempMesh.IndexBufferView());
		dx12.myBundle->DrawIndexedInstanced(myTempMesh.VertexCount(), 1, 0, 0, 0);
		ThrowIfFailed(dx12.myBundle->Close());
	}
}

#include "Mesh.h"
void D3D12Window::LoadMesh(Mesh& aMesh)
{
	ThrowIfFailed(dx12.myCommandAllocator[dx12.myFrameIndex]->Reset());
	ThrowIfFailed(dx12.myCommandList->Reset(dx12.myCommandAllocator[dx12.myFrameIndex].Get(), dx12.myPipelineState.Get()));
	
	aMesh.InitUploadBufferTransfer(dx12.myDevice, dx12.myCommandList);

	ThrowIfFailed(dx12.myCommandList->Close());

	// Execute the command list
	ID3D12CommandList* commandLists[] = { dx12.myCommandList.Get() };
	dx12.myCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	dx12.WaitForGPU();
}

void D3D12Window::LoadTexture()
{
	ThrowIfFailed(dx12.myCommandAllocator[dx12.myFrameIndex]->Reset());
	ThrowIfFailed(dx12.myCommandList->Reset(dx12.myCommandAllocator[dx12.myFrameIndex].Get(), dx12.myPipelineState.Get()));

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
			ThrowIfFailed(dx12.myDevice->CreateCommittedResource(
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
			ThrowIfFailed(dx12.myDevice->CreateCommittedResource(
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

			UpdateSubresources(dx12.myCommandList.Get(), m_texture.Get(), textureUploadHeap.Get(), 0, 0, 1, &textureData);
			auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				m_texture.Get(),
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			);
			dx12.myCommandList->ResourceBarrier(1, &barrier);
		}

		// Describe and create a SRV for the texture.
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = textureDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		const UINT descriptorSize = dx12.myDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(dx12.mySrvHeap->GetCPUDescriptorHandleForHeapStart());
		for (size_t i = 0; i < dx12.SrvCount; i++)
		{
			dx12.myDevice->CreateShaderResourceView(m_texture.Get(), &srvDesc, srvHandle);
			srvHandle.Offset(1, descriptorSize);
		}
	}

	ThrowIfFailed(dx12.myCommandList->Close());

	// Execute the command list
	ID3D12CommandList* commandLists[] = { dx12.myCommandList.Get() };
	dx12.myCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	dx12.WaitForGPU();
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
	if (!myTempMesh.InitializedBuffer())
	{
		auto package = ModelFactory::LoadMeshFromFBX(StringHelper::ws2s(GetAssetFullPath(L"sm_oneTrueCube.fbx")));

		myTempMesh.LoadMeshData(package.meshData[0].vertices, package.meshData[0].indices);
		LoadMesh(myTempMesh);

		LoadTexture();
	}

	
	const float translationSpeed = 0.005f;
	const float offsetBounds = 1.f;


	//cameraTransform = DirectX::XMMatrixMultiply(cameraTransform, DirectX::XMMatrixRotationY(3.14f * _timer.GetDeltaTime()));

	dx12.frameBufferData.projection = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(110.0f), myAspectRatio, 0.1f, 1000000.0f);
	dx12.frameBufferData.view = DirectX::XMMatrixInverse(nullptr, _cameraTransform);
	dx12.frameBufferData.farPlane = 1000000.0f;
	dx12.frameBufferData.nearPlane = 0.1f;
	dx12.frameBufferData.resolution = DirectX::XMFLOAT2(static_cast<float>(myWidth), static_cast<float>(myHeight));
	dx12.frameBufferData.viewport = DirectX::XMFLOAT2(static_cast<float>(dx12.myViewport.Width), static_cast<float>(dx12.myViewport.Height));

	
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
		dx12.frameBufferData.testTransform = S * R * T;

		dx12.frameBufferData.offset.x += translationSpeed;
		if (dx12.frameBufferData.offset.x > offsetBounds)
		{
			dx12.frameBufferData.offset.x = -offsetBounds;
		}
	}

	memcpy(dx12.frameBufferCbvDataBegin, &dx12.frameBufferData, sizeof(dx12.frameBufferData));
}

// Render the scene.
void D3D12Window::OnRender()
{
	// Record all the commands we need to render the scene into the command list.
	PopulateCommandList();

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { dx12.myCommandList.Get() };
	dx12.myCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame.
	ThrowIfFailed(dx12.mySwapChain->Present(1, 0));

	dx12.MoveToNextFrame();
}

void D3D12Window::OnDestroy()
{
	// Ensure that the GPU is no longer referencing resources that are about to be
	// cleaned up by the destructor.
	dx12.WaitForGPU();

	CloseHandle(dx12.myFenceEvent);

	InputManager::DestroyInstance();
}

void D3D12Window::PopulateCommandList()
{
	// Command list allocators can only be reset when the associated 
	// command lists have finished execution on the GPU; apps should use 
	// fences to determine GPU execution progress.
	ThrowIfFailed(dx12.myCommandAllocator[dx12.myFrameIndex]->Reset());

	// However, when ExecuteCommandList() is called on a particular command 
	// list, that command list can then be reset at any time and must be before 
	// re-recording.
	ThrowIfFailed(dx12.myCommandList->Reset(dx12.myCommandAllocator[dx12.myFrameIndex].Get(), dx12.myPipelineState.Get()));

	// Set necessary state.
	dx12.myCommandList->SetGraphicsRootSignature(dx12.myRootSignature.Get());

	ID3D12DescriptorHeap* ppHeaps[] = { dx12.mySrvHeap.Get() };
	dx12.myCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	dx12.myCommandList->SetGraphicsRootConstantBufferView(0, dx12.frameBuffer->GetGPUVirtualAddress());
	dx12.myCommandList->SetGraphicsRootDescriptorTable(1, dx12.mySrvHeap->GetGPUDescriptorHandleForHeapStart());
	dx12.myCommandList->RSSetViewports(1, &dx12.myViewport);
	dx12.myCommandList->RSSetScissorRects(1, &dx12.myScissorRect);

	// Indicate that the back buffer will be used as a render target.
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			dx12.myRenderTargets[dx12.myFrameIndex].Get(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET);

		dx12.myCommandList->ResourceBarrier(1, &barrier);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dx12.myDsvHeap->GetCPUDescriptorHandleForHeapStart();
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(dx12.myRtvHeap->GetCPUDescriptorHandleForHeapStart(), dx12.myFrameIndex, dx12.myRtvDescriptorSize);
	dx12.myCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	dx12.myCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, NULL);

	// Record commands.
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	dx12.myCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// Execute the commands stored in the bundle.
	//dx12.myCommandList->ExecuteBundle(dx12.myBundle.Get());

	dx12.myCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	dx12.myCommandList->IASetVertexBuffers(0, 1, &myTempMesh.VertexBufferView());
	dx12.myCommandList->IASetIndexBuffer(&myTempMesh.IndexBufferView());
	//dx12.myCommandList->DrawInstanced(myTempMesh.VertexCount(), 1, 0, 0);

	{
		size_t vertexCount = myTempMesh.VertexCount();    // Number of vertices
		size_t indexCount = myTempMesh.IndexCount();     // Number of indices

		// StartIndexLocation is the number of vertices, since the index buffer starts right after the vertex buffer
		UINT startIndexLocation = static_cast<UINT>(vertexCount);

		//dx12.myCommandList->DrawIndexedInstanced(
		//	indexCount,             // Number of indices
		//	1,                      // Number of instances
		//	0,                      // Start vertex location (0 for starting at the beginning of the vertex buffer)
		//	startIndexLocation,     // Start index location (the index from where to start drawing)
		//	0                       // Start instance location (0 for no instance offset)
		//);
		dx12.myCommandList->DrawIndexedInstanced(
			indexCount,             // Number of indices
			1,                      // Number of instances
			0,                      // Start vertex location (typically 0)
			0,                      // Start index location (typically 0)
			0                       // Start instance location (typically 0)
		);
	}

	// Indicate that the back buffer will now be used to present.
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			dx12.myRenderTargets[dx12.myFrameIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT);
		dx12.myCommandList->ResourceBarrier(1, &barrier);
	}

	ThrowIfFailed(dx12.myCommandList->Close());
}
