#include "pch.h"
#include "EditorWindow.h"

#include "rendering/DX12.h"

#if USE_IMGUI
#include "IWindow.h"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_dx12.h>
#include <imgui/backends/imgui_impl_win32.h>

bool IsAABBVisible(DX12* dx12, DirectX::XMMATRIX transform, Vector3f minBounds, Vector3f maxBounds)
{
    DirectX::XMVECTOR aabbCorners[8] = {
        DirectX::XMVECTOR{minBounds.x, minBounds.y, minBounds.z, 1.0f},
        DirectX::XMVECTOR{maxBounds.x, minBounds.y, minBounds.z, 1.0f},
        DirectX::XMVECTOR{minBounds.x, maxBounds.y, minBounds.z, 1.0f},
        DirectX::XMVECTOR{maxBounds.x, maxBounds.y, minBounds.z, 1.0f},
        DirectX::XMVECTOR{minBounds.x, minBounds.y, maxBounds.z, 1.0f},
        DirectX::XMVECTOR{maxBounds.x, minBounds.y, maxBounds.z, 1.0f},
        DirectX::XMVECTOR{minBounds.x, maxBounds.y, maxBounds.z, 1.0f},
        DirectX::XMVECTOR{maxBounds.x, maxBounds.y, maxBounds.z, 1.0f}
    };

    for (int i = 0; i < 8; ++i)
    {
        auto vertexObjectPosition = DirectX::XMVector4Transform({
            aabbCorners[i].m128_f32[0],
            aabbCorners[i].m128_f32[1],
            aabbCorners[i].m128_f32[2],
            1
        }, transform);
        auto vertexViewPosition = DirectX::XMVector4Transform(vertexObjectPosition, dx12->frameBuffer.data->data.view);
        auto vertexProjectionPosition = DirectX::XMVector4Transform(vertexViewPosition, dx12->frameBuffer.data->data.projection);
        DirectX::XMVECTOR ndc = {
            vertexProjectionPosition.m128_f32[0] / vertexProjectionPosition.m128_f32[3],
            vertexProjectionPosition.m128_f32[1] / vertexProjectionPosition.m128_f32[3],
            vertexProjectionPosition.m128_f32[2] / vertexProjectionPosition.m128_f32[3],
            1.0f
        };
        bool isVisibleX = ndc.m128_f32[0] >= -1.0f && ndc.m128_f32[0] <= 1.0f;
        bool isVisibleY = ndc.m128_f32[1] >= -1.0f && ndc.m128_f32[2] <= 1.0f;
        bool isVisibleZ = ndc.m128_f32[2] >= 0 && ndc.m128_f32[2] <= 1.0f;

        if (isVisibleX && isVisibleY && isVisibleZ)
            return true;
    }

    return false;
}

void EditorWindow::Render()
{
    ImGui::Begin("Data");
    ImGui::Text(
        ("FPS: " + std::to_string((int)(1.0f / window->GetTimer().GetDeltaTime())) + ", " + std::to_string(
            window->GetTimer().GetDeltaTime()) + "ms").c_str());


    {
        auto transform = dx12->instanceBuffer.cpuData[0].transform;

        DirectX::XMMATRIX t = {
            transform.data.f[0], transform.data.f[1], transform.data.f[2], 0,
            transform.data.f[4], transform.data.f[5], transform.data.f[6], 0,
            transform.data.f[8], transform.data.f[9], transform.data.f[10], 0,
            transform.data.f[3], transform.data.f[7], transform.data.f[11], 1
        };
        
        IsAABBVisible(dx12, t, dx12->instanceBuffer.instanceCountBuffer.cpuData[0].aabb.min, dx12->instanceBuffer.instanceCountBuffer.cpuData[0].aabb.max);
    }


    
    ImGui::End();
}

// Simple free list based allocator
struct ExampleDescriptorHeapAllocator
{
    ID3D12DescriptorHeap* Heap = nullptr;
    D3D12_DESCRIPTOR_HEAP_TYPE HeapType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
    D3D12_CPU_DESCRIPTOR_HANDLE HeapStartCpu;
    D3D12_GPU_DESCRIPTOR_HANDLE HeapStartGpu;
    UINT HeapHandleIncrement;
    ImVector<int> FreeIndices;

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

EditorWindow::~EditorWindow()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (imguiSrvDescHeap)
    {
        imguiSrvDescHeap->Release();
        imguiSrvDescHeap = nullptr;
    }
}
#endif

void EditorWindow::Init(HWND hWnd, class DX12* aDx12, class IWindow* aWindow)
{
#if !USE_IMGUI
	hWnd;
	aDx12;
	aWindow;
#else
    dx12 = aDx12;
    window = aWindow;

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = IMGUI_SRV_HEAP_SIZE;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (dx12->myDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&imguiSrvDescHeap)) != S_OK)
            return;
        imguiSrvDescHeapAlloc.Create(dx12->myDevice.Get(), imguiSrvDescHeap);
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends

    //ImGui_ImplDX12_Init()

    ImGui_ImplWin32_Init(hWnd);

    ImGui_ImplDX12_InitInfo init_info = {};
    init_info.Device = dx12->myDevice.Get();
    init_info.CommandQueue = dx12->myCommandQueue.Get();
    init_info.NumFramesInFlight = RenderConstants::FrameCount;
    init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
    // Allocating SRV descriptors (for textures) is up to the application, so we provide callbacks.
    // (current version of the backend will only allocate one descriptor, future versions will need to allocate more)
    init_info.SrvDescriptorHeap = imguiSrvDescHeap;

    init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle,
                                        D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle)
    {
        return imguiSrvDescHeapAlloc.Alloc(out_cpu_handle, out_gpu_handle);
    };
    init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle,
                                       D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle)
    {
        return imguiSrvDescHeapAlloc.Free(cpu_handle, gpu_handle);
    };
    ImGui_ImplDX12_Init(&init_info);
#endif
}

void EditorWindow::EndFrame()
{
#if !USE_IMGUI
	__noop;
#else
    dx12->WaitForGPU();

    ThrowIfFailed(dx12->myCommandAllocator[dx12->myFrameIndex]->Reset());
    ThrowIfFailed(dx12->myCommandList->Reset(dx12->myCommandAllocator[dx12->myFrameIndex].Get(), nullptr));

    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            dx12->myRenderTargets[dx12->myFrameIndex].Get(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET);

        dx12->myCommandList->ResourceBarrier(1, &barrier);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(dx12->myRtvHeap->GetCPUDescriptorHandleForHeapStart(),
                                                dx12->myFrameIndex,
                                                dx12->myRtvDescriptorSize);
        dx12->myCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
    }

    // Bind ImGui's descriptor heap
    dx12->myCommandList->SetDescriptorHeaps(1, &imguiSrvDescHeap);

    // Render ImGui
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    Render();

    // Render ImGui draw data
    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dx12->myCommandList.Get());

    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            dx12->myRenderTargets[dx12->myFrameIndex].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT);

        dx12->myCommandList->ResourceBarrier(1, &barrier);

        ThrowIfFailed(dx12->myCommandList->Close());

        // Execute the command list.
        ID3D12CommandList* ppCommandLists[] = {dx12->myCommandList.Get()};
        dx12->myCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
    }
#endif
}
