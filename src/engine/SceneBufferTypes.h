#pragma once
#include "DirectXMath.h"

struct FrameBufferData
{
    DirectX::XMMATRIX view;
    DirectX::XMMATRIX projection;
    DirectX::XMFLOAT2 viewport;
    float nearPlane;
    float farPlane;
    float padding0[4];
    DirectX::XMMATRIX testTransform;
    DirectX::XMFLOAT4 offset;
    float time;
    float padding[3]; // Padding so the constant buffer is 256-byte aligned.
};
static_assert((sizeof(FrameBufferData) % 256) == 0, "Constant Buffer size must be 256-byte aligned");
