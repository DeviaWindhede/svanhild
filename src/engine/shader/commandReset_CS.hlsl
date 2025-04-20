#include "types.hlsli"

cbuffer RootConstants : register(b0)
{
    uint NumInstances;
    uint NumCommands;
    uint FrameIndex;
};

RWStructuredBuffer<DrawIndirectArgs> outputCommands[FRAME_COUNT] : register(u0, space0);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID)
{
    outputCommands[FrameIndex][DTid.x].InstanceCount = 0;
}
