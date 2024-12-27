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

#include <string>
#include "DXHelper.h"
#include "Win32Application.h"
#include "ApplicationTimer.h"

#define USE_IMGUI 1

class IWindow
{
private:
    // Root assets path
    static std::wstring myAssetsPath;

    // Root shader path
    static std::wstring myEngineShaderPath;

public:
    IWindow(UINT width, UINT height, std::wstring name);
    virtual ~IWindow();

    virtual void OnInit() { __noop; }
    virtual void OnUpdate() { __noop; }
    virtual void OnRender() { __noop; }
    virtual void OnBeginFrame() { __noop; }
    virtual void OnEndFrame() { _timer.Update(); }
    virtual void OnDestroy() { __noop; }

    // Samples override the event handlers to handle specific messages
    virtual void OnKeyDown(UINT8 /*key*/)   { __noop; }
    virtual void OnKeyUp(UINT8 /*key*/)     { __noop; }

    // Accessors.
    UINT GetWidth() const           { return myWidth; }
    UINT GetHeight() const          { return myHeight; }
    const WCHAR* GetTitle() const   { return myTitle.c_str(); }

    void ParseCommandLineArgs(_In_reads_(argc) WCHAR* argv[], int argc);

    static std::wstring GetAssetFullPath(LPCWSTR assetName) { return myAssetsPath + assetName; }
    static std::wstring GetEngineShaderFullPath(LPCWSTR assetName) { return myEngineShaderPath + assetName; }
protected:
#if USE_IMGUI
    static const int APP_SRV_HEAP_SIZE = 64;
#endif

    void ImGui_Init(class DX12& aDx12);
    void ImGui_EndFrame(class DX12& aDx12);

    void SetCustomWindowText(LPCWSTR text);

    ApplicationTimer _timer;

    // Viewport dimensions
    UINT myWidth;
    UINT myHeight;
    float myAspectRatio;

    // Adapter info
    bool myUseWarpDevice;

private:
    // Window title
    std::wstring myTitle;
};
