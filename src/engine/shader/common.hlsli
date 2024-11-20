cbuffer SceneConstantBuffer : register(b0)
{
    float4x4 g_view; // mesh to world (inverse view)
    float4x4 g_projection; // world to clip
    float2 g_resolution;
    float2 g_viewport;
    float g_nearPlane;
    float g_farPlane;
    float2 g_padding;
    float4x4 testTransform; // world to clip
    float4 offset;
    float4 padding[4];
};

struct VertexInputType
{
    float3 position : POSITION;
    float3 color : COLOR;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 binormal : BINORMAL;
    float3 tangent : TANGENT;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};
