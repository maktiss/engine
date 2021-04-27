#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <cfloat>


namespace Engine {
class DebugState {
public:
	float avgFrameTime {};
	std::unordered_map<std::string, std::vector<float>> rendererExecutionTimes {};

	struct ExecutionTime {
		uint level {};
		std::string name {};
		float cpuTime = -FLT_MIN;
		float gpuTime = -FLT_MIN;
	};

	using ExecutionTimeArrays = std::vector<std::vector<ExecutionTime>>;

	ExecutionTimeArrays executionTimeArrays {};
	ExecutionTimeArrays avgExecutionTimeArrays {};


	uint logOffset {};
	uint logCount {};
	std::vector<std::vector<char>> logs {};
};
} // namespace Engine