#pragma once

#include "SystemBase.hpp"

// FIXME: test
#include "engine/graphics/Buffer.hpp"
#include "engine/graphics/meshes/StaticMesh.hpp"

#include "engine/renderers/RendererBase.hpp"


#include "engine/utils/IO.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <spdlog/spdlog.h>

#define VULKAN_HPP_NO_EXCEPTIONS 1
#include <vulkan/vulkan.hpp>

#include "vk_mem_alloc.h"

#include <set>
#include <unordered_map>
#include <vector>


#define RETURN_IF_VK_ERROR(result, msg)                                                            \
	{                                                                                              \
		auto resultValue = result;                                                                 \
		if (resultValue != vk::Result::eSuccess) {                                                 \
			spdlog::error("[vulkan] {}. Error code: {} ({})", msg, result, vk::to_string(result)); \
			return 1;                                                                              \
		}                                                                                          \
	}


namespace Engine {
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


	// Helper class to simplfy render graph description
	class RenderGraph {
	public:
		struct NodeReference {
			std::string rendererName {};
			std::string slotName {};

			// TODO Specifies whenever a resource will be used by renderer next frame and sould be preserved
			bool nextFrame = false;


			bool operator<(const NodeReference& other) const {
				return (rendererName < other.rendererName) ||
					   (rendererName == other.rendererName) && (slotName < other.slotName);
			}

			bool operator==(const NodeReference& other) const {
				return (rendererName == other.rendererName) && (slotName == other.slotName);
			}
		};

		struct Node {
			// One output can be connected to multiple inputs but to only one output
			std::unordered_map<std::string, std::set<NodeReference>> inputReferenceSets {};
			std::unordered_map<std::string, NodeReference> outputReferences {};

			// References to what is connected to input/output slot
			std::unordered_map<std::string, NodeReference> backwardInputReferences {};
			std::unordered_map<std::string, NodeReference> backwardOutputReferences {};
		};


	public:
		std::unordered_map<std::string, Node> nodes {};


	public:
		// Connects source output to destination input slot
		void addInputConnection(std::string srcName, std::string srcOutputName, std::string dstName,
								std::string dstInputName, bool nextFrame = false);

		// Connects source output to destination output slot
		void addOutputConnection(std::string srcName, std::string srcOutputName, std::string dstName,
								 std::string dstOutputName, bool nextFrame = false);
	};


private:
	PROPERTY(uint, "Video", windowWidth, 1920);
	PROPERTY(uint, "Video", windowHeight, 1080);
	PROPERTY(int, "Video", windowMode, 1);
	PROPERTY(int, "Video", vSync, 1);

	PROPERTY(uint, "Graphics", renderingThreadCount, 0);

	PROPERTY(uint, "Graphics", irradianceMapSize, 64);
	PROPERTY(uint, "Graphics", shadowMapSize, 2048);
	PROPERTY(uint, "Graphics", skyMapSize, 1024);

	PROPERTY(uint, "Graphics", directionalLightCascadeCount, 3);
	PROPERTY(uint, "Graphics", shadowBlurKernelSize, 5);

	PROPERTY(float, "Graphics", volumetricLightResolutionScale, 0.5);

