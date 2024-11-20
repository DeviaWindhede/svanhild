#include "pch.h"

#include "ApplicationTimer.h"

ApplicationTimer::ApplicationTimer()
{
    _lastTimePoint = std::chrono::high_resolution_clock::now();
    _totalTime = 0.0;
    _deltaTime = 0.0;
}

void ApplicationTimer::Update()
{
    auto currentTimePoint = std::chrono::high_resolution_clock::now();

    _deltaTime = std::chrono::duration<float>(currentTimePoint - _lastTimePoint).count();
    _totalTime += _deltaTime;

    _lastTimePoint = currentTimePoint;
}
