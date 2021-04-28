#include "Core.hpp"

#include "engine/managers/EntityManager.hpp"

#include "engine/systems/ImGuiSystem.hpp"
#include "engine/systems/InputSystem.hpp"
#include "engine/systems/RenderingSystem.hpp"
#include "engine/systems/ScriptingSystem.hpp"

#include "engine/utils/CPUTimer.hpp"
#include "engine/utils/Importer.hpp"

// #include "thirdparty/imgui/imgui_impl_glfw.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
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


	auto& debugState = GlobalStateManager::getWritable<DebugState>();
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


	auto inputSystem = std::make_shared<InputSystem>();
	inputSystem->setWindow(glfwWindow);

	auto imGuiSystem = std::make_shared<ImGuiSystem>();
	imGuiSystem->setWindow(glfwWindow);

	auto scriptingSystem = std::make_shared<ScriptingSystem>();

	auto renderingSystem = std::make_shared<RenderingSystem>();
	renderingSystem->setWindow(glfwWindow);


	systems.push_back(std::static_pointer_cast<SystemBase>(inputSystem));
	systems.push_back(std::static_pointer_cast<SystemBase>(imGuiSystem));
	systems.push_back(std::static_pointer_cast<SystemBase>(scriptingSystem));
	systems.push_back(std::static_pointer_cast<SystemBase>(renderingSystem));


	// Each system has at least 1 timer
	debugState.executionTimeArrays.resize(systems.size(), std::vector<DebugState::ExecutionTime>(1));
	debugState.executionTimeArrays[0][0].name = "InputSystem";
	debugState.executionTimeArrays[1][0].name = "ImGuiSystem";
	debugState.executionTimeArrays[2][0].name = "ScriptingSystem";
	debugState.executionTimeArrays[3][0].name = "RenderingSystem";
	
	
	for (auto& system : systems) {
		if (system->init()) {
			return 1;
		}
	}


	ScriptManager::init();

	EntityManager::init();


	auto cameraEntity = EntityManager::createEntity<TransformComponent, ScriptComponent, CameraComponent>();
	cameraEntity.apply<TransformComponent, ScriptComponent, CameraComponent>(
		[](auto& transform, auto& script, auto& camera) {
			transform.position.z = -3.0f;

			script.handle = ScriptManager::getScriptHandle("script_camera");

			camera.viewport = { 1920.0f, 1080.0f };
		});


	auto modelEntity = EntityManager::createEntity<TransformComponent, ModelComponent, ScriptComponent>();

	std::vector<MeshManager::Handle> meshHandles {};
	if (Importer::importMesh("assets/models/dragon.obj", meshHandles)) {
		return 1;
	}
	modelEntity.getComponent<ModelComponent>().meshHandles[0] = meshHandles[0];

	auto albedoTextureHandle = TextureManager::createObject(0);
	if (Importer::importTexture("assets/textures/Concrete_Panels_01_Base_Color.jpg", albedoTextureHandle, true)) {
		return 1;
	}
	auto normalTextureHandle = TextureManager::createObject(0);
	if (Importer::importTexture("assets/textures/Concrete_Panels_01_Normal.jpg", normalTextureHandle, false)) {
		return 1;
	}

	auto materialHandle = MaterialManager::createObject(0);
	materialHandle.apply([&albedoTextureHandle, &normalTextureHandle](auto& material) {
		// material.color			   = glm::vec3(0.5f, 0.3f, 0.8f);
		material.textureAlbedo = albedoTextureHandle;
		material.textureNormal = normalTextureHandle;
	});
	materialHandle.update();

	modelEntity.getComponent<ModelComponent>().materialHandles[0] = materialHandle;

	modelEntity.getComponent<ModelComponent>().shaderHandles[0] =
		GraphicsShaderManager::getHandle(meshHandles[0], materialHandle);


	modelEntity.getComponent<ScriptComponent>().handle = ScriptManager::getScriptHandle("script_floating_object");


	auto directionalLightEntity = EntityManager::createEntity<TransformComponent, LightComponent>();
	directionalLightEntity.getComponent<LightComponent>().type			 = LightComponent::Type::DIRECTIONAL;
	directionalLightEntity.getComponent<LightComponent>().color			 = { 2.00f, 1.98f, 1.95f };
	directionalLightEntity.getComponent<LightComponent>().castsShadows	 = true;
	directionalLightEntity.getComponent<TransformComponent>().rotation.x = -0.8f;
	directionalLightEntity.getComponent<TransformComponent>().rotation.y = 0.3f;
	directionalLightEntity.getComponent<TransformComponent>().rotation.z = 0.2f;


	auto tileEntity = EntityManager::createEntity<TransformComponent, ModelComponent>();
	// std::vector<MeshManager::Handle> meshHandles {};
	if (Importer::importMesh("assets/models/tile.fbx", meshHandles)) {
		return 1;
	}
	tileEntity.getComponent<ModelComponent>().meshHandles[0] = meshHandles[0];
	// materialHandle = MaterialManager::createObject(0);
	// materialHandle.apply([](auto& material){
	// 	material.color = glm::vec3(1.0);
	// });
	// materialHandle.update();
	tileEntity.getComponent<ModelComponent>().materialHandles[0] = materialHandle;
	tileEntity.getComponent<ModelComponent>().shaderHandles[0] =
		GraphicsShaderManager::getHandle(meshHandles[0], materialHandle);


	spdlog::info("Initialization completed successfully");

	return 0;
}

