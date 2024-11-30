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
//#include "common.hlsli"


Texture2D g_texture : register(t0);
Texture2D g_texture1 : register(t1);
SamplerState g_sampler : register(s0);

PSInput VSMain(VertexInputType Input)
{
    PSInput result;
    
    // todo add object transform instanced data
    float4 vertexObjectPosition = mul(testTransform, float4(Input.position.x, Input.position.y, Input.position.z, 1.0f));
    float4 vertexViewPosition = mul(g_view, vertexObjectPosition);
    float4 vertexProjectionPosition = mul(g_projection, vertexViewPosition);
    
    result.position = vertexProjectionPosition;
    result.uv = Input.uv;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(input.uv.x, 0, 0, 1);
    //return g_texture.Sample(g_sampler, input.uv);
}
