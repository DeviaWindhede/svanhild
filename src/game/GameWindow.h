#pragma once
#include <D3D12Window.h>

class GameWindow : public D3D12Window
{
public:
	GameWindow(UINT width, UINT height, std::wstring name);

	//virtual void OnRender() override;
};

