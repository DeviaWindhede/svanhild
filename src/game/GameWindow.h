#pragma once
#include <D3D12Window.h>
#include "Mesh.h"
#include "Texture.h"

class GameWindow : public D3D12Window
{
public:
	GameWindow(UINT width, UINT height, std::wstring name);

	virtual void OnInit() override;
	virtual void OnUpdate() override;
	virtual void OnRender() override;

private:
	Mesh myTempMesh;
	Texture myTempTexture;
};

