#include "pch.h"
#include <iostream>
#include <chrono>
#include <iostream>
#include <thread>

#include "World.h"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    using namespace std::chrono_literals;

    AllocConsole();
    FILE* pCout;
    freopen_s(&pCout, "CONOUT$", "w", stdout);

	std::cout << "Test";




    char temp = 0;
    while (temp != 'q')
    {
        std::cin >> temp;
        std::this_thread::sleep_for(30ms);
    }

	return 0;
}
