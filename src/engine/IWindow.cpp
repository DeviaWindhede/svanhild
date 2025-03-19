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

#include "InputManager.h"


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
}

bool IWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (InputManager::GetInstance()->UpdateEvents(message, wParam, lParam))
		return true;

	//switch (message)
	//{
	//case WM_PAINT:
	//	OnBeginFrame();
	//	OnUpdate();
	//	OnRender();
	//	OnEndFrame();

	//	InputManager::GetInstance()->Update();
	//	return true;
	//}

	return false;
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
