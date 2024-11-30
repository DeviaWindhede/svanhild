#include "pch.h"
#include <WinBase.h>
#include "GameWindow.h"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    GameWindow sample(1280, 720, L"svanhild engine");
    return Win32Application::Run(&sample, hInstance, nCmdShow);
}
