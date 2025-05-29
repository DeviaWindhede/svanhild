#define FRAME_COUNT 2

struct FrustumPlane {
    float3 normal;
    float d;
};

struct FrameBufferViewport
{
    float2 bounds;
    float nearPlane;
    float farPlane;
};

struct FrameBufferData
{
    float4x4 g_view; // mesh to world (inverse view)
    float4x4 g_projection; // world to clip
    FrameBufferViewport g_viewport;
    float g_time;
    uint g_renderPass;
    uint g_frameIndex;
    float g_padding;
    FrustumPlane g_frustumPlanes[6];
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

struct AABB
{
    float3 min;
    float3 max;
};

struct InstanceCountData
{
    AABB bounds;
    uint instanceCount;
    uint offset;
};
