#include "compute_common.hlsli"

RWStructuredBuffer<DrawIndirectArgs> outputCommands : register(u0, space1);
RWStructuredBuffer<uint> visibleInstanceIndices : register(u1, space1);

// RWStructuredBuffer<DrawIndirectArgs> outputCommands[FRAME_COUNT] : register(u0, space1);
// RWStructuredBuffer<uint> visibleInstanceIndices[FRAME_COUNT] : register(u1, space1);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID)
{
    outputCommands[DTid.x].args.InstanceCount = 0;
    visibleInstanceIndices[0] = 0;
    
    // outputCommands[frameBuffer.g_frameIndex][DTid.x].InstanceCount = 0;
    // visibleInstanceIndices[frameBuffer.g_frameIndex][0] = 0;
}
