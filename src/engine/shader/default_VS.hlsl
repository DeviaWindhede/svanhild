#include "common.hlsli"

PSInput main(VertexInputType Input)
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
    float4 vertexViewPosition = mul(frameBuffer.g_view, vertexObjectPosition);
    float4 vertexProjectionPosition = mul(frameBuffer.g_projection, vertexViewPosition);
    
    
    result.position = vertexProjectionPosition;
    result.uv = Input.uv;
    result.color = float4(Input.color.rgb, 1);
    result.temp = frameBuffer.g_renderPass;
    result.time = frameBuffer.g_time;
    
    return result;
}
