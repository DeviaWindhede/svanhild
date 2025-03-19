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

class IWindow
{
private:
    static std::wstring myAssetsPath;
    static std::wstring myCSOPath;
    static std::wstring myEngineShaderPath;

public:
    IWindow(UINT width, UINT height, std::wstring name);
    virtual ~IWindow();

    virtual bool WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
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
    const ApplicationTimer& GetTimer() const { return _timer; }
    
    void ParseCommandLineArgs(_In_reads_(argc) WCHAR* argv[], int argc);

    static std::wstring GetAssetFullPath(const std::wstring& assetName) { return myAssetsPath + assetName; }
    static std::wstring GetEngineShaderFullPath(const std::wstring& assetName) { return myEngineShaderPath + assetName; }
    static std::wstring GetCSOPath(const std::wstring& assetName) { return myCSOPath + assetName; }
protected:
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
