#include "Core.hpp"

#include "engine/utils/Importer.hpp"

#include <chrono>


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


	auto inputSystem = std::make_shared<Systems::InputSystem>();
	inputSystem->setWindow(glfwWindow);

	auto scriptingSystem = std::make_shared<Systems::ScriptingSystem>();

	auto renderingSystem = std::make_shared<Systems::RenderingSystem>();
	renderingSystem->setWindow(glfwWindow);


	systems.push_back(std::static_pointer_cast<Systems::SystemBase>(inputSystem));
	systems.push_back(std::static_pointer_cast<Systems::SystemBase>(scriptingSystem));
	systems.push_back(std::static_pointer_cast<Systems::SystemBase>(renderingSystem));


	// systems initialization

	for (auto& system : systems) {
		if (system->init()) {
			return 1;
		}
	}


	Engine::Managers::ScriptManager::init();

	Engine::Managers::EntityManager::init();


	auto cameraEntity =
		Engine::Managers::EntityManager::createEntity<Components::Transform, Components::Script, Components::Camera>();
	cameraEntity.apply<Components::Transform, Components::Script, Components::Camera>(
		[](auto& transform, auto& script, auto& camera) {
			transform.position.z = -3.0f;

			script.handle = Engine::Managers::ScriptManager::getScriptHandle("script_camera");

			camera.viewport = { 1920.0f, 1080.0f };
		});


	auto modelEntity =
		Engine::Managers::EntityManager::createEntity<Components::Transform, Components::Model, Components::Script>();

	// auto meshHandle = Engine::Managers::MeshManager::createObject(0);
	if (Engine::Utils::Importer::importMesh("assets/models/dragon.obj",
											modelEntity.getComponent<Components::Model>().meshHandles)) {
		return 1;
	}
	// meshHandle.update();
	// modelEntity.getComponent<Components::Model>().meshHandles.push_back(meshHandle);

	auto textureHandle = Engine::Managers::TextureManager::createObject(0);
	Engine::Utils::Importer::importTexture("assets/textures/Concrete_Panels_01_Base_Color.jpg", textureHandle);

	auto materialHandle = Engine::Managers::MaterialManager::createObject(0);
	materialHandle.apply([&textureHandle](auto& material) {
		material.color			   = glm::vec3(0.5f, 0.3f, 0.8f);
		material.textureHandles[0] = textureHandle;
	});
	materialHandle.update();
	modelEntity.getComponent<Components::Model>().materialHandles.push_back(materialHandle);

	modelEntity.getComponent<Components::Script>().handle =
		Engine::Managers::ScriptManager::getScriptHandle("script_floating_object");


	spdlog::info("Initialization completed successfully");

	return 0;
}

int Core::run() {
	double dt = 0.0;

	auto timeNow = std::chrono::high_resolution_clock::now();

	while (!glfwWindowShouldClose(glfwWindow)) {
		auto timeLast = timeNow;
		timeNow		  = std::chrono::high_resolution_clock::now();

		dt = std::chrono::duration_cast<std::chrono::duration<double>>(timeNow - timeLast).count();

		if (dt > 0.033) {
			dt = 0.033;
		}

		glfwPollEvents();

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