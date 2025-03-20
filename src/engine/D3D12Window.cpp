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
#include <ShaderCompiler.h>

#if USE_IMGUI
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_win32.cpp>
#endif

D3D12Window::D3D12Window(UINT width, UINT height, std::wstring name) :
	IWindow(width, height, name),
	dx12(width, height, myUseWarpDevice),
	camera(),
	resourceLoader(&dx12)
{
}

D3D12Window::~D3D12Window()
{
}

bool D3D12Window::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
#if USE_IMGUI
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;
#endif
	switch (message)
	{
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	}

	return IWindow::WndProc(hWnd, message, wParam, lParam);
}

void D3D12Window::OnInit()
{
	InputManager::CreateInstance();

	dx12.LoadPipeline();
	editorWindow.Init(Win32Application::GetHwnd(), &dx12, this);
}

void D3D12Window::OnUpdate()
{
}

void D3D12Window::OnBeginFrame()
{
	dx12.frameBuffer.Update(dx12, camera, _timer);
	resourceLoader.BeginFrame();

	dx12.PrepareRender();
	
	dx12.myCommandList->IASetVertexBuffers(0, 1, &resourceLoader.GetBuffers().vertexBuffer.vbv);
	dx12.myCommandList->IASetIndexBuffer(&resourceLoader.GetBuffers().indexBuffer.ibv);
	dx12.myCommandList->IASetVertexBuffers(1, 1, &dx12.instanceBuffer.instanceBufferView);
	ShaderCompiler::GetPSO(dx12.currentPSO).Set(dx12);
}

void D3D12Window::OnEndFrame()
{
	dx12.ExecuteRender();
	editorWindow.EndFrame();
	dx12.EndRender();

	IWindow::OnEndFrame();
}

void D3D12Window::OnDestroy()
{
	dx12.Cleanup();
	InputManager::DestroyInstance();
}
