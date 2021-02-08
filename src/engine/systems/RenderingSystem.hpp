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
#include <vector>


#define RETURN_IF_VK_ERROR(result, msg)                                                            \
	{                                                                                              \
		auto resultValue = result;                                                                 \
		if (resultValue != vk::Result::eSuccess) {                                                 \
			spdlog::error("[vulkan] {}. Error code: {} ({})", msg, result, vk::to_string(result)); \
			return 1;                                                                              \
		}                                                                                          \
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


	struct RenderGraphNodeReference {
		uint rendererIndex	 = -1;
		uint attachmentIndex = -1;

		// Specifies whenever a resource would be used by renderer next frame and sould be preserved
		bool nextFrame = false;
	};

	struct RenderGraphNode {
		// One output can be connected to multiple inputs but to only one output
		std::vector<std::set<RenderGraphNodeReference>> inputReferenceSets {};
		std::vector<RenderGraphNodeReference> outputReferences {};
	};

	// Helper class to simplfy render graph description
	class RenderGraph {
	public:
		struct NodeReference {
			uint rendererIndex	 = -1;
			uint attachmentIndex = -1;

			// Specifies whenever a resource would be used by renderer next frame and sould be preserved
			bool nextFrame = false;


			bool operator<(const NodeReference& other) const {
				return rendererIndex < other.rendererIndex;
			}

			bool operator==(const NodeReference& other) const {
				return rendererIndex == other.rendererIndex;
			}
		};

		struct Node {
			// One output can be connected to multiple inputs but to only one output
			std::vector<std::set<NodeReference>> inputReferenceSets {};
			std::vector<NodeReference> outputReferences {};

			// References to what is connected to input/output slot
			std::vector<NodeReference> backwardInputReferences {};
			std::vector<NodeReference> backwardOutputReferences {};
		};

		std::vector<Node> nodes {};

	public:
		inline void setNodeCount(uint count) {
			nodes.resize(count);
		}

		inline void setInputCount(uint nodeIndex, uint count) {
			nodes[nodeIndex].backwardInputReferences.resize(count);
		}

		inline void setOutputCount(uint nodeIndex, uint count) {
			nodes[nodeIndex].inputReferenceSets.resize(count);
			nodes[nodeIndex].outputReferences.resize(count);

			nodes[nodeIndex].backwardOutputReferences.resize(count);
		}

		inline const auto& getNodes() const {
			return nodes;
		}

		// Connects source output to destination input slot
		void addInputConnection(uint srcNodeIndex, uint srcOutputIndex, uint dstNodeIndex, uint dstInputIndex,
									  bool nextFrame = false);

		// Connects source output to destination output slot
		void addOutputConnection(uint srcNodeIndex, uint srcOutputIndex, uint dstNodeIndex, uint dstOutputIndex,
									  bool nextFrame = false);
	};


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
	uint currentCommandPool = 0;

	std::vector<vk::CommandBuffer> vkCommandBuffers {};
	std::vector<vk::Fence> vkCommandBufferFences {};

	std::vector<vk::Semaphore> vkImageAvailableSemaphores {};
	std::vector<vk::Semaphore> vkImageBlitFinishedSemaphores {};

	// For every frame in flight for every renderer
	std::vector<std::vector<vk::Semaphore>> vkRenderPassWaitSemaphores {};
	std::vector<std::vector<vk::Semaphore>> vkRenderPassSignalSemaphores {};

	std::vector<vk::CommandBuffer> vkImageBlitCommandBuffers {};
	std::vector<vk::Fence> vkImageBlitCommandBufferFences {};

	struct Sync {
		// Semaphores per renderer
		std::vector<std::vector<vk::Semaphore>> waitSemaphores;
		std::vector<std::vector<vk::Semaphore>> signalSemaphores;
	};

	// Sync objects per frame-in-flight
	std::vector<Sync> syncObjects {};

	Engine::Managers::TextureManager::Handle finalTextureHandle {};

	std::vector<const char*> requiredInstanceExtensionNames = {};
	std::vector<const char*> requiredDeviceExtensionNames	= { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

	std::vector<vk::PhysicalDevice> vkSupportedPhysicalDevices;
	uint32_t activePhysicalDeviceIndex = 0;

	VmaAllocator vmaAllocator = nullptr;

	std::vector<std::shared_ptr<Engine::Renderers::RendererBase>> renderers {};

	// // The graph is positioned from left to right
	// std::vector<RenderGraphNode> renderGraph {};

	RenderGraph renderGraph {};

	// Compiled from render graph
	std::vector<uint> rendererExecutionOrder {};

	// Output to blit from into swapchain image
	RenderGraphNodeReference finalOutputReference {};


public:
	~RenderingSystem() {
		vkDevice.waitIdle();

		for (auto& renderer : renderers) {
			renderer->dispose();
		}

		Engine::Managers::MeshManager::destroy();
		Engine::Managers::MaterialManager::dispose();
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
		return frameIndex * renderers.size() * (threadCount + 1) + rendererIndex * (threadCount + 1) + threadIndex;
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
} // namespace Engine::Systems
