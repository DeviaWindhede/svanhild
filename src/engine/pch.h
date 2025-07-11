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

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently.

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers.
#endif

#define _USE_MATH_DEFINES

#include <windows.h>

#include <directx/d3dx12.h> // https://github.com/microsoft/DirectX-Headers/tree/main
#include <d3d12.h>
#include <dxgi1_6.h>
#include <tchar.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>

#ifdef _DEBUG
#define DX12_ENABLE_DEBUG_LAYER
#endif

#ifdef DX12_ENABLE_DEBUG_LAYER
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

#include <string>
#include <wrl.h>
#include <shellapi.h>

#include <vector>
#include <stddef.h>
#include <math.h>
#include <cmath>
#include "math/MathDefines.h"
#include "util/ClassHelper.h"

