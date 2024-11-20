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

#include "DXHelper.h"
#include "Win32Application.h"
#include "ApplicationTimer.h"

class IWindow
{
public:
    IWindow(UINT width, UINT height, std::wstring name);
    virtual ~IWindow();

    virtual void OnInit() = 0;
    virtual void OnUpdate() = 0;
    virtual void OnRender() = 0;
    virtual void OnBeginFrame() { __noop; }
    virtual void OnEndFrame() { _timer.Update(); }
    virtual void OnDestroy() = 0;

    // Samples override the event handlers to handle specific messages
    virtual void OnKeyDown(UINT8 /*key*/)   { __noop; }
    virtual void OnKeyUp(UINT8 /*key*/)     { __noop; }

    // Accessors.
    UINT GetWidth() const           { return myWidth; }
    UINT GetHeight() const          { return myHeight; }
    const WCHAR* GetTitle() const   { return myTitle.c_str(); }

    void ParseCommandLineArgs(_In_reads_(argc) WCHAR* argv[], int argc);

protected:
    std::wstring GetAssetFullPath(LPCWSTR assetName) const;
    std::wstring GetEngineShaderFullPath(LPCWSTR assetName) const;

    void GetHardwareAdapter(
        _In_ IDXGIFactory1* pFactory,
        _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter,
        bool requestHighPerformanceAdapter = false);

    void SetCustomWindowText(LPCWSTR text);

    ApplicationTimer _timer;

    // Viewport dimensions
    UINT myWidth;
    UINT myHeight;
    float myAspectRatio;

    // Adapter info
    bool myUseWarpDevice;

private:
    // Root assets path
    std::wstring myAssetsPath;

    // Root shader path
    std::wstring myEngineShaderPath;

    // Window title
    std::wstring myTitle;
};
