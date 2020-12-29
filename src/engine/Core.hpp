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

#include "engine/systems/RenderingSystem.hpp"

#include <array>
#include <memory>


namespace Engine {
class Core {
public:

private:
	std::array<std::shared_ptr<Systems::SystemBase>, 1> systems { std::make_shared<Systems::RenderingSystem>() };

	GLFWwindow* glfwWindow = nullptr;

public:
	~Core();

	int init(int argc, char** argv);

	int run();
};
} // namespace Engine