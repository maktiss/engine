#pragma once

#define VULKAN_HPP_NO_EXCEPTIONS 1
#include <vulkan/vulkan.hpp>

#include "vk_mem_alloc.h"

#include <vector>


namespace Engine::Graphics {
// Uniform buffer designed for exclusive usage, uses its own descriptor pool, layout and set, with binding == 0
class UniqueUniformBuffer {
private:
	vk::Device vkDevice {};
	VmaAllocator vmaAllocator {};

	uint setCount {};

	std::vector<vk::Buffer> vkBuffers {};
	std::vector<VmaAllocation> vmaAllocations {};

	vk::DeviceSize vkBufferSize {};

	vk::DescriptorPool vkDescriptorPool {};
	vk::DescriptorSetLayout vkDescriptorSetLayout {};
	std::vector<vk::DescriptorSet> vkDescriptorSets {};

public:
	int init(vk::Device device, VmaAllocator allocator, vk::DeviceSize size, uint setCount);
	int update(uint index, void* pData, uint64_t size);

	void dispose();


	inline const auto& getVkDescriptorSetLayout() const {
		return vkDescriptorSetLayout;
	}

	inline const auto& getVkDescriptorSet(uint index) const {
		return vkDescriptorSets[index];
	}
};
}