#include "pch.h"
#include "InstanceCountBuffer.h"

#include "DX12.h"

InstanceCountBuffer::InstanceCountBuffer(class DX12* aDx12) :
ResourceBuffer<InstanceCountData>(D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
dx12(aDx12)
{
}

void InstanceCountBuffer::Create(ComPtr<ID3D12Device>& aDevice, size_t aSize)
{
    ResourceBuffer<InstanceCountData>::Create(aDevice, aSize);
    resource->SetName(L"InstanceCountBuffer");
    {
        
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.NumElements = cpuData.capacity();
        srvDesc.Buffer.StructureByteStride = sizeof(InstanceCountData);
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(dx12->myComputeCbvSrvUavHeap.GetStaticCPUHandle(static_cast<UINT>(ComputeSrvStaticOffsets::InstanceCount)));
        dx12->myDevice->CreateShaderResourceView(resource.Get(), &srvDesc, handle);
        
        handle = CD3DX12_CPU_DESCRIPTOR_HANDLE (dx12->myGraphicsCbvSrvUavHeap.GetStaticCPUHandle(static_cast<UINT>(GraphicsSrvStaticOffsets::InstanceCount)));
        dx12->myDevice->CreateShaderResourceView(resource.Get(), &srvDesc, handle);
    }
}
