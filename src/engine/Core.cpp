#include "Core.hpp"

#include "engine/managers/EntityManager.hpp"

#include "engine/systems/ImGuiSystem.hpp"
#include "engine/systems/InputSystem.hpp"
#include "engine/systems/RenderingSystem.hpp"
#include "engine/systems/ScriptingSystem.hpp"

#include "engine/utils/Importer.hpp"

// #include "thirdparty/imgui/imgui_impl_glfw.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/spdlog.h>

#include <chrono>


namespace Engine {
Core::~Core() {
	glfwDestroyWindow(glfwWindow);
	glfwTerminate();
}

int Core::init(int argc, char** argv) {
	// loggers initialization

	std::array<spdlog::sink_ptr, 3> logSinks { std::make_shared<spdlog::sinks::ansicolor_stdout_sink_st>(),
											   std::make_shared<spdlog::sinks::basic_file_sink_st>("Engine.log"),
											   std::make_shared<spdlog::sinks::ostream_sink_st>(logBuffer) };
	spdlog::set_default_logger(std::make_shared<spdlog::logger>("logger", logSinks.begin(), logSinks.end()));
	spdlog::set_pattern("[%Y-%m-%d %T] [%^%l%$] %v");


	auto& debugState = Engine::Managers::GlobalStateManager::getWritable<Engine::States::DebugState>();
	debugState.logs.resize(maxLogEntries, std::vector<char>(maxLogLength));


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

	auto imGuiSystem = std::make_shared<Systems::ImGuiSystem>();
	imGuiSystem->setWindow(glfwWindow);

	auto scriptingSystem = std::make_shared<Systems::ScriptingSystem>();

	auto renderingSystem = std::make_shared<Systems::RenderingSystem>();
	renderingSystem->setWindow(glfwWindow);


	systems.push_back(std::static_pointer_cast<Systems::SystemBase>(inputSystem));
	systems.push_back(std::static_pointer_cast<Systems::SystemBase>(imGuiSystem));
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
		material.textureAlbedo = albedoTextureHandle;
		material.textureNormal = normalTextureHandle;
	});
	materialHandle.update();

	modelEntity.getComponent<Components::Model>().materialHandles[0] = materialHandle;

	modelEntity.getComponent<Components::Model>().shaderHandles[0] =
		Engine::Managers::GraphicsShaderManager::getHandle(meshHandles[0], materialHandle);


	modelEntity.getComponent<Components::Script>().handle =
		Engine::Managers::ScriptManager::getScriptHandle("script_floating_object");


	auto directionalLightEntity =
		Engine::Managers::EntityManager::createEntity<Components::Transform, Components::Light>();
	directionalLightEntity.getComponent<Components::Light>().type			= Components::Light::Type::DIRECTIONAL;
	directionalLightEntity.getComponent<Components::Light>().color			= { 2.00f, 1.98f, 1.95f };
	directionalLightEntity.getComponent<Components::Light>().castsShadows	= true;
	directionalLightEntity.getComponent<Components::Transform>().rotation.x = -0.8f;
	directionalLightEntity.getComponent<Components::Transform>().rotation.y = 0.3f;
	directionalLightEntity.getComponent<Components::Transform>().rotation.z = 0.2f;



	auto tileEntity =
		Engine::Managers::EntityManager::createEntity<Components::Transform, Components::Model>();
	// std::vector<Engine::Managers::MeshManager::Handle> meshHandles {};
	if (Engine::Utils::Importer::importMesh("assets/models/tile.fbx", meshHandles)) {
		return 1;
	}
	tileEntity.getComponent<Components::Model>().meshHandles[0] = meshHandles[0];
	// materialHandle = Engine::Managers::MaterialManager::createObject(0);
	// materialHandle.apply([](auto& material){
	// 	material.color = glm::vec3(1.0);
	// });
	// materialHandle.update();
	tileEntity.getComponent<Components::Model>().materialHandles[0] = materialHandle;
	tileEntity.getComponent<Components::Model>().shaderHandles[0] =
		Engine::Managers::GraphicsShaderManager::getHandle(meshHandles[0], materialHandle);


	spdlog::info("Initialization completed successfully");

	return 0;
}

int Core::run() {
	double dt = 0.0;

	uint frameCount = 0;
	double cumulativeDT = 0.0;

	auto timeNow = std::chrono::high_resolution_clock::now();

	while (!glfwWindowShouldClose(glfwWindow)) {
		auto timeLast = timeNow;
		timeNow		  = std::chrono::high_resolution_clock::now();

		dt = std::chrono::duration_cast<std::chrono::duration<double>>(timeNow - timeLast).count();

		if (dt > 0.033) {
			dt = 0.033;
		}

		glfwPollEvents();

		auto& debugState = Engine::Managers::GlobalStateManager::getWritable<Engine::States::DebugState>();

		while(logBuffer) {
			logBuffer.getline(debugState.logs[debugState.logOffset].data(), maxLogLength);

			debugState.logOffset = (debugState.logOffset + 1) % maxLogEntries;
			if (debugState.logCount < maxLogEntries) {
				debugState.logCount++;
			}
		}

		frameCount++;
		cumulativeDT += dt;
		if (cumulativeDT >= 0.1f) {
			debugState.avgFrameTime = cumulativeDT / frameCount;

			frameCount = 0;
			cumulativeDT = 0.0;
		}

		Engine::Managers::GlobalStateManager::update();

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