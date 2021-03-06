#include "Core.hpp"

#include "engine/managers/EntityManager.hpp"

#include "engine/systems/ImGuiSystem.hpp"
#include "engine/systems/InputSystem.hpp"
#include "engine/systems/RenderingSystem.hpp"
#include "engine/systems/ScriptingSystem.hpp"

#include "engine/utils/CPUTimer.hpp"
#include "engine/utils/Generator.hpp"
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
	glfwWindow = glfwCreateWindow(windowWidth, windowHeight, "Engine", glfwGetPrimaryMonitor(), nullptr);
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


	auto cameraEntity = EntityManager::createEntity<TransformComponent, ScriptComponent, CameraComponent>("camera");
	cameraEntity.apply<TransformComponent, ScriptComponent, CameraComponent>(
		[&](auto& transform, auto& script, auto& camera) {
			transform.position.z = -3.0f;

			script.handle = ScriptManager::getScriptHandle("script_camera");

			camera.viewport.x = static_cast<float>(windowWidth);
			camera.viewport.y = static_cast<float>(windowHeight);
		});


	auto modelEntity = EntityManager::createEntity<TransformComponent, ModelComponent, ScriptComponent>("dragon");

	std::vector<MeshManager::Handle> meshHandles {};
	if (Importer::importMesh("assets/models/dragon.fbx", meshHandles)) {
		return 1;
	}
	modelEntity.getComponent<ModelComponent>().meshHandles[0] = meshHandles[0];

	auto albedoTextureHandle = TextureManager::createObject<Texture2D>("rock_moss_diffuse");
	if (Importer::importTexture("assets/textures/Rock Moss_diffuse.png", albedoTextureHandle, 4, 8,
								vk::Format::eR8G8B8A8Srgb)) {
		return 1;
	}
	auto normalTextureHandle = TextureManager::createObject<Texture2D>("rock_moss_normal");
	if (Importer::importTexture("assets/textures/Rock Moss_normal.png", normalTextureHandle, 4, 8,
								vk::Format::eR8G8B8A8Unorm)) {
		return 1;
	}
	auto mraTextureHandle = TextureManager::createObject<Texture2D>("rock_moss_mra");
	if (Importer::importTexture("assets/textures/Rock Moss_mra.jpg", mraTextureHandle, 4, 8,
								vk::Format::eR8G8B8A8Srgb)) {
		return 1;
	}

	auto materialHandle = MaterialManager::createObject<SimpleMaterial>("rock_moss");
	materialHandle.apply([&](auto& material) {
		// material.color			   = glm::vec3(0.5f, 0.3f, 0.8f);
		material.textureAlbedo = albedoTextureHandle;
		material.textureNormal = normalTextureHandle;
		material.textureMRA	   = mraTextureHandle;
	});
	materialHandle.update();

	modelEntity.getComponent<ModelComponent>().materialHandles[0] = materialHandle;
	modelEntity.getComponent<ModelComponent>().shaderHandles[0] =
		GraphicsShaderManager::getHandle(meshHandles[0], materialHandle);

	modelEntity.getComponent<ScriptComponent>().handle = ScriptManager::getScriptHandle("script_floating_object");


	modelEntity = EntityManager::createEntity<TransformComponent, ModelComponent>("sphere");

	modelEntity.getComponent<TransformComponent>().position.x = 3.0f;

	// std::vector<MeshManager::Handle> meshHandles {};
	if (Importer::importMesh("assets/models/sphere.fbx", meshHandles)) {
		return 1;
	}
	modelEntity.getComponent<ModelComponent>().meshHandles[0] = meshHandles[0];

	modelEntity.getComponent<ModelComponent>().materialHandles[0] = materialHandle;
	modelEntity.getComponent<ModelComponent>().shaderHandles[0] =
		GraphicsShaderManager::getHandle(meshHandles[0], materialHandle);


	auto directionalLightEntity =
		EntityManager::createEntity<TransformComponent, LightComponent, ScriptComponent>("sun");
	directionalLightEntity.getComponent<LightComponent>().type			 = LightComponent::Type::DIRECTIONAL;
	directionalLightEntity.getComponent<LightComponent>().color			 = { 2.00f, 1.98f, 1.95f };
	directionalLightEntity.getComponent<LightComponent>().castsShadows	 = true;
	directionalLightEntity.getComponent<TransformComponent>().rotation.x = -0.8f;
	directionalLightEntity.getComponent<TransformComponent>().rotation.y = 0.3f;
	directionalLightEntity.getComponent<TransformComponent>().rotation.z = 0.2f;
	directionalLightEntity.getComponent<ScriptComponent>().handle =
		ScriptManager::getScriptHandle("script_sun_movement");


	// auto tileEntity = EntityManager::createEntity<TransformComponent, ModelComponent>("tile");
	// std::vector<MeshManager::Handle> meshHandles {};
	if (Importer::importMesh("assets/models/tile.fbx", meshHandles)) {
		return 1;
	}
	// tileEntity.getComponent<ModelComponent>().meshHandles[0] = meshHandles[0];


	albedoTextureHandle = TextureManager::createObject<Texture2D>("mud_with_vegetation_albedo");
	if (Importer::importTexture("assets/textures/mud_with_vegetation_albedo.png", albedoTextureHandle, 4, 8,
								vk::Format::eR8G8B8A8Srgb)) {
		return 1;
	}
	normalTextureHandle = TextureManager::createObject<Texture2D>("mud_with_vegetation_normal");
	if (Importer::importTexture("assets/textures/mud_with_vegetation_normal.png", normalTextureHandle, 4, 8,
								vk::Format::eR8G8B8A8Unorm)) {
		return 1;
	}
	mraTextureHandle = TextureManager::createObject<Texture2D>("mud_with_vegetation_mra");
	if (Importer::importTexture("assets/textures/mud_with_vegetation_mra.png", mraTextureHandle, 4, 8,
								vk::Format::eR8G8B8A8Srgb)) {
		return 1;
	}

	materialHandle = MaterialManager::createObject<SimpleMaterial>("mud_with_vegetation");
	materialHandle.apply([&](auto& material) {
		// material.color			   = glm::vec3(0.5f, 0.3f, 0.8f);
		material.textureAlbedo = albedoTextureHandle;
		material.textureNormal = normalTextureHandle;
		material.textureMRA	   = mraTextureHandle;
	});
	materialHandle.update();


	// tileEntity.getComponent<ModelComponent>().materialHandles[0] = materialHandle;
	// tileEntity.getComponent<ModelComponent>().shaderHandles[0] =
	// 	GraphicsShaderManager::getHandle(meshHandles[0], materialHandle);


	// uint index = 0;
	// for (int x = -100; x <= 100; x++) {
	// 	for (int z = -100; z <= 100; z++) {
	// 		index++;
	// 		tileEntity =
	// 			EntityManager::createEntity<TransformComponent, ModelComponent>("tile_" + std::to_string(index));
	// 		tileEntity.getComponent<ModelComponent>().meshHandles[0]	 = meshHandles[0];
	// 		tileEntity.getComponent<ModelComponent>().materialHandles[0] = materialHandle;
	// 		tileEntity.getComponent<ModelComponent>().shaderHandles[0] =
	// 			GraphicsShaderManager::getHandle(meshHandles[0], materialHandle);

	// 		tileEntity.apply<TransformComponent>([=](auto& transform) {
	// 			transform.position.x = x * 11;
	// 			transform.position.z = z * 11;

	// 			transform.position.y = glm::length(glm::vec2(x, z)) * 0.5;

	// 			transform.rotation.x = -z * 0.1;
	// 			transform.rotation.z = x * 0.1;
	// 		});
	// 	}
	// }


	auto& terrainState = GlobalStateManager::getWritable<TerrainState>();

	terrainState.size				= 4096;
	terrainState.maxHeight			= 512;
	terrainState.materialHandles[0] = materialHandle;
	terrainState.shaderHandles[0]	= GraphicsShaderManager::getHandle<TerrainMesh>(materialHandle);
	terrainState.heightMapHandle	= TextureManager::createObject<Texture2D>("terrain_height");
	if (Importer::importTexture("assets/textures/terrain_05_height.png", terrainState.heightMapHandle, 1, 16,
								vk::Format::eR16Unorm)) {
		return 1;
	}

	terrainState.normalMapHandle = TextureManager::createObject<Texture2D>("terrain_normal");
	Generator::normalMapFromHeight(terrainState.heightMapHandle, terrainState.normalMapHandle, terrainState.maxHeight);


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