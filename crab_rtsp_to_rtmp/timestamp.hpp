#pragma once
#include <chrono>
#include <functional>
#include <cstdint>
#include <chrono>
#include <thread>

class Timestamp
{
public:
	Timestamp()
		: _beginTimePoint(std::chrono::high_resolution_clock::now())
	{ }

	void reset()
	{
		_beginTimePoint = std::chrono::high_resolution_clock::now();
	}

	int64_t elapsed()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - _beginTimePoint).count();
	}

	static std::string localtime();

private:
	std::chrono::time_point<std::chrono::high_resolution_clock> _beginTimePoint;
};