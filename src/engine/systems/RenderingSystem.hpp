#pragma once

#include "SystemBase.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

#include <vector>


namespace Engine::Systems {
class RenderingSystem : public SystemBase {
private:
	GLFWwindow* glfwWindow = nullptr;

	vk::Instance vkInstance;
	vk::SurfaceKHR vkSurface;

	std::vector<const char*> requiredInstanceExtensionNames = { };
	std::vector<const char*> requiredDeviceExtensionNames = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

public:
	int init() override;
	int run(double dt) override;

	int initVkInstance();

	void setWindow(GLFWwindow* pGLFWWindow);
};
} // namespace Engine::Systems
