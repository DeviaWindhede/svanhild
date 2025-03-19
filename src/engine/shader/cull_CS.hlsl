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


#define threadBlockSize 128

[numthreads(threadBlockSize, 1, 1)]
void main(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex, uint3 DTid : SV_DispatchThreadID)
{
    uint index = (groupId.x * threadBlockSize) + groupIndex;
    
    if (DTid.x >= 1)
        return;
    
    if (index >= 1)
        return;

    // InstanceData instance = instances[index];
    
    // DrawIndirectArgs args;
    // args.BaseVertexLocation = 0;
    // args.IndexCountPerInstance = 36;
    // args.InstanceCount = 1;
    // args.StartIndexLocation = 0;
    // args.StartInstanceLocation = 0;
    // outputCommands.Append(args);
    outputCommands[0] = inputCommands[0];
}
