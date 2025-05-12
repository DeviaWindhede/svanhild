#include "compute_common.hlsli"

RWStructuredBuffer<DrawIndirectArgs> outputCommands0 : register(u0, space1);
RWStructuredBuffer<uint> visibleInstanceIndices0 : register(u1, space1);

RWStructuredBuffer<DrawIndirectArgs> outputCommands1 : register(u2, space1);
RWStructuredBuffer<uint> visibleInstanceIndices1 : register(u3, space1);


// RWStructuredBuffer<DrawIndirectArgs> outputCommands[FRAME_COUNT] : register(u0, space1);
// RWStructuredBuffer<uint> visibleInstanceIndices[FRAME_COUNT] : register(u1, space1);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID)
{
    // TEMP
    if (frameBuffer.g_frameIndex == 0)
    {
        outputCommands0[DTid.x].InstanceCount = 0;
        visibleInstanceIndices0[0] = 0;
        return;
    }
    
    outputCommands1[DTid.x].InstanceCount = 0;
    visibleInstanceIndices1[0] = 0;

    
    // outputCommands[frameBuffer.g_frameIndex][DTid.x].InstanceCount = 0;
    // visibleInstanceIndices[frameBuffer.g_frameIndex][0] = 0;
}
