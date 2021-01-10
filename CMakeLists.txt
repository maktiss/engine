cmake_minimum_required(VERSION 3.0.0)
project(Engine VERSION 0.1.0)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

include(CTest)
enable_testing()

set(CMAKE_CXX_STANDARD 17)

add_executable(
	${PROJECT_NAME}
	src/engine/Core.cpp
	src/engine/Core.hpp
	src/engine/components/Camera.hpp
	src/engine/components/ComponentBase.hpp
	src/engine/components/Model.hpp
	src/engine/components/Script.hpp
	src/engine/components/Transform.hpp
	src/engine/graphics/Buffer.cpp
	src/engine/graphics/Buffer.hpp
	src/engine/graphics/UniqueUniformBuffer.cpp
	src/engine/graphics/UniqueUniformBuffer.hpp
	src/engine/graphics/materials/MaterialBase.hpp
	src/engine/graphics/materials/SolidColorMaterial.hpp
	src/engine/graphics/meshes/MeshBase.hpp
	src/engine/graphics/meshes/StaticMesh.hpp
	src/engine/graphics/shaders/ShaderBase.hpp
	src/engine/graphics/shaders/SolidColorShader.hpp
	src/engine/graphics/textures/Texture2D.hpp
	src/engine/graphics/textures/TextureBase.hpp
	src/engine/managers/EntityManager.hpp
	src/engine/managers/EntityManagerBase.hpp
	src/engine/managers/InputManager.cpp
	src/engine/managers/InputManager.hpp
	src/engine/managers/MaterialManager.cpp
	src/engine/managers/MaterialManager.hpp
	src/engine/managers/MeshManager.cpp
	src/engine/managers/MeshManager.hpp
	src/engine/managers/ResourceManagerBase.hpp
	src/engine/managers/ScriptManager.hpp
	src/engine/managers/ScriptManagerBase.hpp
	src/engine/managers/ShaderManager.hpp
	src/engine/managers/ShaderManagerBase.hpp
	src/engine/managers/TextureManager.cpp
	src/engine/managers/TextureManager.hpp
	src/engine/renderers/ForwardRenderer.hpp
	src/engine/renderers/RendererBase.hpp
	src/engine/scripts/CameraScript.cpp
	src/engine/scripts/CameraScript.hpp
	src/engine/scripts/ScriptBase.hpp
	src/engine/systems/InputSystem.cpp
	src/engine/systems/InputSystem.hpp
	src/engine/systems/RenderingSystem.cpp
	src/engine/systems/RenderingSystem.hpp
	src/engine/systems/ScriptingSystem.cpp
	src/engine/systems/ScriptingSystem.hpp
	src/engine/systems/SystemBase.hpp
	src/engine/utils/Importer.cpp
	src/engine/utils/Importer.hpp
	src/engine/utils/IO.hpp
	src/engine/utils/VkMemAllocImpl.cpp
	src/main.cpp
)

target_include_directories(
	${PROJECT_NAME} PRIVATE
	src/
	include/
	/usr/include
)

target_link_libraries(
	${PROJECT_NAME}
	Vulkan::Vulkan
	glfw
	spdlog
	glslang
	SPIRV
	SPIRV-Tools
	SPIRV-Tools-opt
	shaderc_combined
	assimp
	${CMAKE_DL_LIBS}
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
