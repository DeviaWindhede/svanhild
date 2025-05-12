#pragma once
#include "DXHelper.h"
#include "OutputCommandBuffer.h"
#include "RenderConstants.h"
// #include "ResourceBuffer.h"
// #include "ResourceBuffer.h"

struct RootConstants
{
    UINT InstanceLength;
    UINT InstanceCapacity;
    UINT CommandLength;
    UINT CommandCapacity;
};
static_assert((sizeof(RootConstants) % sizeof(UINT)) == 0, "Root Signature size must be 32bit value aligned");
static_assert(sizeof(RootConstants) <= 64, "Root Signature size must be or below 64-bytes");

enum class ComputeRootParameters
{
    CbvSrvUavTable,
    PerFrameCbvSrvUavTable,
    FrameBuffer,
    RootConstants,
    Count
};

class Mesh;

/*
 *Gameplan:
 *	1. Skapa en stor buffer som innehåller både indices och vertices
 *	2. Skapa en array med (struct) array offsets som används som en map från model index till array offset
 *	3. Skapa en "create render command" funktion eller liknande som specifierar mängd data, model index etc som sedan mappas in i command buffern
 *	4. Gör så att det finns en subscription event för när modelindexen ändras pga unload eller defrag
 *	5. Gör så allt är bindless, dvs ha texturer också och bind det rätt
 *	6. Lägg till en defragmentation funktion
 */

class MeshRenderer final
{
public:
    friend class DX12;

    explicit MeshRenderer();
    ~MeshRenderer();

    void Cleanup();
    void Create(DX12* aDx12);
    void Update(ComPtr<ID3D12GraphicsCommandList>& aComputeCommandList);
    size_t AddItem(ComPtr<ID3D12Device>& aDevice, DrawIndirectArgs* aData, size_t aSize);

    void Dispatch();
    void ExecuteIndirectRender();
    void OnEndFrame();

    bool IsReady() const;
private:
    static constexpr UINT MIN_BUFFER_CONTENT_SIZE = 1;
    
    void UpdateRootConstants();
    
    RootConstants rootConstants;
    OutputCommandBuffer buffers[RenderConstants::FrameCount];
    
    DX12* dx12 = nullptr;
};


