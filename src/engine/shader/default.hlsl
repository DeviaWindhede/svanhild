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
//#include "common.hlsli"


Texture2D g_texture[MAX_BOUND_SRV_COUNT] : register(t0);
SamplerState g_sampler : register(s0);

PSInput VSMain(VertexInputType Input)
{
    PSInput result;
    
    //float4x4 transform = float4x4(
    //    1, 0, 0, 0,
    //    0, 1, 0, 0,
    //    0, 0, 1, 0,
    //    0, 0, 0, 1
    //);
    float4x4 transform = 0;
    
    transform._11_12_13_14 = Input.instanceTransform._11_12_13_41;
    transform._21_22_23_24 = Input.instanceTransform._21_22_23_42;
    transform._31_32_33_34 = Input.instanceTransform._31_32_33_43;
    transform._44 = 1;
    
    // todo add object transform instanced data
    float4 vertexObjectPosition = mul(transform, float4(Input.position.x, Input.position.y, Input.position.z, 1.0f));
    float4 vertexViewPosition = mul(g_view, vertexObjectPosition);
    float4 vertexProjectionPosition = mul(g_projection, vertexViewPosition);
    
    result.position = vertexProjectionPosition;
    result.uv = Input.uv;
    result.color = float4(Input.color.rgb, 1);
    result.temp = g_renderPass;
    
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    //return float4(input.uv.xy, 0, 1);
    //return g_texture.Sample(g_sampler, float2(0, 0));
    float2 uv = float2(0, 0);
    //uv.x = cos(input.color.x);
    //uv.y = sin(input.color.x);
    //return g_texture.Sample(g_sampler, uv);
    
    switch (input.temp)
    {
        case 0:
            return g_texture[0].Sample(g_sampler, input.uv);
        case 1:
            return g_texture[1].Sample(g_sampler, input.uv);
        case 2:
            return g_texture[2].Sample(g_sampler, input.uv);
        case 3:
            return g_texture[3].Sample(g_sampler, input.uv);
        case 4:
            return g_texture[4].Sample(g_sampler, input.uv);
        case 5:
            return g_texture[5].Sample(g_sampler, input.uv);
        case 6:
            return g_texture[6].Sample(g_sampler, input.uv);
        case 7:
            return g_texture[7].Sample(g_sampler, input.uv);
        default:
            break;
    }
    
    return float4(1, input.temp, 1, 1);
    //return g_texture[input.temp].Sample(g_sampler, input.uv);
    //return input.color;

}
