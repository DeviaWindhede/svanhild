
struct FrameBufferData
{
    float4x4 g_view; // mesh to world (inverse view)
    float4x4 g_projection; // world to clip
    float2 g_viewport;
    float g_nearPlane;
    float g_farPlane;
    float g_time;
    uint g_renderPass;
};

struct InstanceData
{
    float4x3 instanceTransform : WORLD;
    uint modelIndex;
};
