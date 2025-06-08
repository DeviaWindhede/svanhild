#pragma once

class DescriptorHeapHandle
{
public:
    DescriptorHeapHandle()
    {
        cpuHandle.ptr = NULL;
        gpuHandle.ptr = NULL;
        heapIndex = 0;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
    UINT heapIndex;

    bool IsValid() const { return cpuHandle.ptr != NULL; }
    bool IsReferencedByShader() const { return gpuHandle.ptr != NULL; }
};


