#include "pch.h"
#include "MeshRenderer.h"

#include "ShaderCompiler.h"
#include "d3dx/d3d12/DX12.h"
/*
 * TODO
 * fyll output commands med input (gör swappen)
 * kör reset compute
 */


MeshRenderer::MeshRenderer()
{
}

MeshRenderer::~MeshRenderer()
{
	Cleanup();
}

void MeshRenderer::Cleanup()
{
	for (UINT i = 0; i < RenderConstants::FrameCount; i++)
	{
		buffers[i].Cleanup();
	}
}

void MeshRenderer::Create(DX12* aDx12)
{
	for (UINT i = 0; i < RenderConstants::FrameCount; i++)
	{
		buffers[i].Create(aDx12, MIN_BUFFER_CONTENT_SIZE, i);
	}
	dx12 = aDx12;
}

void MeshRenderer::Update(ComPtr<ID3D12GraphicsCommandList>& aComputeCommandList)
{
	for (UINT i = 0; i < RenderConstants::FrameCount; i++)
	{
		buffers[i].Update(aComputeCommandList);
	}
}

size_t MeshRenderer::AddItem(ComPtr<ID3D12Device>& aDevice, DrawIndirectArgs* aData, size_t aSize)
{
	size_t result = 0;
	for (UINT i = 0; i < RenderConstants::FrameCount; i++)
	{
		result = buffers[i].AddItem(aDevice, aData, aSize);
	}
	return result;
}

void MeshRenderer::UpdateRootConstants()
{
	rootConstants.InstanceLength = dx12->instanceBuffer.gpuSize;
	rootConstants.InstanceCapacity = dx12->instanceBuffer.cpuData.size();
	rootConstants.CommandLength = buffers[dx12->myFrameIndex].gpuSize;
	rootConstants.CommandCapacity = buffers[dx12->myFrameIndex].cpuData.size();
	
	dx12->myComputeCommandList->SetComputeRoot32BitConstants(static_cast<UINT>(ComputeRootParameters::RootConstants), sizeof(RootConstants) / sizeof(float), reinterpret_cast<void*>(&rootConstants), 0);
}

void MeshRenderer::Dispatch()
{
	if (buffers[dx12->myFrameIndex].gpuSize == 0)
		return;

	UpdateRootConstants();
	
	buffers[dx12->myFrameIndex].Reset();
		
	CD3DX12_RESOURCE_BARRIER barriers[2]
	{
		CD3DX12_RESOURCE_BARRIER::Transition(
			buffers[dx12->myFrameIndex].resource.Get(),
			D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
			buffers[dx12->myFrameIndex].defaultResourceState
		),
		CD3DX12_RESOURCE_BARRIER::Transition(
			dx12->instanceBuffer.visibleInstancesBuffer[dx12->myFrameIndex].resource.Get(),
			D3D12_RESOURCE_STATE_COMMON,
			dx12->instanceBuffer.visibleInstancesBuffer[dx12->myFrameIndex].defaultResourceState
		)
	};
            
	dx12->myComputeCommandList->ResourceBarrier(_countof(barriers), barriers);

	// TEMP
	{
		ShaderCompiler::GetPSO(2).Set(*dx12);
		UINT frameCount = RenderConstants::GetFrameGroupCount(dx12->instanceBuffer.instanceCountBuffer.size, 1);
		dx12->myComputeCommandList->Dispatch(frameCount, 1, 1);
	}
            
	ShaderCompiler::GetPSO(1).Set(*dx12);
	// ShaderCompiler::GetPSO(dx12->currentComputePSO).Set(*dx12);
	
	UINT frameCount = RenderConstants::GetFrameGroupCount(dx12->instanceBuffer.size, 64);
	dx12->myComputeCommandList->Dispatch(frameCount, 1, 1);

	CD3DX12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(buffers[dx12->myFrameIndex].resource.Get());
	dx12->myComputeCommandList->ResourceBarrier(1, &uavBarrier);
}

void MeshRenderer::ExecuteIndirectRender()
{
	if (buffers[dx12->myFrameIndex].gpuSize == 0 || !buffers[dx12->myFrameIndex].resource)
		return;
	
	{
		CD3DX12_RESOURCE_BARRIER barriers[2]
		{
			CD3DX12_RESOURCE_BARRIER::Transition(
				buffers[dx12->myFrameIndex].resource.Get(),
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT
			),
			CD3DX12_RESOURCE_BARRIER::Transition(
				dx12->instanceBuffer.visibleInstancesBuffer[dx12->myFrameIndex].resource.Get(),
				dx12->instanceBuffer.visibleInstancesBuffer[dx12->myFrameIndex].defaultResourceState,
				D3D12_RESOURCE_STATE_COMMON
			)
		};
            
		dx12->myCommandList->ResourceBarrier(_countof(barriers), barriers);
	}
	
	dx12->myCommandList->ExecuteIndirect(
		dx12->myCommandSignature.Get(), // Predefined signature for DrawIndexedInstanced
		buffers[dx12->myFrameIndex].gpuSize,                              // Execute 1 draw call
		buffers[dx12->myFrameIndex].resource.Get(),       // Buffer with draw arguments
		0,                              // Argument buffer offset
		nullptr,                         // No counters
		0
	);
}

void MeshRenderer::OnEndFrame()
{
	
}

bool MeshRenderer::IsReady() const
{
	for (UINT i = 0; i < RenderConstants::FrameCount; i++)
	{
		if (!buffers[i].uavArgsHandle.ptr)
			return false;
	}
	return true;
}
