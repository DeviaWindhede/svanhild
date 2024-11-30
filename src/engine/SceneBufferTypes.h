#pragma once
#include "DirectXMath.h"

struct FrameBuffer
{
    DirectX::XMMATRIX view;
    DirectX::XMMATRIX projection;
    DirectX::XMFLOAT2 windowSize;
    DirectX::XMFLOAT2 viewport;
    float nearPlane;
    float farPlane;
    float padding0[2];
    DirectX::XMMATRIX testTransform;
    DirectX::XMFLOAT4 offset;
    float padding[4]; // Padding so the constant buffer is 256-byte aligned.
};
static_assert((sizeof(FrameBuffer) % 256) == 0, "Constant Buffer size must be 256-byte aligned");
