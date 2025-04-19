#include "types.hlsli"

cbuffer RootConstants : register(b0)
{
    uint NumInstances;
    uint NumCommands;
};

StructuredBuffer<InstanceData> instances : register(t0, space0);
StructuredBuffer<InstanceCountData> instanceCount : register(t1, space0);
RWStructuredBuffer<DrawIndirectArgs> outputCommands[FRAME_COUNT] : register(u0, space0);

#define threadBlockSize 64

[numthreads(threadBlockSize, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID)
{
    // uint instanceIndex = DTid.x;
    // uint commandIndex = 0;
    //
    // // if (instanceIndex != 0)
    // //     return;
    //
    // // if (instanceCount[1].offset != 180)
    // //     return;
    //
    // if (instanceIndex >= NumInstances)
    //     return;
    //
    // {
    //     // Find which draw command this instance belongs to
    //     uint accumulatedInstances = 0;
    //     for (uint i = 0; i < NumCommands; i++)
    //     {
    //         accumulatedInstances += inputCommands[i].InstanceCount;
    //         if (instanceIndex < accumulatedInstances)
    //         {
    //             commandIndex = i;
    //             break;
    //         }
    //     }
    // }
    //
    // if (inputCommands[commandIndex].InstanceCount == 0)
    //     return;
        
    // // outputCommands[commandIndex] = inputCommands[commandIndex];
}
