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

	// Init Frame Buffer
	{
		const UINT descriptorSize = dx12.myDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		const UINT constantBufferSize = sizeof(FrameBufferData);    // CB size is required to be 256-byte aligned.

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
}

void D3D12Window::OnBeginFrame()
{
	resourceLoader.Update();
	UpdateFrameBuffer();


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

void D3D12Window::OnEndFrame()
{
	// Indicate that the back buffer will now be used to present.
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			dx12.myRenderTargets[dx12.myFrameIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT);
		dx12.myCommandList->ResourceBarrier(1, &barrier);
	}

	ThrowIfFailed(dx12.myCommandList->Close());

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { dx12.myCommandList.Get() };
	dx12.myCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame.
	ThrowIfFailed(dx12.mySwapChain->Present(1, 0));

	dx12.MoveToNextFrame();

	IWindow::OnEndFrame();
}

void D3D12Window::OnDestroy()
{
	// Ensure that the GPU is no longer referencing resources that are about to be
	// cleaned up by the destructor.
	dx12.WaitForGPU();

	CloseHandle(dx12.myFenceEvent);

	InputManager::DestroyInstance();
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
