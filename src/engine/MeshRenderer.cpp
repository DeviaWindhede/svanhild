#include "pch.h"
#include "MeshRenderer.h"

#include "DX12.h"

static constexpr size_t DEFAULT_INSTANCE_BUFFER_SIZE = 1;

void MeshRenderer::Create(class DX12* aDx12)
{
	heapSize = DEFAULT_INSTANCE_BUFFER_SIZE; // temp
	
	if (heapSize == 0)
	{
		inputCommandBuffer = nullptr;
		indirectArgsBuffer = nullptr;
		commandBufferUpload = nullptr;

		commands.clear();
		commands.resize(0);
		return;
	}
	
	
	D3D12_HEAP_PROPERTIES heapProps = { D3D12_HEAP_TYPE_DEFAULT };
    
	// input commands
	{
		commands.resize(heapSize);
		const UINT commandBufferSize = heapSize * sizeof(DrawIndirectArgs);


		heapProps = { D3D12_HEAP_TYPE_DEFAULT };
		D3D12_RESOURCE_DESC commandBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(commandBufferSize);
		ThrowIfFailed(aDx12->myDevice->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&commandBufferDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&inputCommandBuffer)));

		// CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		// 	inputCommandBuffer.Get(),
		// 	D3D12_RESOURCE_STATE_COMMON,
		// 	D3D12_RESOURCE_STATE_COPY_DEST
		// );
		
		heapProps = { D3D12_HEAP_TYPE_UPLOAD };
		ThrowIfFailed(aDx12->myDevice->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&commandBufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&commandBufferUpload)));

		NAME_D3D12_OBJECT(inputCommandBuffer);

		UINT commandIndex = 0;
		for (UINT frame = 0; frame < commands.size(); frame++)
		{
			commands[commandIndex].BaseVertexLocation = 0;
			commands[commandIndex].IndexCountPerInstance = 36;
			commands[commandIndex].InstanceCount = 4;
			commands[commandIndex].StartIndexLocation = 0;
			commands[commandIndex].StartInstanceLocation = 0;

			commandIndex++;
		}

		// Copy data to the intermediate upload heap and then schedule a copy
		// from the upload heap to the command buffer.
		//D3D12_SUBRESOURCE_DATA commandData = {};
		//commandData.pData = reinterpret_cast<UINT8*>(&commands[0]);
		//commandData.RowPitch = commandBufferSize;
		//commandData.SlicePitch = commandData.RowPitch;

		{
			void* data = nullptr;
			ThrowIfFailed(commandBufferUpload->Map(0, nullptr, &data));
			memcpy(data, commands.data(), sizeof(DrawIndirectArgs) * commands.size());
			commandBufferUpload->Unmap(0, nullptr);

			aDx12->myCommandList->CopyBufferRegion(
				inputCommandBuffer.Get(),
				0,  // Update with proper offset for chunk
				commandBufferUpload.Get(),
				0,
				sizeof(DrawIndirectArgs) * commands.size()
			);

			CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				inputCommandBuffer.Get(),
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
			);
			aDx12->myCommandList->ResourceBarrier(1, &barrier);
		}

		//UpdateSubresources<1>(aDx12->myCommandList.Get(), inputCommandBuffer.Get(), commandBufferUpload.Get(), 0, 0, 1, &commandData);
		//aDx12->myCommandList->ResourceBarrier(1, 
		//	&keep(CD3DX12_RESOURCE_BARRIER::Transition(
		//		inputCommandBuffer.Get(),
		//		D3D12_RESOURCE_STATE_COPY_DEST,
		//		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
		//	))
		//);
	}

	
	// draw argument buffer
	{
		D3D12_RESOURCE_DESC argsDesc = {};
		argsDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		argsDesc.Width = sizeof(DrawIndirectArgs);
		// argsDesc.Width = sizeof(DrawIndirectArgs) * heapSize;
		argsDesc.Height = 1;
		argsDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		argsDesc.MipLevels = 1;
		argsDesc.DepthOrArraySize = 1;
		argsDesc.SampleDesc.Count = 1;
		argsDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		heapProps = { D3D12_HEAP_TYPE_DEFAULT };
		aDx12->myDevice->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&argsDesc,
			D3D12_RESOURCE_STATE_COMMON,
			// D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(&indirectArgsBuffer)
		);
		NAME_D3D12_OBJECT(indirectArgsBuffer);

		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			indirectArgsBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS
		);
	}

	
	{
		// CD3DX12_CPU_DESCRIPTOR_HANDLE handle(aDx12->myComputeCbvSrvUavHeap.cpuStart, 1, aDx12->cbvSrvUavDescriptorSize);
		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(aDx12->myComputeCbvSrvUavHeap.cpuStart);
		handle.ptr += aDx12->cbvSrvUavDescriptorSize;

		// Create SRVs for the command buffers.
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.NumElements = heapSize;
		srvDesc.Buffer.StructureByteStride = sizeof(DrawIndirectArgs);
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		//srvDesc.Buffer.FirstElement = frame * heapSize;

		aDx12->myDevice->CreateShaderResourceView(inputCommandBuffer.Get(), &srvDesc, handle);
		uavHandle = handle;

		handle.ptr += aDx12->cbvSrvUavDescriptorSize;

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavArgsDesc = {};
		uavArgsDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavArgsDesc.Buffer.FirstElement = 0;
		uavArgsDesc.Buffer.NumElements = 1;
		// uavArgsDesc.Buffer.NumElements = heapSize;
		uavArgsDesc.Buffer.StructureByteStride = sizeof(DrawIndirectArgs);
		uavArgsDesc.Buffer.CounterOffsetInBytes = 0;
		uavArgsDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

		aDx12->myDevice->CreateUnorderedAccessView(indirectArgsBuffer.Get(), nullptr, &uavArgsDesc, handle);
		uavArgsHandle = handle;

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
}

