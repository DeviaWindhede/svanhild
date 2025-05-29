#include "types.hlsli"

cbuffer SceneConstantBuffer : register(b0, space0)
{
    FrameBufferData frameBuffer;
    //float g_padding[1]; // Padding so the constant buffer is 256-byte aligned.
};

cbuffer RootConstants : register(b0, space1)
{
    uint InstanceLength;
    uint InstanceCapacity;
    uint CommandLength;
    uint CommandCapacity;
};

