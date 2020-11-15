#include "Core.hpp"


namespace Engine {
Core::~Core() {
	glfwDestroyWindow(glfwWindow);
	glfwTerminate();
}

int Core::init(int argc, char** argv) {
	// loggers initialization

	std::array<spdlog::sink_ptr, 2> logSinks { std::make_shared<spdlog::sinks::ansicolor_stdout_sink_st>(),
											   std::make_shared<spdlog::sinks::basic_file_sink_st>("Engine.log") };
	spdlog::set_default_logger(std::make_shared<spdlog::logger>("logger", logSinks.begin(), logSinks.end()));
	spdlog::set_pattern("[%Y-%m-%d %T] [%^%l%$] %v");


	// window initialization

	if (!glfwInit()) {
		spdlog::critical("Failed to init GLFW");
		return 1;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	// TODO: different window types
	glfwWindow = glfwCreateWindow(1920, 1080, "Engine", glfwGetPrimaryMonitor(), nullptr);
	if (!glfwWindow) {
		spdlog::critical("Failed to create window");
		return 1;
	}


	// FIXME: what if RenderingSystem has different position in a list?
	auto renderingSystem = std::static_pointer_cast<Systems::RenderingSystem>(systems[0]);
	renderingSystem->setWindow(glfwWindow);


	// systems initialization

	for (auto& system : systems) {
		if (system->init()) {
			return 1;
		}
	}


	spdlog::info("Initialization completed successfully");

	return 0;
}

int Core::run() {
	double dt = 0.0;
	while (!glfwWindowShouldClose(glfwWindow)) {
		glfwPollEvents();

		// TODO: calculate dt

		// update systems
		for (auto& system : systems) {
			if (system->run(dt)) {
				// systems can request termination
				glfwSetWindowShouldClose(glfwWindow, true);
			}
		}
	}

	spdlog::info("Main loop terminated successfully");
	return 0;
}
} // namespace Engine