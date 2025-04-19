#include "types.hlsli"

cbuffer RootConstants : register(b0)
{
    uint NumInstances;
    uint NumCommands;
};

StructuredBuffer<InstanceData> instances : register(t0); // instance data
StructuredBuffer<InstanceCountData> instanceCount : register(t1);
StructuredBuffer<DrawIndirectArgs> inputCommands : register(t2); // render commands, i.e mesh info

RWStructuredBuffer<DrawIndirectArgs> outputCommands : register(u0); // output draw commands
// AppendStructuredBuffer<DrawIndirectArgs> outputCommands : register(u0); // output draw commands


#define threadBlockSize 64

[numthreads(threadBlockSize, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID)
{
    return;
    uint instanceIndex = DTid.x;
    uint commandIndex = 0;

    // if (instanceIndex != 0)
    //     return;

    // if (instanceCount[1].offset != 180)
    //     return;
    
    if (instanceIndex >= NumInstances)
        return;

    {
        // Find which draw command this instance belongs to
        uint accumulatedInstances = 0;
        for (uint i = 0; i < NumCommands; i++)
        {
            accumulatedInstances += inputCommands[i].InstanceCount;
            if (instanceIndex < accumulatedInstances)
            {
                commandIndex = i;
                break;
            }
        }
    }
    
    if (inputCommands[commandIndex].InstanceCount == 0)
        return;
        
    // outputCommands[commandIndex] = inputCommands[commandIndex];
}
