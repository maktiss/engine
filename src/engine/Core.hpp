#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_LEFT_HANDED
#define GLM_DEPTH_ZERO_TO_ONE

#include "engine/systems/SystemBase.hpp"

#include <array>
#include <memory>
#include <sstream>
#include <vector>


namespace Engine {
class Core {
public:
private:
	std::vector<std::shared_ptr<Systems::SystemBase>> systems {};

	GLFWwindow* glfwWindow = nullptr;

	std::stringstream logBuffer {};

	PROPERTY(uint, "Debug", maxLogEntries, 1024);
	PROPERTY(uint, "Debug", maxLogLength, 1024);

public:
	~Core();

	int init(int argc, char** argv);

	int run();
};
} // namespace Engine