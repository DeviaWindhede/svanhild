#include "types.hlsli"

cbuffer SceneConstantBuffer : register(b0, space0)
{
    FrameBufferData frameBuffer;
    float g_padding[23]; // Padding so the constant buffer is 256-byte aligned.
};

cbuffer RootConstants : register(b0, space1)
{
    uint NumInstances;
    uint NumCommands;
};

