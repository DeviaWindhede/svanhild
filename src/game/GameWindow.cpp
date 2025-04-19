#include "pch.h"
#include "GameWindow.h"
#include "DX12.h"
#include <InputManager.h>
#include "CubePrimitive.h"
#include "SpherePrimitive.h"
#include "ShaderCompiler.h"
#include <mesh/ModelFactory.h>
#include <StringHelper.h>
#include <iostream>
#include "StringHelper.h"

GameWindow::GameWindow(UINT width, UINT height, std::wstring name) : D3D12Window(width, height, name)
{
}

GameWindow::~GameWindow()
{
	meshes.clear();
}

void GameWindow::OnInit()
{
	D3D12Window::OnInit();

	// Create and record the bundle.
	//{
	//	ThrowIfFailed(dx12.myDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, dx12.myBundleAllocator.Get(), dx12.myPipelineState.Get(), IID_PPV_ARGS(&dx12.myBundle)));
	//	dx12.myBundle->SetGraphicsRootSignature(dx12.myRootSignature.Get());

	//	dx12.myBundle->SetGraphicsRootConstantBufferView(0, frameBuffer.resource->GetGPUVirtualAddress());
	//	dx12.myBundle->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//	dx12.myBundle->IASetVertexBuffers(0, 1, &myTempMesh.VertexBufferView());
	//	dx12.myBundle->IASetIndexBuffer(&myTempMesh.IndexBufferView());
	//	dx12.myBundle->DrawIndexedInstanced(myTempMesh.VertexCount(), 1, 0, 0, 0);
	//	ThrowIfFailed(dx12.myBundle->Close());
	//}

	meshes.clear();

	meshes.push_back({});
	meshes.back().mesh = new SpherePrimitive();
	((SpherePrimitive*)meshes.back().mesh)->InitPrimitive();
	
	meshes.push_back({});
	meshes.back().mesh = new CubePrimitive();
	((CubePrimitive*)meshes.back().mesh)->InitPrimitive();
	
	
	std::vector<DrawIndirectArgs> args;
	std::vector<UINT> instanceOffsets;
	
	float offset = 5.0f;
	size_t amount = 180;
	for (int i = 0; i < meshes.size(); i++)
	{
		instanceOffsets.push_back(totalInstances.size());
		meshes[i].instanceOffset = totalInstances.size();
		
		std::vector<InstanceData> instances;
		for (int j = 0; j < amount; j++)
		{
			auto S = DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f);
			auto R = DirectX::XMMatrixRotationY(0);
			auto T = DirectX::XMMatrixTranslation(offset, offset + i * 10.0f, offset * j + 10.0f);

			instances.push_back({ S * R * T , meshes[1].mesh->Index() });
			totalInstances.push_back(instances.back());
			meshes[i].instanceCount++;
			if (i > 0)
			{
				meshes[i].mesh->verticesIndex = meshes[i - 1].mesh->verticesIndex + meshes[i - 1].mesh->VertexCount();
				meshes[i].mesh->indeciesIndex = meshes[i - 1].mesh->indeciesIndex + meshes[i - 1].mesh->IndexCount();
			}
		}
		dx12.instanceBuffer.AddItem(dx12.myDevice, instances.data(), instances.size());
	}

	for (size_t i = 0; i < instanceOffsets.size(); i++)
	{
		size_t baseVertex = meshes[i].mesh->verticesIndex;
		size_t startIndex = meshes[i].mesh->indeciesIndex;
		
		args.push_back(
		{
			.IndexCountPerInstance = static_cast<UINT>(meshes[i].mesh->IndexCount()),
			.InstanceCount = meshes[i].instanceCount,
			.StartIndexLocation =  static_cast<UINT>(startIndex),
			.BaseVertexLocation = static_cast<UINT>(baseVertex),
			.StartInstanceLocation = meshes[i].instanceOffset
		});
	}
	//meshes[1].buffer.Create(&dx12);
	
	dx12.meshRenderer.AddItem(dx12.myDevice, args.data(), args.size());
	// dx12.meshRenderer.AddCommand(args);
}

void GameWindow::OnUpdate()
{
	D3D12Window::OnUpdate();
	
	if (textures.size() == 0)
	{
		resourceLoader.LoadResource<Texture>(new Texture(), [&](Texture* aResource) {
			std::cout << &aResource << " loaded" << std::endl;
			textures.push_back(aResource);
			//((Texture*)aResource)->Bind(dx12);
			// TODO: Wait for root signature to be bound before first callback
		});
	}

	for (size_t i = 0; i < meshes.size(); i++)
	{
		if (!meshes[i].mesh->GPUInitialized())
		{
			resourceLoader.LoadResource<Mesh>(meshes[i].mesh);
		}
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


			if (im->IsKeyPressed(VK_F1))
				dx12.frameBuffer.data[0].data.renderPass = 0;
			if (im->IsKeyPressed(VK_F2))
				dx12.frameBuffer.data[0].data.renderPass = 1;
			if (im->IsKeyPressed(VK_F3))
				dx12.frameBuffer.data[0].data.renderPass = 2;
			if (im->IsKeyPressed(VK_F4))
				dx12.frameBuffer.data[0].data.renderPass = 3;
			if (im->IsKeyPressed(VK_F5))
				dx12.frameBuffer.data[0].data.renderPass = 4;
			if (im->IsKeyPressed(VK_F6))
				dx12.frameBuffer.data[0].data.renderPass = 5;
			if (im->IsKeyPressed(VK_F7))
				dx12.frameBuffer.data[0].data.renderPass = 6;
			if (im->IsKeyPressed(VK_F8))
				dx12.frameBuffer.data[0].data.renderPass = 7;


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
}

void GameWindow::OnRender()
{
#if SHOULD_RECOMPILE_DURING_RUNTIME
	{
		std::lock_guard<std::mutex> lock(ShaderCompiler::ShaderAccessMutex());
		if (ShaderCompiler::GetShadersToRecompile().size() > 0)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(16));

			auto& shaders = ShaderCompiler::GetShadersToRecompile();
			do
			{
				size_t index = shaders.front();
				shaders.pop();
				auto& shader = ShaderCompiler::GetShader(index);
				std::wstring fileName = shader.path;
				ShaderCompiler::RecompileShader(shader);
			} while (!shaders.empty());
		}
	}
#endif
	if (textures.size() > 0 && textures[0]->GPUInitialized())
	{
		for (UINT i = 0; i < textures.size(); ++i) {
			if (boundTextures[i] == textures[i]->Index())
				continue;

			if (!textures[i]->Bind(i, &dx12))
				continue;

			boundTextures[i] = textures[i]->Index();
		}
	}
	
	if (meshes.size() == 0)
		return;

	// dx12.instanceBuffer.Update(dx12, totalInstances);
	dx12.instanceBuffer.Update(dx12.myCommandList);
	dx12.meshRenderer.Update(dx12.myCommandList);
	resourceLoader.OnRender();
}

