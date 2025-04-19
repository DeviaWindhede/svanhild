#include "types.hlsli"

cbuffer RootConstants : register(b0)
{
    uint NumInstances;
    uint NumCommands;
};

RWByteAddressBuffer outputCommands : register(u0);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID)
{
    outputCommands.Store(4, 0); // instance count is on offset 4
}
