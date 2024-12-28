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
#include "IWindow.h"

#if USE_IMGUI
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_dx12.h>
#include <imgui/backends/imgui_impl_win32.h>
#include "DX12.h"
#include <InputManager.h>


// Simple free list based allocator
struct ExampleDescriptorHeapAllocator
{
	ID3D12DescriptorHeap* Heap = nullptr;
	D3D12_DESCRIPTOR_HEAP_TYPE  HeapType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
	D3D12_CPU_DESCRIPTOR_HANDLE HeapStartCpu;
	D3D12_GPU_DESCRIPTOR_HANDLE HeapStartGpu;
	UINT                        HeapHandleIncrement;
	ImVector<int>               FreeIndices;

	void Create(ID3D12Device* device, ID3D12DescriptorHeap* heap)
	{
		IM_ASSERT(Heap == nullptr && FreeIndices.empty());
		Heap = heap;
		D3D12_DESCRIPTOR_HEAP_DESC desc = heap->GetDesc();
		HeapType = desc.Type;
		HeapStartCpu = Heap->GetCPUDescriptorHandleForHeapStart();
		HeapStartGpu = Heap->GetGPUDescriptorHandleForHeapStart();
		HeapHandleIncrement = device->GetDescriptorHandleIncrementSize(HeapType);
		FreeIndices.reserve((int)desc.NumDescriptors);
		for (int n = desc.NumDescriptors; n > 0; n--)
			FreeIndices.push_back(n);
	}
	void Destroy()
	{
		Heap = nullptr;
		FreeIndices.clear();
	}
	void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
	{
		IM_ASSERT(FreeIndices.Size > 0);
		int idx = FreeIndices.back();
		FreeIndices.pop_back();
		out_cpu_desc_handle->ptr = HeapStartCpu.ptr + (idx * HeapHandleIncrement);
		out_gpu_desc_handle->ptr = HeapStartGpu.ptr + (idx * HeapHandleIncrement);
	}
	void Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_desc_handle)
	{
		int cpu_idx = (int)((out_cpu_desc_handle.ptr - HeapStartCpu.ptr) / HeapHandleIncrement);
		int gpu_idx = (int)((out_gpu_desc_handle.ptr - HeapStartGpu.ptr) / HeapHandleIncrement);
		IM_ASSERT(cpu_idx == gpu_idx);
		FreeIndices.push_back(cpu_idx);
	}
};

static ID3D12DescriptorHeap* imguiSrvDescHeap = nullptr;
static ExampleDescriptorHeapAllocator imguiSrvDescHeapAlloc;

#endif

using namespace Microsoft::WRL;

std::wstring IWindow::myAssetsPath = L"";
std::wstring IWindow::myEngineShaderPath = L"";
std::wstring IWindow::myCSOPath = L"";

IWindow::IWindow(UINT width, UINT height, std::wstring name) :
    myWidth(width),
    myHeight(height),
    myTitle(name),
    myUseWarpDevice(false)
{
    WCHAR assetsPath[512];
    GetAssetsPath(assetsPath, _countof(assetsPath));
    myAssetsPath = assetsPath;
    myAssetsPath += L"\\assets\\";

    myEngineShaderPath = assetsPath;
    myEngineShaderPath += L"..\\src\\engine\\shader\\";

	myCSOPath = myAssetsPath;
	myCSOPath += L"shaders\\";

    myAspectRatio = static_cast<float>(width) / static_cast<float>(height);
}

IWindow::~IWindow()
{
#if USE_IMGUI
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	if (imguiSrvDescHeap) {
		imguiSrvDescHeap->Release();
		imguiSrvDescHeap = nullptr;
	}
#endif
}

bool IWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (InputManager::GetInstance()->UpdateEvents(message, wParam, lParam))
		return true;

	switch (message)
	{
	case WM_PAINT:
		OnBeginFrame();
		OnUpdate();
		OnRender();
		OnEndFrame();

		InputManager::GetInstance()->Update();
		return true;
	}

	return false;
}

void IWindow::ImGui_Init(DX12&
#if USE_IMGUI
	aDx12
#endif
)
{
#if USE_IMGUI
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = APP_SRV_HEAP_SIZE;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		if (aDx12.myDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&imguiSrvDescHeap)) != S_OK)
			return;
		imguiSrvDescHeapAlloc.Create(aDx12.myDevice.Get(), imguiSrvDescHeap);
	}

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends

	//ImGui_ImplDX12_Init()

	ImGui_ImplWin32_Init(Win32Application::GetHwnd());

	ImGui_ImplDX12_InitInfo init_info = {};
	init_info.Device = aDx12.myDevice.Get();
	init_info.CommandQueue = aDx12.myCommandQueue.Get();
	init_info.NumFramesInFlight = DX12::FrameCount;
	init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
	// Allocating SRV descriptors (for textures) is up to the application, so we provide callbacks.
	// (current version of the backend will only allocate one descriptor, future versions will need to allocate more)
	init_info.SrvDescriptorHeap = imguiSrvDescHeap;

	init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) { return imguiSrvDescHeapAlloc.Alloc(out_cpu_handle, out_gpu_handle); };
	init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) { return imguiSrvDescHeapAlloc.Free(cpu_handle, gpu_handle); };
	ImGui_ImplDX12_Init(&init_info);
#else
	__noop;
#endif
}

void IWindow::ImGui_EndFrame(DX12&
#if USE_IMGUI
	aDx12
#endif
)
{
#if USE_IMGUI
	// Bind ImGui's descriptor heap
	aDx12.myCommandList->SetDescriptorHeaps(1, &imguiSrvDescHeap);

	// Render ImGui
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// Add ImGui elements
	ImGui::Begin("Hello, ImGui!");
	ImGui::Text("This is ImGui with its own descriptor heap.");
	ImGui::End();

	// Render ImGui draw data
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), aDx12.myCommandList.Get());
#else
	__noop;
#endif
}

// Helper function for setting the window's title text.
void IWindow::SetCustomWindowText(LPCWSTR text)
{
    std::wstring windowText = myTitle + L": " + text;
    SetWindowText(Win32Application::GetHwnd(), windowText.c_str());
}

// Helper function for parsing any supplied command line args.
_Use_decl_annotations_
void IWindow::ParseCommandLineArgs(WCHAR* argv[], int argc)
{
    for (int i = 1; i < argc; ++i)
    {
        if (_wcsnicmp(argv[i], L"-warp", wcslen(argv[i])) == 0 || 
            _wcsnicmp(argv[i], L"/warp", wcslen(argv[i])) == 0)
        {
            myUseWarpDevice = true;
            myTitle = myTitle + L" (WARP)";
        }
    }
}
