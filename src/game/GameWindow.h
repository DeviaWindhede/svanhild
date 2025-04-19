#pragma once
#include <D3D12Window.h>
#include "Mesh.h"
#include "CubePrimitive.h"
#include "Texture.h"
#include "InstanceBuffer.h"
#include <vector>
#include <array>

struct TempMeshCollection
{
	Mesh* mesh;
	UINT instanceOffset;
	UINT instanceCount;
};

class GameWindow : public D3D12Window
{
public:
	GameWindow(UINT width, UINT height, std::wstring name);
	~GameWindow();

	virtual void OnInit() override;
	virtual void OnUpdate() override;
	virtual void OnRender() override;

private:
	std::vector<InstanceData> totalInstances;
	std::vector<TempMeshCollection> meshes;
	std::vector<Texture*> textures;
	std::array<UINT, 8> boundTextures{ UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX };
};

