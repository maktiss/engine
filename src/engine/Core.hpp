#pragma once

// Logging
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_LEFT_HANDED
#define GLM_DEPTH_ZERO_TO_ONE

#include "engine/managers/EntityManager.hpp"

#include "engine/systems/InputSystem.hpp"
#include "engine/systems/RenderingSystem.hpp"
#include "engine/systems/ScriptingSystem.hpp"

#include <array>
#include <memory>
#include <vector>


namespace Engine {
class Core {
public:
private:
	std::vector<std::shared_ptr<Systems::SystemBase>> systems;

	GLFWwindow* glfwWindow = nullptr;

public:
	~Core();

	int init(int argc, char** argv);

	int run();
};
} // namespace Engine