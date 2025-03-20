#include "types.hlsli"

struct DrawIndirectArgs
{
    uint IndexCountPerInstance;
    uint InstanceCount;
    uint StartIndexLocation;
    uint BaseVertexLocation;
    uint StartInstanceLocation;
};

StructuredBuffer<InstanceData> instances : register(t0); // instance data
StructuredBuffer<DrawIndirectArgs> inputCommands : register(t1); // render commands, i.e mesh info

RWStructuredBuffer<DrawIndirectArgs> outputCommands : register(u0); // output draw commands
// AppendStructuredBuffer<DrawIndirectArgs> outputCommands : register(u0); // output draw commands


#define threadBlockSize 64

[numthreads(threadBlockSize, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID)
{
    if (DTid.x > 0)
        return;
    
    //outputCommands[0] = inputCommands[0];
    uint instanceIndex = DTid.x; // Unique instance index
    
     // TODO: SET ALL THIS INFO AS CONSTANT DATA
     {
        uint numInstances;
        uint stride;
        instances.GetDimensions(numInstances, stride);
        if (instanceIndex >= numInstances)
            return;
    }
    
    uint commandIndex = 0;
     {
        uint numCommands;
        uint stride;
        instances.GetDimensions(numCommands, stride);
         
         // Find which draw command this instance belongs to
        uint accumulatedInstances = 0;
        for (uint i = 0; i < numCommands; i++)
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
    
    outputCommands[commandIndex] = inputCommands[commandIndex];
}
