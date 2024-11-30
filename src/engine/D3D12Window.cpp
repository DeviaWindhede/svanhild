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
	camera(),
	resourceLoader(dx12)
{
}

void D3D12Window::OnInit()
{
	InputManager::CreateInstance();

	dx12.LoadPipeline();
	LoadAssets();
}

void D3D12Window::OnBeginFrame()
{
	resourceLoader.Update();
	UpdateFrameBuffer();


}

#include "mesh/ModelFactory.h"
#include "StringHelper.h"

// Load the sample assets.
void D3D12Window::LoadAssets()
{

	// Frame Buffer
	{
		const UINT descriptorSize = dx12.myDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		const UINT constantBufferSize = sizeof(FrameBuffer);    // CB size is required to be 256-byte aligned.

		ThrowIfFailed(dx12.myDevice->CreateCommittedResource(
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

	// Create and record the bundle.
	{
		ThrowIfFailed(dx12.myDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, dx12.myBundleAllocator.Get(), dx12.myPipelineState.Get(), IID_PPV_ARGS(&dx12.myBundle)));
		dx12.myBundle->SetGraphicsRootSignature(dx12.myRootSignature.Get());

		dx12.myBundle->SetGraphicsRootConstantBufferView(0, frameBuffer->GetGPUVirtualAddress());
		dx12.myBundle->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		dx12.myBundle->IASetVertexBuffers(0, 1, &myTempMesh.VertexBufferView());
		dx12.myBundle->IASetIndexBuffer(&myTempMesh.IndexBufferView());
		dx12.myBundle->DrawIndexedInstanced(myTempMesh.VertexCount(), 1, 0, 0, 0);
		ThrowIfFailed(dx12.myBundle->Close());
	}
}

// Update frame-based values.
void D3D12Window::OnUpdate()
{
	if (!myTempMesh.GPUInitialized())
	{
		auto package = ModelFactory::LoadMeshFromFBX(StringHelper::ws2s(GetAssetFullPath(L"sm_oneTrueCube.fbx")));

		myTempMesh.LoadMeshData(package.meshData[0].vertices, package.meshData[0].indices);
		resourceLoader.LoadResource(&myTempMesh);
		resourceLoader.LoadResource(&myTempTexture);
	}

	const float translationSpeed = 0.005f;
	const float offsetBounds = 1.f;


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

			camera.pitch += xDelta;
			camera.yaw += yDelta;

			camera.pitch = std::clamp(camera.pitch, -DirectX::XM_PIDIV2, DirectX::XM_PIDIV2);
			camera.yaw = std::fmod(camera.yaw, DirectX::XM_2PI);
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

			

			Vector3f right = camera.Right();
			Vector3f up = camera.Up();
			Vector3f forward = camera.Forward();

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

			camera.position = {
				camera.position.x + finalMoveDirection.x,
				camera.position.y + finalMoveDirection.y,
				camera.position.z + finalMoveDirection.z
			};
		}
	}

	{
		auto S = DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f);
		//auto R = DirectX::XMMatrixRotationY(std::sin(_timer.GetTotalTime()));
		auto R = DirectX::XMMatrixRotationY(0);
		auto T = DirectX::XMMatrixTranslation(0, 0, 10);
		frameBufferData.testTransform = S * R * T;

		frameBufferData.offset.x += translationSpeed;
		if (frameBufferData.offset.x > offsetBounds)
		{
			frameBufferData.offset.x = -offsetBounds;
		}
	}



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

	dx12.myCommandList->SetGraphicsRootConstantBufferView(0, frameBuffer->GetGPUVirtualAddress());

	myTempTexture.Bind(dx12);

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
	if (myTempMesh.GPUInitialized())
	{
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

void D3D12Window::UpdateFrameBuffer()
{
	frameBufferData.projection = camera.Projection();
	frameBufferData.view = camera.View();
	frameBufferData.nearPlane = camera.nearPlane;
	frameBufferData.farPlane = camera.farPlane;
	frameBufferData.windowSize = DirectX::XMFLOAT2(static_cast<float>(myWidth), static_cast<float>(myHeight));
	frameBufferData.viewport = DirectX::XMFLOAT2(static_cast<float>(dx12.myViewport.Width), static_cast<float>(dx12.myViewport.Height));

	memcpy(frameBufferCbvDataBegin, &frameBufferData, sizeof(frameBufferData));
}
