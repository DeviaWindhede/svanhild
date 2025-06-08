#include "common.hlsli"

DrawIndirectArgsData DrawArgsData : register(b1);

StructuredBuffer<InstanceData> instanceBuffer : register(t0, space0);
StructuredBuffer<InstanceCountData> instanceCount : register(t1, space0);
StructuredBuffer<uint> visibleInstanceIndices: register(t2, space0);

// TODO: BIND VISIBLE

PSInput main(VertexInputType Input, uint instanceID : SV_InstanceID)
{
    PSInput result;
    
    //float4x4 transform = float4x4(
    //    1, 0, 0, 0,
    //    0, 1, 0, 0,
    //    0, 0, 1, 0,
    //    0, 0, 0, 1
    //);
    float4x4 transform = 0;

    uint visibleInstanceIndex = instanceID + DrawArgsData.StartInstanceOffset;
    uint index = visibleInstanceIndices[visibleInstanceIndex];
    transform._11_12_13_14 = instanceBuffer[index].instanceTransform._11_12_13_41;
    transform._21_22_23_24 = instanceBuffer[index].instanceTransform._21_22_23_42;
    transform._31_32_33_34 = instanceBuffer[index].instanceTransform._31_32_33_43;
    transform._44 = 1;
    
    // todo add object transform instanced data
    float4 vertexObjectPosition = mul(transform, float4(Input.position.x, Input.position.y, Input.position.z, 1.0f));
    float4 vertexViewPosition = mul(frameBuffer.g_view, vertexObjectPosition);
    float4 vertexProjectionPosition = mul(frameBuffer.g_projection, vertexViewPosition);
    
    result.position = vertexProjectionPosition;
    result.uv = Input.uv;
    result.renderPass = frameBuffer.g_renderPass;
    result.textureIndex = 0;//DrawArgsData.MeshIndex;
    result.time = frameBuffer.g_time;
    result.color = float4(Input.color.rgb, 1);
    
    return result;
}
