#pragma once

#include "ResourceManagerBase.hpp"

#include "engine/graphics/materials/Materials.hpp"

#define VULKAN_HPP_NO_EXCEPTIONS 1
#include <vulkan/vulkan.hpp>

#include "vk_mem_alloc.h"


namespace Engine {
class MaterialManager : public ResourceManagerBase<MaterialManager, SimpleMaterial> {
public:
	struct MaterialInfo {
		vk::Buffer uniformBuffer; // TODO: move uniformbuffer out; not needed for rendering
		vk::DescriptorSet descriptorSet;
	};


private:
	struct AllocationInfo {
		VmaAllocation vmaAllocation {};
		uint descriptorPoolIndex {};
	};

	struct DescriptorPoolInfo {
		vk::DescriptorPool vkDescriptorPool {};
		uint descriptorSetCount {};
	};


private:
	static std::vector<MaterialInfo> materialInfos;
	static std::vector<AllocationInfo> allocationInfos;

	static std::vector<DescriptorPoolInfo> descriptorPoolInfos;

	static vk::DescriptorSetLayout vkDescriptorSetLayout;

	static vk::Device vkDevice;
	static VmaAllocator vmaAllocator;


public:
	static int init();

	static void postCreate(Handle& handle);
	static void update(Handle& handle);


	static inline void setVkDevice(vk::Device device) {
		vkDevice = device;
	}

	static inline void setVulkanMemoryAllocator(VmaAllocator allocator) {
		vmaAllocator = allocator;
	}


	static inline auto getVkDescriptorSetLayout() {
		return vkDescriptorSetLayout;
	}


	static inline MaterialInfo& getMaterialInfo(const Handle& handle) {
		return materialInfos[handle.getIndex()];
	}


	static int createDescriptorPool();

	static void dispose();


private:
	MaterialManager() {};

	static void destroy(uint32_t index);
};
} // namespace Engine