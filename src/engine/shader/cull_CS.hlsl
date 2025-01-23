struct InstanceData
{
    float4x3 WorldMatrix;
    uint ModelIndex;
};

struct D3D12_DRAW_INDEXED_ARGUMENTS
{
    uint IndexCountPerInstance;
    uint InstanceCount;
    uint StartIndexLocation;
    int BaseVertexLocation;
    uint StartInstanceLocation;
};

struct IndirectDraw
{
    D3D12_DRAW_INDEXED_ARGUMENTS args;
    uint ModelIndex;
};

StructuredBuffer<InstanceData> gInstanceBuffer : register(t0);
RWStructuredBuffer<uint> gVisibilityBuffer : register(u0);
RWStructuredBuffer<IndirectDraw> gIndirectBuffer : register(u1);

cbuffer FrustumCB : register(b0)
{
    matrix gFrustumPlanes[6];
};

float Dot(float4 a, float4 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

[numthreads(256, 1, 1)]
void CullingCS(uint3 threadId : SV_DispatchThreadID)
{
    uint instanceId = threadId.x;
    
    InstanceData instance = gInstanceBuffer[instanceId];
    
    // Perform frustum culling (pseudo-code for intersection test)
    bool isVisible = true; // Replace with actual frustum intersection test
    for (int i = 0; i < 6; ++i)
    {
        if (Dot(gFrustumPlanes[i]._11_12_13_14, float4(instance.WorldMatrix[3], 1.0)) < 0.0)
        {
            isVisible = false;
            break;
        }
    }

    gVisibilityBuffer[instanceId] = isVisible ? 1 : 0;

    // Update indirect arguments for the corresponding model
    if (isVisible)
    {
        InterlockedAdd(gIndirectBuffer[instance.ModelIndex].args.InstanceCount, 1);
    }
}
