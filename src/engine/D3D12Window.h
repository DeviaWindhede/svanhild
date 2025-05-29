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

#pragma once

#include "IWindow.h"
#include "DX12.h"
#include "ResourceLoader.h"
#include <Camera.h>
#include <FrameBuffer.h>

#include "EditorWindow.h"
#include "IndexBuffer.h"
#include "VertexBuffer.h"

using Microsoft::WRL::ComPtr;

class D3D12Window : public IWindow
{
public:
    D3D12Window(UINT width, UINT height, std::wstring name);
    ~D3D12Window() override;

    virtual bool WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override;
    virtual void OnInit() override;
    virtual void OnUpdate() override;
    virtual void OnBeginFrame() override;
    virtual void OnEndFrame() override;
    virtual void OnDestroy() override;

    __forceinline void Quit() { PostQuitMessage(0); };
protected:
    EditorWindow editorWindow;
    
    DX12 dx12;
    Camera camera;
    ResourceLoader resourceLoader;

    int maxFrameRate = 0;
    
    void UpdateFrameBuffer();
};
