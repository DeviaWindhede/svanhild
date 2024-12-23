#define MAX_BOUND_SRV_COUNT 8

cbuffer SceneConstantBuffer : register(b0)
{
    float4x4 g_view; // mesh to world (inverse view)
    float4x4 g_projection; // world to clip
    float2 g_viewport;
    float g_nearPlane;
    float g_farPlane;
    float g_time;
    uint g_renderPass;
    float g_padding[23]; // Padding so the constant buffer is 256-byte aligned.
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
};
