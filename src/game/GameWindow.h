#pragma once
#include "rendering/d3dx/d3d12/D3D12Window.h"

struct TempMeshCollection
{
	class Mesh* mesh;
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
	std::vector<class Texture*> textures;
};

