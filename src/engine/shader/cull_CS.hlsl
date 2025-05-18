#include "compute_common.hlsli"

StructuredBuffer<InstanceData> instances : register(t0, space0);
StructuredBuffer<InstanceCountData> instanceCount : register(t1, space0);

// RWByteAddressBuffer uavArray[4] : register(u0, space1);

// RWStructuredBuffer<DrawIndirectArgs> outputCommands[FRAME_COUNT] : register(u0, space1);
// RWStructuredBuffer<uint> visibleInstanceIndices[FRAME_COUNT] : register(u1, space1);
//
// RWStructuredBuffer<DrawIndirectArgs> outputCommands[FRAME_COUNT] : register(u0, space1);
// RWStructuredBuffer<uint> visibleInstanceIndices[FRAME_COUNT] : register(u1, space1);

//
// uint indirectArgsSize = 20;
// uint commandSize = indirectArgsSize * commandIndex;
// uint baseIndex = frameBuffer.g_frameIndex * (CommandCapacity + InstanceCapacity);
// //uint index = baseIndex + commandIndex;
// uint index = baseIndex + commandSize + 4;
// // uavArray.InterlockedAdd(index, 1);
// uavArray[2].InterlockedAdd(commandSize + 4, 1);
    
// frameBuffer.g_frameIndex
// uavBuffers[index].InterlockedAdd(4, 1);

// InterlockedAdd(outputCommands[frameBuffer.g_frameIndex][commandIndex].InstanceCount, 1);


// TODO: FIX PROPER BINDLESS WITHOUT COMPILER COMPLAINING ABOUT OUT OF BOUNDS ACCESS

RWStructuredBuffer<DrawIndirectArgs> outputCommands0 : register(u0, space1);
RWStructuredBuffer<uint> visibleInstanceIndices0 : register(u1, space1);
RWStructuredBuffer<DrawIndirectArgs> outputCommands1 : register(u2, space1);
RWStructuredBuffer<uint> visibleInstanceIndices1 : register(u3, space1);

#define threadBlockSize 64

[numthreads(threadBlockSize, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID)
{
    uint instanceIndex = DTid.x;
    
    if (instanceIndex >= InstanceLength)
        return;
    
    uint commandIndex = 0;
    {
        uint accumulatedInstances = 0;

        for (uint i = 1; i < CommandLength; i++)
        {
            if (instanceIndex < instanceCount[i].offset)
            {
                commandIndex = i;
                break;
            }
        }

        uint localInstanceIndex = instanceIndex - instanceCount[commandIndex].offset;

        bool isVisible = true;
        // isVisible = instances[instanceIndex].instanceTransform._31_32_33_43.w < 50;
        // isVisible = localInstanceIndex % 2 == 1;
        if (!isVisible)
            return;
    }
    
    uint previousValue = 0;
    // TEMP
    if (frameBuffer.g_frameIndex == 0)
    {
        InterlockedAdd(outputCommands0[commandIndex].args.InstanceCount, 1, previousValue);
        previousValue += outputCommands0[commandIndex].args.StartInstanceLocation;
        visibleInstanceIndices0[previousValue] = instanceIndex;
    }
    else
    {
        InterlockedAdd(outputCommands1[commandIndex].args.InstanceCount, 1, previousValue);
        previousValue += outputCommands0[commandIndex].args.StartInstanceLocation;
        visibleInstanceIndices1[previousValue] = instanceIndex;
    }
}
