#include "pch.h"
#include "MeshRenderer.h"

#include "DX12.h"
#include "ShaderCompiler.h"
#include "mesh/Vertex.h"

MeshRenderer::MeshRenderer() : ResourceBuffer(D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER)
{
}

void MeshRenderer::Create(class DX12* aDx12, size_t aSize)
{
	dx12 = aDx12;
	Create(aDx12->myDevice, aSize);
}

void MeshRenderer::Update(ComPtr<ID3D12GraphicsCommandList>& aCommandList)
{
	bool wasDirty = dirty;
	ResourceBuffer<DrawIndirectArgs>::Update(aCommandList);
	
	if (wasDirty)
	{
		CreateResourceViews();
	}
}

void MeshRenderer::Dispatch(DX12* aDx12)
{
	if (gpuSize == 0)
		return;
	
	D3D12_GPU_DESCRIPTOR_HANDLE uavGPUHandle = aDx12->myComputeCbvSrvUavHeap.gpuStart;
	uavGPUHandle.ptr += aDx12->cbvSrvUavDescriptorSize;
	D3D12_GPU_DESCRIPTOR_HANDLE uavArgsGPUHandle = uavGPUHandle;
	uavArgsGPUHandle.ptr += aDx12->cbvSrvUavDescriptorSize;

	ID3D12DescriptorHeap* descriptorHeaps[] = { aDx12->myComputeCbvSrvUavHeap.descriptorHeap };
	aDx12->myComputeCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	aDx12->myComputeCommandList->SetComputeRootDescriptorTable(0, aDx12->myComputeCbvSrvUavHeap.gpuStart); // SRV
	//aDx12->myComputeCommandList->SetComputeRootDescriptorTable(1, uavGPUHandle); // UAV for visible instances
	//aDx12->myComputeCommandList->SetComputeRootDescriptorTable(2, uavArgsGPUHandle); // UAV for indirect draw args
	
	{
	
		// TODO: Reset indirectArgsBuffer here
		
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			indirectArgsBuffer[aDx12->myFrameIndex].Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS
		);
		aDx12->myComputeCommandList->ResourceBarrier(1, &barrier);
		
		aDx12->myComputeCommandList->Dispatch(GetFrameGroupCount(aDx12->instanceBuffer.GetCpuSize()), 1, 1);
	}
	
	{
		D3D12_GPU_DESCRIPTOR_HANDLE uavGPUHandle = aDx12->myComputeCbvSrvUavHeap.gpuStart;
		uavGPUHandle.ptr += aDx12->myDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		
		//aDx12->myCommandList->SetGraphicsRootDescriptorTable(0, aDx12->myCbvSrvUavHeap.gpuStart);
		//aDx12->myCommandList->SetGraphicsRootDescriptorTable(1, uavGPUHandle);

		// Issue indirect draw call
	}
}

void MeshRenderer::PrepareRender(DX12* aDx12)
{
	if (gpuSize == 0)
		return;
	
	CD3DX12_RESOURCE_BARRIER barriers[1]
	{
		CD3DX12_RESOURCE_BARRIER::Transition(
			indirectArgsBuffer[aDx12->myFrameIndex].Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT
		)
	};
            
	aDx12->myCommandList->ResourceBarrier(_countof(barriers), barriers);
}


static inline UINT AlignForUavCounter(UINT bufferSize)
{
    const UINT alignment = D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT;
    return (bufferSize + (alignment - 1)) & ~(alignment - 1);
}

void MeshRenderer::ExecuteIndirectRender(DX12* aDx12)
{
	if (gpuSize == 0 || !indirectArgsBuffer)
		return;
	
	const UINT sizePerFrame = gpuSize * sizeof(DrawIndirectArgs);
	const UINT counterOffset = AlignForUavCounter(sizePerFrame);
	
	aDx12->myCommandList->ExecuteIndirect(
		aDx12->myCommandSignature.Get(), // Predefined signature for DrawIndexedInstanced
		gpuSize,                              // Execute 1 draw call
		indirectArgsBuffer[aDx12->myFrameIndex].Get(),       // Buffer with draw arguments
		0,                              // Argument buffer offset
		indirectArgsBuffer[aDx12->myFrameIndex].Get(),                         // No counters
		counterOffset
	);
    
    
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			indirectArgsBuffer[aDx12->myFrameIndex].Get(),
			D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
			D3D12_RESOURCE_STATE_COPY_DEST);
    
	aDx12->myCommandList->ResourceBarrier(1, &barrier);
	// CD3DX12_RESOURCE_BARRIER barriers[1]
	// {
	// 	CD3DX12_RESOURCE_BARRIER::Transition(
	// 		indirectArgsBuffer.Get(),
	// 		D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
	// 		D3D12_RESOURCE_STATE_COPY_DEST
	// 		)
	// };
	// aDx12->myCommandList->ResourceBarrier(_countof(barriers), barriers);
}

