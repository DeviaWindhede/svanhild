#pragma once
#include "DXHelper.h"
#include "ResourceBuffer.h"
// #include "ResourceBuffer.h"
// #include "ResourceBuffer.h"

struct DrawIndirectArgs {
    UINT IndexCountPerInstance;
    UINT InstanceCount;
    UINT StartIndexLocation;
    UINT BaseVertexLocation;
    UINT StartInstanceLocation;
};

struct RootConstants
{
    UINT NumInstances;
    UINT NumCommands;
};
static_assert((sizeof(RootConstants) % sizeof(UINT)) == 0, "Root Signature size must be 32bit value aligned");
static_assert(sizeof(RootConstants) <= 64, "Root Signature size must be or below 64-bytes");

enum class ComputeRootParameters
{
    SrvUavTable,
    RootConstants,
    ComputeRootParametersCount
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

class MeshRenderer final : public ResourceBuffer<DrawIndirectArgs>
{
public:
    friend class DX12;

    explicit MeshRenderer();
    void Create(class DX12* aDx12, size_t aSize);
    size_t AddItem(ComPtr<ID3D12Device>& aDevice, DrawIndirectArgs* aData, size_t aSize) override;
    void Update(ComPtr<ID3D12GraphicsCommandList>& aComputeCommandList) override;
    
    void Dispatch(DX12* aDx12);
    void ExecuteIndirectRender(DX12* aDx12);
    void OnEndFrame(DX12* aDx12);
private:
    void UpdateRootConstants(DX12* aDx12);
    void Create(ComPtr<ID3D12Device>& aDevice, size_t aSize) override;
    void CreateResourceViews();
    static UINT GetFrameGroupCount(size_t aSize);
    
    CD3DX12_CPU_DESCRIPTOR_HANDLE uavArgsHandle;
    
    RootConstants rootConstants;
    
    DX12* dx12 = nullptr;
};


