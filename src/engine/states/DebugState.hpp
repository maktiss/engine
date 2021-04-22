#pragma once

#include <string>
#include <unordered_map>
#include <vector>


namespace Engine::States {
class DebugState {
public:
	float avgFrameTime {};
	std::unordered_map<std::string, std::vector<float>> rendererExecutionTimes {};

	uint logOffset {};
	uint logCount {};
	std::vector<std::vector<char>> logs {};
};
} // namespace Engine::States