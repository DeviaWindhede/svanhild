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
#include <vector>
#include "UploadBuffer.h"
#include "DX12.h"
#include "Mesh.h"
#include "ResourceLoader.h"
#include <Camera.h>
#include <Texture.h>

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
// An example of this can be found in the class method: OnDestroy().
using Microsoft::WRL::ComPtr;

class D3D12Window : public IWindow
{
public:
    D3D12Window(UINT width, UINT height, std::wstring name);

    virtual void OnInit() override;
    virtual void OnBeginFrame() override;
    virtual void OnEndFrame() override;
    virtual void OnDestroy() override;

    __forceinline void Quit() { PostQuitMessage(0); };
protected:
    DX12 dx12;
    FrameBufferData frameBufferData = {};
    Camera camera;
    ResourceLoader resourceLoader;

    ComPtr<ID3D12Resource> frameBuffer;
    UINT8* frameBufferCbvDataBegin = 0;

    void UpdateFrameBuffer();
};
