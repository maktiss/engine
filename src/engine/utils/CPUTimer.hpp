#pragma once

#include <chrono>


namespace Engine {
class CPUTimer {
private:
	std::chrono::_V2::system_clock::time_point timeStart {};

public:
	inline void start() {
		timeStart = std::chrono::high_resolution_clock::now();
	}

	inline double stop() {
		auto timeNow = std::chrono::high_resolution_clock::now();
		return std::chrono::duration_cast<std::chrono::duration<double>>(timeNow - timeStart).count();
	}
};
} // namespace Engine