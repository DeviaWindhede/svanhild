#pragma once

#define USE_IMGUI 0

class EditorWindow
{
public:
    void Init(HWND hWnd, class DX12* aDx12, class IWindow* aWindow);
    void EndFrame();
#if USE_IMGUI
    ~EditorWindow();
protected:
    void Render();
    static const int APP_NUM_FRAMES_IN_FLIGHT = 3;
    static const int APP_NUM_BACK_BUFFERS = 3;
    static const int APP_SRV_HEAP_SIZE = 64;

    class DX12* dx12;
    class IWindow* window;
#endif
};