void MeshRenderer::Dispatch(DX12* aDx12)
{
	if (commands.size() == 0)
		return;
	
	D3D12_GPU_DESCRIPTOR_HANDLE uavGPUHandle = aDx12->myComputeCbvSrvUavHeap.gpuStart;
	uavGPUHandle.ptr += aDx12->cbvSrvUavDescriptorSize;
	D3D12_GPU_DESCRIPTOR_HANDLE uavArgsGPUHandle = uavGPUHandle;
	uavArgsGPUHandle.ptr += aDx12->cbvSrvUavDescriptorSize;

	ID3D12DescriptorHeap* descriptorHeaps[] = { aDx12->myComputeCbvSrvUavHeap.descriptorHeap };
	aDx12->myComputeCommandList->SetDescriptorHeaps(1, descriptorHeaps);
	aDx12->myComputeCommandList->SetComputeRootDescriptorTable(0, aDx12->myComputeCbvSrvUavHeap.gpuStart); // SRV
	//aDx12->myComputeCommandList->SetComputeRootDescriptorTable(1, uavGPUHandle); // UAV for visible instances
	//aDx12->myComputeCommandList->SetComputeRootDescriptorTable(2, uavArgsGPUHandle); // UAV for indirect draw args
	
	{
	
		// TODO: Reset indirectArgsBuffer here
		
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			indirectArgsBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS
		);
		aDx12->myComputeCommandList->ResourceBarrier(1, &barrier);
		
		aDx12->myComputeCommandList->Dispatch(GetFrameGroupCount(), 1, 1);
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
	if (commands.size() == 0)
		return;
	
	CD3DX12_RESOURCE_BARRIER barriers[1]
	{
		CD3DX12_RESOURCE_BARRIER::Transition(
			indirectArgsBuffer.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT
		)
	};
            
	aDx12->myCommandList->ResourceBarrier(_countof(barriers), barriers);
}

void MeshRenderer::ExecuteIndirectRender(DX12* aDx12)
{
	if (commands.size() == 0)
		return;
	
	aDx12->myCommandList->ExecuteIndirect(
		aDx12->myCommandSignature.Get(), // Predefined signature for DrawIndexedInstanced
		1,                              // Execute 1 draw call
		indirectArgsBuffer.Get(),       // Buffer with draw arguments
		0,                              // Argument buffer offset
		nullptr,                         // No counters
		0
	);
    
    
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			indirectArgsBuffer.Get(),
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

UINT MeshRenderer::GetFrameGroupCount() const
{
	return (heapSize + 63) / 64;
}
