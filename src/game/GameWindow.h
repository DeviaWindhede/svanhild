#pragma once
#include <D3D12Window.h>
#include "Mesh.h"
#include "CubePrimitive.h"
#include "Texture.h"
#include <vector>

struct TempMeshCollection
{
	Mesh* mesh;
	std::vector<GPUTransform> instances;
};

class GameWindow : public D3D12Window
{
public:
	GameWindow(UINT width, UINT height, std::wstring name);

	virtual void OnInit() override;
	virtual void OnUpdate() override;
	virtual void OnRender() override;

private:
	std::vector<TempMeshCollection> meshes;
	Texture myTempTexture;
};

