#pragma once
#include <SceneBufferTypes.h>

using Microsoft::WRL::ComPtr;

class FrameBuffer
{
public:
    void Init(class DX12& aDx12);
    void Update(class DX12& aDx12, class Camera& aCamera);

    FrameBufferData frameBufferData = {};
    ComPtr<ID3D12Resource> resource;
    UINT8* frameBufferCbvDataBegin = 0;
};
