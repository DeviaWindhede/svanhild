#include "types.hlsli"

#define MAX_TEXTURE_COUNT 4096

cbuffer SceneConstantBuffer : register(b0)
{
    FrameBufferData frameBuffer;
    //float g_padding[1]; // Padding so the constant buffer is 256-byte aligned.
};

struct VertexInputType
{
    float3 position : POSITION;
    float3 color : COLOR;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 binormal : BINORMAL;
    float3 tangent : TANGENT;
    float4x3 instanceTransform : WORLD;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
    uint temp : WHATEVER;
    float time : TIME;
};
