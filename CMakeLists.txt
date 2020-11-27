cmake_minimum_required(VERSION 3.0.0)
project(Engine VERSION 0.1.0)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

include(CTest)
enable_testing()

set(CMAKE_CXX_STANDARD 17)

add_executable(
	${PROJECT_NAME}
	src/main.cpp
	src/engine/Core.cpp
	src/engine/graphics/Buffer.cpp
	src/engine/managers/MeshManager.cpp
	src/engine/systems/RenderingSystem.cpp
	src/engine/utils/VkMemAllocImpl.cpp
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
	${CMAKE_DL_LIBS}
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
