#pragma once

#include "SystemBase.hpp"

// FIXME: test
#include "engine/graphics/Mesh.hpp"
#include "engine/graphics/Buffer.hpp"

#include "engine/utils/IO.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <spdlog/spdlog.h>

#define VULKAN_HPP_NO_EXCEPTIONS 1
#include <vulkan/vulkan.hpp>

#include "vk_mem_alloc.h"

#include <vector>


#define RETURN_IF_VK_ERROR(result, msg) { \
	auto resultValue = result; \
	if (resultValue != vk::Result::eSuccess) { \
		spdlog::error("[vulkan] {}. Error code: {} ({})", msg, result, vk::to_string(result)); \
		return 1; \
	} \
}


namespace Engine::Systems {
class RenderingSystem : public SystemBase {
private:
	struct QueueFamilyIndices {
		uint32_t graphicsFamily = -1;
		uint32_t presentFamily	= -1;

		bool isComplete() {
			return (graphicsFamily != -1) && (presentFamily != -1);
		}
	};

	struct SwapchainSupportInfo {
		vk::SurfaceCapabilitiesKHR capabilities;
		std::vector<vk::SurfaceFormatKHR> formats;
		std::vector<vk::PresentModeKHR> presentModes;
	};


	// FIXME test
	std::vector<uint8_t> vshCode;
	std::vector<uint8_t> fshCode;

	Engine::Managers::MeshManager::Handle meshHandle;


private:
	GLFWwindow* glfwWindow = nullptr;

	vk::Instance vkInstance;
	vk::SurfaceKHR vkSurface;

	vk::Device vkDevice;

	vk::Queue vkGraphicsQueue;
	vk::Queue vkPresentQueue;

	struct SwapchainInfo {
		vk::SwapchainKHR swapchain;

		std::vector<vk::Image> images;
		vk::Format imageFormat;
		vk::Extent2D extent;

		std::vector<vk::ImageView> imageViews;

		std::vector<vk::Framebuffer> framebuffers;
	} vkSwapchainInfo;

	vk::RenderPass vkRenderPass;

	vk::CommandPool vkCommandPool;
	std::vector<vk::CommandBuffer> vkCommandBuffers;

	vk::Semaphore vkImageAvailableSemaphore;
	vk::Semaphore vkRenderingFinishedSemaphore;

	vk::PipelineLayout vkPipelineLayout;
	vk::Pipeline vkPipeline;

	std::vector<const char*> requiredInstanceExtensionNames = {};
	std::vector<const char*> requiredDeviceExtensionNames	= { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

	std::vector<vk::PhysicalDevice> vkSupportedPhysicalDevices;
	uint32_t activePhysicalDeviceIndex = 0;

	VmaAllocator vmaAllocator = nullptr;

public:
	~RenderingSystem() {
		Engine::Managers::MeshManager::destroy();
		vmaDestroyAllocator(vmaAllocator);
	}

	int init() override;
	int run(double dt) override;

	void setWindow(GLFWwindow* pGLFWWindow);

private:
	int initVkInstance();
	int enumeratePhysicalDevices();

	int createLogicalDevice();

	int initVulkanMemoryAllocator();

	int createSwapchain();

	int present();

	bool isPhysicalDeviceSupported(vk::PhysicalDevice physicalDevice) const;
	QueueFamilyIndices getQueueFamilies(vk::PhysicalDevice physicalDevice) const;

	bool checkDeviceExtensionsSupport(vk::PhysicalDevice physicalDevice) const;
	SwapchainSupportInfo getSwapchainSupportInfo(vk::PhysicalDevice physicalDevice) const;

	void addRequiredInstanceExtensionNames(const std::vector<const char*>& extensionNames);
	void addRequiredDeviceExtensionNames(const std::vector<const char*>& extensionNames);

	inline vk::PhysicalDevice& getActivePhysicalDevice() {
		return vkSupportedPhysicalDevices[activePhysicalDeviceIndex];
	}
};
} // namespace Engine::Systems
