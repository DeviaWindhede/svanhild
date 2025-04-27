#include "compute_common.hlsli"

StructuredBuffer<InstanceData> instances : register(t0, space0);
StructuredBuffer<InstanceCountData> instanceCount : register(t1, space0);
RWStructuredBuffer<DrawIndirectArgs> outputCommands[FRAME_COUNT] : register(u0, space1);

#define threadBlockSize 64

[numthreads(threadBlockSize, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID)
{
    uint instanceIndex = DTid.x;
    
    if (instanceIndex >= NumInstances)
        return;
    
    uint commandIndex = 0;
    {
        uint accumulatedInstances = 0;

        for (uint i = 1; i < NumCommands; i++)
        {
            if (instanceIndex < instanceCount[i].offset)
            {
                commandIndex = i;
                break;
            }
        }

        uint localInstanceIndex = instanceIndex - instanceCount[commandIndex].offset;
        bool isVisible = instances[instanceIndex].instanceTransform._31_32_33_43.w < 50;
        if (!isVisible)
            return;
    }

    InterlockedAdd(outputCommands[frameBuffer.g_frameIndex][commandIndex].InstanceCount, 1);
}
