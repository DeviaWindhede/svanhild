#include "pch.h"
#include "GameWindow.h"
#include "DX12.h"

GameWindow::GameWindow(UINT width, UINT height, std::wstring name) : D3D12Window(width, height, name)
{
}

//void GameWindow::OnRender()
//{
//	// Execute the commands stored in the bundle.
//	//dx12.myCommandList->ExecuteBundle(dx12.myBundle.Get());
//
//	dx12.myCommandList->IASetVertexBuffers(0, 1, &myTempMesh.VertexBufferView());
//	dx12.myCommandList->IASetIndexBuffer(&myTempMesh.IndexBufferView());
//	//dx12.myCommandList->DrawInstanced(myTempMesh.VertexCount(), 1, 0, 0);
//
//	{
//		size_t vertexCount = myTempMesh.VertexCount();    // Number of vertices
//		size_t indexCount = myTempMesh.IndexCount();     // Number of indices
//
//		// StartIndexLocation is the number of vertices, since the index buffer starts right after the vertex buffer
//		UINT startIndexLocation = static_cast<UINT>(vertexCount);
//
//		//dx12.myCommandList->DrawIndexedInstanced(
//		//	indexCount,             // Number of indices
//		//	1,                      // Number of instances
//		//	0,                      // Start vertex location (0 for starting at the beginning of the vertex buffer)
//		//	startIndexLocation,     // Start index location (the index from where to start drawing)
//		//	0                       // Start instance location (0 for no instance offset)
//		//);
//		dx12.myCommandList->DrawIndexedInstanced(
//			indexCount,             // Number of indices
//			1,                      // Number of instances
//			0,                      // Start vertex location (typically 0)
//			0,                      // Start index location (typically 0)
//			0                       // Start instance location (typically 0)
//		);
//	}
//
//}
