#include "compute_common.hlsli"

StructuredBuffer<InstanceData> instances : register(t0, space0);
StructuredBuffer<InstanceCountData> instanceCount : register(t1, space0);

RWStructuredBuffer<DrawIndirectArgs> outputCommands : register(u0, space1);
RWStructuredBuffer<uint> visibleInstanceIndices : register(u1, space1);

bool IsAABBVisible(float4x4 transform, float3 minBounds, float3 maxBounds)
{
    float3 aabbCorners[8] = {
        float3(minBounds.x, minBounds.y, minBounds.z),
        float3(maxBounds.x, minBounds.y, minBounds.z),
        float3(minBounds.x, maxBounds.y, minBounds.z),
        float3(maxBounds.x, maxBounds.y, minBounds.z),
        float3(minBounds.x, minBounds.y, maxBounds.z),
        float3(maxBounds.x, minBounds.y, maxBounds.z),
        float3(minBounds.x, maxBounds.y, maxBounds.z),
        float3(maxBounds.x, maxBounds.y, maxBounds.z)
    };

    for (int i = 0; i < 8; ++i)
    {
        float4 vertexObjectPosition = mul(transform, float4(aabbCorners[i].xyz, 1.0f));
        float4 vertexViewPosition = mul(frameBuffer.g_view, vertexObjectPosition);
        float4 vertexProjectionPosition = mul(frameBuffer.g_projection, vertexViewPosition);

        if (vertexProjectionPosition.w < 0.05f)
            continue;
        
        float3 ndc = float3(
            vertexProjectionPosition.x / vertexProjectionPosition.w,
            vertexProjectionPosition.y / vertexProjectionPosition.w,
            vertexProjectionPosition.z / vertexProjectionPosition.w
        );

        bool isVisibleX = ndc.x >= -1.0f && ndc.x <= 1.0f;
        bool isVisibleY = ndc.y >= -1.0f && ndc.y <= 1.0f;
        bool isVisibleZ = ndc.z >= 0 && ndc.z <= 1.0f;
        isVisibleZ = true;

        if (isVisibleX && isVisibleY && isVisibleZ)
            return true;
    }

    return false;
}

#define threadBlockSize 64

// TODO: Occlusion culling based on previous frames depth buffer

[numthreads(threadBlockSize, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID)
{
    uint instanceIndex = DTid.x;
    
    if (instanceIndex >= InstanceLength)
        return;
    
    uint commandIndex = 0;
    {
        for (uint i = 1; i < CommandLength; i++)
        {
            if (instanceIndex < instanceCount[i].offset)
            {
                commandIndex = i;
                break;
            }
        }

        bool cullingEnabled = true;
        if (cullingEnabled)
        {
            uint localInstanceIndex = instanceIndex - instanceCount[commandIndex].offset;

            float4x4 transform = 0;
            transform._11_12_13_14 = instances[instanceIndex].instanceTransform._11_12_13_41;
            transform._21_22_23_24 = instances[instanceIndex].instanceTransform._21_22_23_42;
            transform._31_32_33_34 = instances[instanceIndex].instanceTransform._31_32_33_43;
            transform._44 = 1;

            bool isVisible = IsAABBVisible(transform, instanceCount[commandIndex].bounds.min, instanceCount[commandIndex].bounds.max);
            // isVisible = instances[instanceIndex].instanceTransform._31_32_33_43.w < 50;
            //isVisible = localInstanceIndex % 5 == 1;
            //isVisible = IsAABBVisible(min, max);
            
            if (!isVisible)
                return;
        }

    }
    
    uint previousValue = 0;
    InterlockedAdd(outputCommands[commandIndex].args.InstanceCount, 1, previousValue);
    previousValue += outputCommands[commandIndex].args.StartInstanceLocation;
    visibleInstanceIndices[previousValue] = instanceIndex;
    
    //
    // uint previousValue = 0;
    // // TEMP
    // if (frameBuffer.g_frameIndex == 0)
    // {
    //     InterlockedAdd(outputCommands0[commandIndex].args.InstanceCount, 1, previousValue);
    //     previousValue += outputCommands0[commandIndex].args.StartInstanceLocation;
    //     visibleInstanceIndices0[previousValue] = instanceIndex;
    // }
    // else
    // {
    //     InterlockedAdd(outputCommands1[commandIndex].args.InstanceCount, 1, previousValue);
    //     previousValue += outputCommands1[commandIndex].args.StartInstanceLocation;
    //     visibleInstanceIndices1[previousValue] = instanceIndex;
    // }
}
