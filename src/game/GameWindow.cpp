#include "pch.h"
#include "GameWindow.h"
#include "DX12.h"
#include <InputManager.h>
#include "CubePrimitive.h"
#include <mesh/ModelFactory.h>
#include <StringHelper.h>

GameWindow::GameWindow(UINT width, UINT height, std::wstring name) : D3D12Window(width, height, name)
{
}

void GameWindow::OnInit()
{
	D3D12Window::OnInit();

	// Create and record the bundle.
	{
		ThrowIfFailed(dx12.myDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, dx12.myBundleAllocator.Get(), dx12.myPipelineState.Get(), IID_PPV_ARGS(&dx12.myBundle)));
		dx12.myBundle->SetGraphicsRootSignature(dx12.myRootSignature.Get());

		dx12.myBundle->SetGraphicsRootConstantBufferView(0, frameBuffer.resource->GetGPUVirtualAddress());
		dx12.myBundle->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		dx12.myBundle->IASetVertexBuffers(0, 1, &myTempMesh.VertexBufferView());
		dx12.myBundle->IASetIndexBuffer(&myTempMesh.IndexBufferView());
		dx12.myBundle->DrawIndexedInstanced(myTempMesh.VertexCount(), 1, 0, 0, 0);
		ThrowIfFailed(dx12.myBundle->Close());
	}
}

void GameWindow::OnUpdate()
{

	if (!myTempMesh.GPUInitialized())
	{
		//auto package = ModelFactory::LoadMeshFromFBX(StringHelper::ws2s(GetAssetFullPath(L"TGE.fbx")));
		//auto package = ModelFactory::LoadMeshFromFBX(StringHelper::ws2s(GetAssetFullPath(L"sm_oneTrueCube.fbx")));

		//myTempMesh.LoadMeshData(package.meshData[0].vertices, package.meshData[0].indices);
		myTempMesh.InitPrimitive();
		resourceLoader.LoadResource(&myTempMesh);
		resourceLoader.LoadResource(&myTempTexture);
	}

	const float translationSpeed = 0.005f;
	const float offsetBounds = 1.f;


	{
		auto* im = InputManager::GetInstance();

		if (im->IsKeyHeld(VK_ESCAPE) && im->IsKeyHeld(VK_SHIFT))
		{
			Quit();
		}

		{
			float xDelta = 0.0f;
			float yDelta = 0.0f;
			if (im->IsKeyHeld(VK_LEFT))
			{
				yDelta += -1.0f * _timer.GetDeltaTime();
			}
			if (im->IsKeyHeld(VK_RIGHT))
			{
				yDelta += 1.0f * _timer.GetDeltaTime();
			}

			if (im->IsKeyHeld(VK_UP))
			{
				xDelta += -1.0f * _timer.GetDeltaTime();
			}
			if (im->IsKeyHeld(VK_DOWN))
			{
				xDelta += 1.0f * _timer.GetDeltaTime();
			}

			camera.pitch += xDelta;
			camera.yaw += yDelta;

			camera.pitch = std::clamp(camera.pitch, -DirectX::XM_PIDIV2, DirectX::XM_PIDIV2);
			camera.yaw = std::fmod(camera.yaw, DirectX::XM_2PI);
		}

		{
			float multiplier = im->IsKeyHeld(VK_SHIFT) ? 10.0f : 1.0f;

			Vector3f moveDirection = {};

			if (im->IsKeyHeld('W'))
			{
				moveDirection.z += 1.0f * _timer.GetDeltaTime();
			}
			if (im->IsKeyHeld('S'))
			{
				moveDirection.z += -1.0f * _timer.GetDeltaTime();
			}
			if (im->IsKeyHeld('A'))
			{
				moveDirection.x += -1.0f * _timer.GetDeltaTime();
			}
			if (im->IsKeyHeld('D'))
			{
				moveDirection.x += 1.0f * _timer.GetDeltaTime();
			}
			if (im->IsKeyHeld('E'))
			{
				moveDirection.y += 1.0f * _timer.GetDeltaTime();
			}
			if (im->IsKeyHeld('Q'))
			{
				moveDirection.y += -1.0f * _timer.GetDeltaTime();
			}



			Vector3f right = camera.Right();
			Vector3f up = camera.Up();
			Vector3f forward = camera.Forward();

			Vector3f finalMoveDirection = { 0, 0, 0 };

			finalMoveDirection = {
				finalMoveDirection.x + right.x * moveDirection.x * multiplier,
				finalMoveDirection.y + right.y * moveDirection.x * multiplier,
				finalMoveDirection.z + right.z * moveDirection.x * multiplier
			};
			finalMoveDirection = {
				finalMoveDirection.x + up.x * moveDirection.y * multiplier,
				finalMoveDirection.y + up.y * moveDirection.y * multiplier,
				finalMoveDirection.z + up.z * moveDirection.y * multiplier
			};
			finalMoveDirection = {
				finalMoveDirection.x + forward.x * moveDirection.z * multiplier,
				finalMoveDirection.y + forward.y * moveDirection.z * multiplier,
				finalMoveDirection.z + forward.z * moveDirection.z * multiplier
			};

			camera.position = {
				camera.position.x + finalMoveDirection.x,
				camera.position.y + finalMoveDirection.y,
				camera.position.z + finalMoveDirection.z
			};
		}
	}

	{
		auto S = DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f);
		//auto R = DirectX::XMMatrixRotationY(std::sin(_timer.GetTotalTime()));
		auto R = DirectX::XMMatrixRotationY(0);
		auto T = DirectX::XMMatrixTranslation(0, 0, 10);
		frameBuffer.frameBufferData.testTransform = S * R * T;

		frameBuffer.frameBufferData.offset.x += translationSpeed;
		if (frameBuffer.frameBufferData.offset.x > offsetBounds)
		{
			frameBuffer.frameBufferData.offset.x = -offsetBounds;
		}
	}
}

void GameWindow::OnRender()
{
	if (myTempMesh.GPUInitialized())
	{
		myTempTexture.Bind(dx12);
		dx12.myCommandList->IASetVertexBuffers(0, 1, &myTempMesh.VertexBufferView());
		dx12.myCommandList->IASetIndexBuffer(&myTempMesh.IndexBufferView());
		//dx12.myCommandList->DrawInstanced(myTempMesh.VertexCount(), 1, 0, 0);

		{
			size_t vertexCount = myTempMesh.VertexCount();    // Number of vertices
			size_t indexCount = myTempMesh.IndexCount();     // Number of indices

			// StartIndexLocation is the number of vertices, since the index buffer starts right after the vertex buffer
			UINT startIndexLocation = static_cast<UINT>(vertexCount);

			//dx12.myCommandList->DrawIndexedInstanced(
			//	indexCount,             // Number of indices
			//	1,                      // Number of instances
			//	0,                      // Start vertex location (0 for starting at the beginning of the vertex buffer)
			//	startIndexLocation,     // Start index location (the index from where to start drawing)
			//	0                       // Start instance location (0 for no instance offset)
			//);
			dx12.myCommandList->DrawIndexedInstanced(
				indexCount,             // Number of indices
				1,                      // Number of instances
				0,                      // Start vertex location (typically 0)
				0,                      // Start index location (typically 0)
				0                       // Start instance location (typically 0)
			);
		}
	}
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