int Core::run() {
	double dt = 0.0;

	uint frameCount		= 0;
	double cumulativeDT = 0.0;

	auto timeNow = std::chrono::high_resolution_clock::now();

	CPUTimer timer {};
	
	auto& debugState = GlobalStateManager::getWritable<DebugState>();
	DebugState::ExecutionTimeArrays cumulativeExecutionTimeArrays = debugState.executionTimeArrays;

	while (!glfwWindowShouldClose(glfwWindow)) {
		auto& debugState = GlobalStateManager::getWritable<DebugState>();

		auto timeLast = timeNow;
		timeNow		  = std::chrono::high_resolution_clock::now();

		dt = std::chrono::duration_cast<std::chrono::duration<double>>(timeNow - timeLast).count();

		if (dt > 0.033) {
			dt = 0.033;
		}

		glfwPollEvents();

		while (logBuffer) {
			logBuffer.getline(debugState.logs[debugState.logOffset].data(), maxLogLength);

			debugState.logOffset = (debugState.logOffset + 1) % maxLogEntries;
			if (debugState.logCount < maxLogEntries) {
				debugState.logCount++;
			}
		}

		frameCount++;
		cumulativeDT += dt;

		for (uint i = 0; i < cumulativeExecutionTimeArrays.size(); i++) {
			for (uint j = 0; j < cumulativeExecutionTimeArrays[i].size(); j++) {
				cumulativeExecutionTimeArrays[i][j].cpuTime += debugState.executionTimeArrays[i][j].cpuTime;
				cumulativeExecutionTimeArrays[i][j].gpuTime += debugState.executionTimeArrays[i][j].gpuTime;
			}
		}

		if (cumulativeDT >= 0.1f) {
			debugState.avgFrameTime = cumulativeDT / frameCount;

			debugState.avgExecutionTimeArrays = cumulativeExecutionTimeArrays;
			for (uint i = 0; i < debugState.avgExecutionTimeArrays.size(); i++) {
				for (uint j = 0; j < debugState.avgExecutionTimeArrays[i].size(); j++) {
					debugState.avgExecutionTimeArrays[i][j].cpuTime /= frameCount + 1;
					debugState.avgExecutionTimeArrays[i][j].gpuTime /= frameCount + 1;
				}
			}

			cumulativeExecutionTimeArrays = debugState.executionTimeArrays;

			frameCount	 = 0;
			cumulativeDT = 0.0;
		}

		GlobalStateManager::update();

		// update systems
		for (uint index = 0; index < systems.size(); index++) {
			timer.start();
			if (systems[index]->run(dt)) {
				// systems can request termination
				glfwSetWindowShouldClose(glfwWindow, true);
			}

			auto& executionTime	  = debugState.executionTimeArrays[index][0];
			executionTime.level	  = 0;
			executionTime.cpuTime = timer.stop();
			executionTime.gpuTime = -1.0f;
		}
	}

	spdlog::info("Main loop terminated");
	return 0;
}
} // namespace Engine