	PROPERTY(uint, "Debug", enableValidationLayers, 0);


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
		vk::Extent2D extent;
	} vkSwapchainInfo;


	uint framesInFlightCount = 3;
	uint threadCount		 = 1;

	uint currentFrameInFlight = 0;

	std::vector<vk::CommandPool> vkCommandPools;

	std::vector<vk::QueryPool> vkTimestampQueryPools {};

	std::vector<uint64_t> timestampQueriesBuffer {};


	std::vector<vk::CommandBuffer> vkPrimaryCommandBuffers {};
	std::vector<std::pair<uint, uint>> vkPrimaryCommandBuffersViews {};
	// TODO: an actual view, not just index-count pair

	std::vector<vk::CommandBuffer> vkSecondaryCommandBuffers {};
	std::vector<std::pair<uint, uint>> vkSecondaryCommandBuffersViews {};


	std::vector<vk::Semaphore> vkRendererWaitSemaphores {};
	std::vector<std::pair<uint, uint>> vkRendererWaitSemaphoresViews {};

	std::vector<vk::Semaphore> vkRendererSignalSemaphores {};
	std::vector<std::pair<uint, uint>> vkRendererSignalSemaphoresViews {};

	std::vector<vk::Fence> vkRendererFences {};

	std::vector<vk::Semaphore> vkImageAvailableSemaphores {};
	std::vector<vk::Semaphore> vkImageBlitFinishedSemaphores {};

	std::vector<vk::CommandBuffer> vkImageBlitCommandBuffers {};
	std::vector<vk::Fence> vkImageBlitCommandBufferFences {};


	TextureManager::Handle finalTextureHandle {};

	std::vector<const char*> requiredInstanceExtensionNames = {};
	std::vector<const char*> requiredDeviceExtensionNames	= { VK_KHR_SWAPCHAIN_EXTENSION_NAME,
																VK_KHR_MULTIVIEW_EXTENSION_NAME };

	std::vector<const char*> validationLayers = {};

	std::vector<vk::PhysicalDevice> vkSupportedPhysicalDevices;
	uint32_t activePhysicalDeviceIndex = 0;

	VmaAllocator vmaAllocator = nullptr;

	std::unordered_map<std::string, std::shared_ptr<RendererBase>> renderers {};

	RenderGraph renderGraph {};

	// Compiled from render graph
	std::vector<std::string> rendererExecutionOrder {};

	// Renderer name to order index map
	std::unordered_map<std::string, uint> rendererIndexMap {};

	// Output to blit from into swapchain image
	RenderGraph::NodeReference finalOutputReference {};

	std::map<std::string, float> cpuTimings {};


public:
	~RenderingSystem() {
		vkDevice.waitIdle();

		for (auto& [rendererName, renderer] : renderers) {
			renderer->dispose();
		}

		MeshManager::destroy();
		MaterialManager::dispose();
		TextureManager::dispose();
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


	inline uint getRendererIndex(std::string rendererName) {
		assert(rendererIndexMap.find(rendererName) != rendererIndexMap.end());
		return rendererIndexMap[rendererName];
	}


	inline uint getCommandPoolIndex(uint frameIndex, uint threadIndex) const {
		return frameIndex * (threadCount + 1) + threadIndex;
	}

	inline uint getCommandBufferIndex(uint frameIndex, uint rendererIndex, uint threadIndex) const {
		return frameIndex * renderers.size() * (threadCount + 1) + rendererIndex * (threadCount + 1) + threadIndex;
	}

	inline uint getTimestampQueryPoolIndex(uint frameIndex, uint rendererIndex) {
		return frameIndex * renderers.size() + rendererIndex;
	}


	inline auto& getTimestampQueryPool(uint frameIndex, uint rendererIndex) {
		return vkTimestampQueryPools[getTimestampQueryPoolIndex(frameIndex, rendererIndex)];
	}


	inline auto& getPrimaryCommandBuffersView(uint frameIndex, uint rendererIndex) {
		return vkPrimaryCommandBuffersViews[frameIndex * renderers.size() + rendererIndex];
	}

	inline auto& getSecondaryCommandBuffersView(uint frameIndex, uint rendererIndex) {
		return vkSecondaryCommandBuffersViews[frameIndex * renderers.size() + rendererIndex];
	}


	inline auto& getRendererWaitSemaphoresView(uint frameIndex, uint rendererIndex) {
		return vkRendererWaitSemaphoresViews[frameIndex * renderers.size() + rendererIndex];
	}

	inline auto& getRendererSignalSemaphoresView(uint frameIndex, uint rendererIndex) {
		return vkRendererSignalSemaphoresViews[frameIndex * renderers.size() + rendererIndex];
	}


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
} // namespace Engine