void MeshRenderer::OnEndFrame(DX12* aDx12)
{
	
}

void MeshRenderer::Create(ComPtr<ID3D12Device>& aDevice, size_t aSize)
{
	ResourceBuffer<DrawIndirectArgs>::Create(aDevice, aSize);

	if (aSize == 0)
		return;
	
	D3D12_HEAP_PROPERTIES heapProps = { D3D12_HEAP_TYPE_DEFAULT };
	resource->SetName(L"inputCommandBuffer");
	
	// draw argument buffer
	D3D12_RESOURCE_DESC argsDesc = {};
	argsDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	argsDesc.Width = sizeof(DrawIndirectArgs) * aSize;
	// argsDesc.Width = sizeof(DrawIndirectArgs) * heapSize;
	argsDesc.Height = 1;
	argsDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	argsDesc.MipLevels = 1;
	argsDesc.DepthOrArraySize = 1;
	argsDesc.SampleDesc.Count = 1;
	argsDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	for (size_t i = 0; i < FrameCount; i++)
	{
		heapProps = { D3D12_HEAP_TYPE_DEFAULT };
		aDevice->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&argsDesc,
			D3D12_RESOURCE_STATE_COMMON,
			// D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(&indirectArgsBuffer[i])
		);
		NAME_D3D12_OBJECT(indirectArgsBuffer[i]);
		//
		// CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		// 	indirectArgsBuffer[i].Get(),
		// 	D3D12_RESOURCE_STATE_COMMON,
		// 	D3D12_RESOURCE_STATE_UNORDERED_ACCESS
		// );
	}

	CreateResourceViews();
}

void MeshRenderer::CreateResourceViews()
{
	if (gpuSize == 0)
		return;
	
	// CD3DX12_CPU_DESCRIPTOR_HANDLE handle(aDx12->myComputeCbvSrvUavHeap.cpuStart, 1, aDx12->cbvSrvUavDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(dx12->myComputeCbvSrvUavHeap.cpuStart);
	handle.ptr += dx12->cbvSrvUavDescriptorSize;

	// Create SRVs for the command buffers.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.NumElements = gpuSize;
	srvDesc.Buffer.StructureByteStride = sizeof(DrawIndirectArgs);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	//srvDesc.Buffer.FirstElement = frame * heapSize;

	dx12->myDevice->CreateShaderResourceView(resource.Get(), &srvDesc, handle);
	uavHandle = handle;

	handle.ptr += dx12->cbvSrvUavDescriptorSize;

	for (size_t i = 0; i < FrameCount; i++)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavArgsDesc = {};
		uavArgsDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavArgsDesc.Buffer.FirstElement = 0;
		// uavArgsDesc.Buffer.NumElements = 1;
		uavArgsDesc.Buffer.NumElements = gpuSize;
		uavArgsDesc.Buffer.StructureByteStride = sizeof(DrawIndirectArgs);
		uavArgsDesc.Buffer.CounterOffsetInBytes = 0;
		uavArgsDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

		dx12->myDevice->CreateUnorderedAccessView(indirectArgsBuffer[i].Get(), nullptr, &uavArgsDesc, handle);
		uavArgsHandle[i] = handle;
	}

	//for (UINT frame = 0; frame < aDx12->FrameCount; frame++)
	//{
	//	srvDesc.Buffer.FirstElement = frame * heapSize;
	//	aDx12->myDevice->CreateShaderResourceView(inputCommandBuffer.Get(), &srvDesc, handle);
	//	handle.Offset(1, aDx12->cbvSrvUavDescriptorSize);

	//	srvDesc.Buffer.FirstElement = frame * heapSize;
	//	aDx12->myDevice->CreateUnorderedAccessView(indirectArgsBuffer.Get(), nullptr, &uavArgsDesc, handle);
	//	handle.Offset(1, aDx12->cbvSrvUavDescriptorSize);
	//}
}

UINT MeshRenderer::GetFrameGroupCount(size_t aSize)
{
	return static_cast<UINT>((aSize + 63) / 64);
}
