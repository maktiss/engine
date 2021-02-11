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

	std::vector<Engine::Managers::MeshManager::Handle> meshHandles {};
	if (Engine::Utils::Importer::importMesh("assets/models/dragon.obj", meshHandles)) {
		return 1;
	}
	modelEntity.getComponent<Components::Model>().meshHandles[0] = meshHandles[0];

	auto albedoTextureHandle = Engine::Managers::TextureManager::createObject(0);
	if (Engine::Utils::Importer::importTexture("assets/textures/Concrete_Panels_01_Base_Color.jpg", albedoTextureHandle,
											   true)) {
		return 1;
	}
	auto normalTextureHandle = Engine::Managers::TextureManager::createObject(0);
	if (Engine::Utils::Importer::importTexture("assets/textures/Concrete_Panels_01_Normal.jpg", normalTextureHandle,
											   false)) {
		return 1;
	}

	auto materialHandle = Engine::Managers::MaterialManager::createObject(0);
	materialHandle.apply([&albedoTextureHandle, &normalTextureHandle](auto& material) {
		// material.color			   = glm::vec3(0.5f, 0.3f, 0.8f);
		material.textureHandles[0] = albedoTextureHandle;
		material.textureHandles[1] = normalTextureHandle;
	});
	materialHandle.update();

	modelEntity.getComponent<Components::Model>().materialHandles[0] = materialHandle;

	modelEntity.getComponent<Components::Model>().shaderHandles[0] =
		Engine::Managers::ShaderManager::getHandle(meshHandles[0], materialHandle);


	modelEntity.getComponent<Components::Script>().handle =
		Engine::Managers::ScriptManager::getScriptHandle("script_floating_object");


	auto directionalLightEntity =
		Engine::Managers::EntityManager::createEntity<Components::Transform, Components::Light>();
	directionalLightEntity.getComponent<Components::Light>().type			= Components::Light::Type::DIRECTIONAL;
	directionalLightEntity.getComponent<Components::Light>().color			= { 2.0f, 1.3f, 0.6f };
	directionalLightEntity.getComponent<Components::Transform>().rotation.x = -0.8f;
	directionalLightEntity.getComponent<Components::Transform>().rotation.y = 0.3f;
	directionalLightEntity.getComponent<Components::Transform>().rotation.z = 0.2f;


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