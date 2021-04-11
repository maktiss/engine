#pragma once

#include "ResourceManagerBase.hpp"

#include "engine/graphics/textures/Texture2D.hpp"

#include "engine/graphics/DescriptorSetArray.hpp"
#include "engine/graphics/StagingBuffer.hpp"

#include "engine/managers/ConfigManager.hpp"

#define VULKAN_HPP_NO_EXCEPTIONS 1
#include <vulkan/vulkan.hpp>

#include "vk_mem_alloc.h"


namespace Engine::Managers {
class TextureManager : public ResourceManagerBase<TextureManager, Engine::Graphics::Texture2D> {
public:
	struct TextureInfo {
		vk::Image image {};
		vk::ImageView imageView {};
		vk::Sampler sampler {};
	};


private:
	static std::vector<TextureInfo> textureInfos;

	static std::vector<VmaAllocation> allocationInfos;

	static vk::Device vkDevice;
	static VmaAllocator vmaAllocator;

	static vk::Queue vkTransferQueue;
	static vk::CommandPool vkCommandPool;

	static Engine::Graphics::StagingBuffer stagingBuffer;

	static Engine::Graphics::DescriptorSetArray descriptorSetArray;

	static vk::Sampler sampler;

	struct Properties {
		PROPERTY(float, "Graphics", anisotropy, 0.0f);

		PROPERTY(uint, "Graphics", maxTextureHandles, 1024);
		PROPERTY(uint, "Graphics", maxTextureSize, 4096);
	};

	static Properties properties;


public:
	static int init();

	static void postCreate(Handle handle);
	static void update(Handle handle);


	static inline void setVkDevice(vk::Device device) {
		vkDevice = device;
	}

	static inline void setVulkanMemoryAllocator(VmaAllocator allocator) {
		vmaAllocator = allocator;
	}

	static inline void setVkTransferQueue(vk::Queue queue) {
		vkTransferQueue = queue;
	}

	// FIXME: create own command pool
	static inline void setVkCommandPool(vk::CommandPool commandPool) {
		vkCommandPool = commandPool;
	}


	static inline TextureInfo& getTextureInfo(Handle handle) {
		return textureInfos[handle.getIndex()];
	}


	static inline auto getVkDescriptorSet() {
		return descriptorSetArray.getVkDescriptorSet(0);
	}

	static inline auto getVkDescriptorSetLayout() {
		return descriptorSetArray.getVkDescriptorSetLayout();
	}


	// Disposes of all manager allocated resources
	static void dispose();


private:
	TextureManager() {
	}

	// Destroys resource given it's index
	static void destroy(uint32_t index);
};
} // namespace Engine::Managers