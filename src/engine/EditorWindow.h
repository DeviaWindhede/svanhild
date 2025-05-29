#pragma once
#include "ClassHelper.h"

#define USE_IMGUI 1

class EditorWindow
{
public:
    EditorWindow() = default;
    DISALLOW_COPY_AND_ASSIGN(EditorWindow);
    
#if !USE_IMGUI
    ~EditorWindow() = default;
#endif
    void Init(HWND hWnd, class DX12* aDx12, class IWindow* aWindow);
    void EndFrame();
#if USE_IMGUI
    ~EditorWindow();

protected:
    void Render();
    static const int IMGUI_SRV_HEAP_SIZE = 64;

    class DX12* dx12 = nullptr;
    class IWindow* window = nullptr;
#endif
};
