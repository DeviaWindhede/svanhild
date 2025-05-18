#define FRAME_COUNT 2

struct FrameBufferData
{
    float4x4 g_view; // mesh to world (inverse view)
    float4x4 g_projection; // world to clip
    float2 g_viewport;
    float g_nearPlane;
    float g_farPlane;
    float g_time;
    uint g_renderPass;
    uint g_frameIndex;
};

struct InstanceData
{
    float4x3 instanceTransform : WORLD;
    uint modelIndex;
};

struct D3D12_DRAW_INDEXED_ARGUMENTS
{
    uint IndexCountPerInstance;
    uint InstanceCount;
    uint StartIndexLocation;
    int BaseVertexLocation;
    uint StartInstanceLocation;
};

struct DrawIndirectArgsData
{
    uint StartInstanceOffset;
    uint MeshIndex;
};

struct DrawIndirectArgs
{
    DrawIndirectArgsData data;
    D3D12_DRAW_INDEXED_ARGUMENTS args;
};

struct InstanceCountData
{
    uint instanceCount;
    uint offset;
};
