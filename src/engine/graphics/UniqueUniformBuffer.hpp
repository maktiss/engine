#pragma once

#define VULKAN_HPP_NO_EXCEPTIONS 1
#include <vulkan/vulkan.hpp>

#include "vk_mem_alloc.h"


namespace Engine::Graphics {
// Uniform buffer designed for exclusive usage, uses its own descriptor pool, layout and set, with binding == 0
class UniqueUniformBuffer {
private:
	vk::Device vkDevice {};
	VmaAllocator vmaAllocator {};

	vk::Buffer vkBuffer {};
	VmaAllocation vmaAllocation {};

	vk::DeviceSize vkBufferSize {};

	vk::DescriptorPool vkDescriptorPool {};
	vk::DescriptorSetLayout vkDescriptorSetLayout {};
	vk::DescriptorSet vkDescriptorSet {};

public:
	int init(vk::Device device, VmaAllocator allocator, vk::DeviceSize size);
	int update(void* pData, uint64_t size);

	void dispose();


	inline const auto& getVkDescriptorSetLayout() const {
		return vkDescriptorSetLayout;
	}

	inline const auto& getVkDescriptorSet() const {
		return vkDescriptorSet;
	}
};
}