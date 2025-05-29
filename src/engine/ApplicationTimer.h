#pragma once
#include <chrono>

class ApplicationTimer
{
public:
	ApplicationTimer();
	ApplicationTimer(const ApplicationTimer& aApplicationTimer) = delete;
	ApplicationTimer& operator=(const ApplicationTimer& aApplicationTimer) = delete;
	
	void Update();

	__forceinline const float& GetDeltaTime() const { return _deltaTime; }
	__forceinline const double& GetTotalTime() const { return _totalTime; }
	__forceinline const std::chrono::high_resolution_clock::time_point& GetLastTimePoint() const { return _lastTimePoint; }
private:
	std::chrono::high_resolution_clock::time_point _lastTimePoint;
	double _totalTime;
	float _deltaTime;
};
