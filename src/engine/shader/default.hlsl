cbuffer SceneConstantBuffer : register(b0)
{
    float4x4 g_view; // mesh to world (inverse view)
    float4x4 g_projection; // world to clip
    float2 g_resolution;
    float2 g_viewport;
    float g_nearPlane;
    float g_farPlane;
    float2 g_padding;
    float4x3 testTransform; // world to clip
    float4 padding0;
    float4 offset;
    float time;
    float4 padding[3];
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
};
//#include "common.hlsli"


Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

PSInput VSMain(VertexInputType Input)
{
    PSInput result;
    
    //padding0 = float4(0, 0, 0, 1);
    
    float4x4 transform = float4x4(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 10,
        0, 0, 0, 1
    );
    //transform = testTransform;
    //transform._11 = testTransform._21;
    transform._11_21_31 = testTransform._11_12_13;
    //transform._12_22_32 = testTransform._21_22_23;
    //transform._13_23_33 = testTransform._31_32_33;
    
    //transform._14_24_34 = testTransform._41_42_43;
    //transform._m00_m01_m02 = testTransform._m00_m01_m02;
    //transform._11_12_13 = testTransform._11_12_13;
    //transform._21_22_23 = testTransform._21_22_23;
    //transform._31_32_33 = testTransform._31_32_33;
    //transform._41_41_41 = testTransform._41_41_41;
    // todo add object transform instanced data
    
    //transform = testTransform;
    transform._11_12_13_14 = Input.instanceTransform._11_12_13_41;
    transform._21_22_23_24 = Input.instanceTransform._21_22_23_42;
    transform._31_32_33_34 = Input.instanceTransform._31_32_33_43;
    //transform._41_42_43_44 = testTransform._41_42_43_44;
    
    // todo add object transform instanced data
    float4 vertexObjectPosition = mul(transform, float4(Input.position.x, Input.position.y, Input.position.z, 1.0f));
    float4 vertexViewPosition = mul(g_view, vertexObjectPosition);
    float4 vertexProjectionPosition = mul(g_projection, vertexViewPosition);
    
    result.position = vertexProjectionPosition;
    result.uv = Input.uv;
    result.color = float4(Input.color.rgb, 1);

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
    return g_texture.Sample(g_sampler, input.uv);
    //return input.color;

}
