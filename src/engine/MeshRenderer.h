#pragma once
#include "DXHelper.h"
// #include "ResourceBuffer.h"

struct DrawIndirectArgs {
    UINT IndexCountPerInstance;
    UINT InstanceCount;
    UINT StartIndexLocation;
    UINT BaseVertexLocation;
    UINT StartInstanceLocation;
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

class MeshRenderer
{
public:
    friend class DX12;
    
    void Create(class DX12* aDx12);
    void Update();


    void Dispatch(DX12* aDx12);
    void PrepareRender(DX12* aDx12);
    void ExecuteIndirectRender(DX12* aDx12);
    void OnEndFrame(DX12* aDx12);
private:
    UINT GetFrameGroupCount() const;
    
    CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle;
    CD3DX12_CPU_DESCRIPTOR_HANDLE uavArgsHandle;
    
    ComPtr<ID3D12Resource> inputCommandBuffer = nullptr; // srv
    ComPtr<ID3D12Resource> indirectArgsBuffer = nullptr; // uav
    ComPtr<ID3D12Resource> commandBufferUpload = nullptr;
    
    size_t heapSize = 0;
	std::vector<DrawIndirectArgs> commands;
    // ResourceBuffer<Mesh> meshes;
};
