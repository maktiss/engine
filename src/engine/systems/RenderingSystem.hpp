#pragma once

#include "SystemBase.hpp"

// FIXME: test
#include "engine/graphics/meshes/StaticMesh.hpp"
#include "engine/graphics/Buffer.hpp"

#include "engine/renderers/ForwardRenderer.hpp"

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

	// vk::RenderPass vkRenderPass;

	uint framesInFlightCount = 3;
	uint threadCount = 1;

	uint currentFrameInFlight = 0;

	std::vector<vk::CommandPool> vkCommandPools;
	uint currentCommandPool = 0;

	// RenderPasses and FrameBuffers for every renderer
	std::vector<vk::RenderPass> vkRenderPasses {};
	std::vector<vk::Framebuffer> vkFramebuffers {};

	std::vector<vk::CommandBuffer> vkCommandBuffers {};
	std::vector<vk::Fence> vkCommandBufferFences {};

	std::vector<vk::Semaphore> vkImageAvailableSemaphores {};
	std::vector<vk::Semaphore> vkImageBlitFinishedSemaphores {};

	std::vector<vk::CommandBuffer> vkImageBlitCommandBuffers {};
	std::vector<vk::Fence> vkImageBlitCommandBufferFences {};

	Engine::Managers::TextureManager::Handle finalTextureHandle {};

	vk::Semaphore vkImageAvailableSemaphore;
	vk::Semaphore vkRenderingFinishedSemaphore;

	// vk::PipelineLayout vkPipelineLayout;
	// vk::Pipeline vkPipeline;

	std::vector<const char*> requiredInstanceExtensionNames = {};
	std::vector<const char*> requiredDeviceExtensionNames	= { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

	std::vector<vk::PhysicalDevice> vkSupportedPhysicalDevices;
	uint32_t activePhysicalDeviceIndex = 0;

	VmaAllocator vmaAllocator = nullptr;

	Engine::Renderers::ForwardRenderer forwardRenderer;


public:
	~RenderingSystem() {
		vkDevice.waitIdle();
		Engine::Managers::MeshManager::destroy();
		Engine::Managers::TextureManager::dispose();
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

	inline uint getCommandPoolIndex(uint frameIndex, uint threadIndex) const {
		return frameIndex * (threadCount + 1) + threadIndex;
	}
	
	inline uint getCommandBufferIndex(uint frameIndex, uint rendererIndex, uint threadIndex) const {
		// FIXME: rendererCount
		uint rendererCount = 1;
		return frameIndex * rendererCount * (threadCount + 1) + rendererIndex * (threadCount + 1) + threadIndex;
	}

	int createRenderPasses();
	int createFramebuffers();

	int createRenderPass(Engine::Renderers::RendererBase* renderer, vk::RenderPass& renderPass);
	int generateGraphicsPipelines(Engine::Renderers::RendererBase* renderer);

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
