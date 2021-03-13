#pragma once

#define VULKAN_HPP_NO_EXCEPTIONS 1
#include <vulkan/vulkan.hpp>

#include "vk_mem_alloc.h"

#include <vector>


namespace Engine::Graphics {
class DescriptorSetArray {
private:
	vk::Device vkDevice {};
	VmaAllocator vmaAllocator {};

	struct BufferInfo {
		vk::Buffer buffer {};
		VmaAllocation allocation {};
	};

	// Buffer for each set for each binding
	std::vector<BufferInfo> bufferInfos { 1, BufferInfo() };

	struct BindingLayoutInfo {
		vk::DescriptorType descriptorType {};
		vk::DeviceSize size {};
	};
	std::vector<BindingLayoutInfo> bindingLayoutInfos { 1, BindingLayoutInfo() };

	vk::DescriptorPool vkDescriptorPool {};
	vk::DescriptorSetLayout vkDescriptorSetLayout {};
	std::vector<vk::DescriptorSet> vkDescriptorSets { 1, vk::DescriptorSet() };

public:
	int init(vk::Device device, VmaAllocator allocator);

	int updateBuffer(uint setIndex, uint bindingIndex, void* pData, uint64_t size);
	int updateImage(uint setIndex, uint bindingIndex, vk::Sampler sampler, vk::ImageView imageView);

	int mapBuffer(uint setIndex, uint bindingIndex, void*& pData);
	int unmapBuffer(uint setIndex, uint bindingIndex);

	void dispose();


	inline void setSetCount(uint count) {
		vkDescriptorSets.resize(count);
	}

	inline void setBindingCount(uint count) {
		bindingLayoutInfos.resize(count);
	}

	inline void setBindingLayoutInfo(uint bindingIndex, vk::DescriptorType descriptorType, vk::DeviceSize size) {
		bindingLayoutInfos[bindingIndex] = { descriptorType, size };
	}


	inline uint getSetCount() const {
		return vkDescriptorSets.size();
	}

	inline uint getBindingCount() const {
		return bindingLayoutInfos.size();
	}


	inline uint getBufferInfoIndex(uint setIndex, uint bindingIndex) const {
		return setIndex * getBindingCount() + bindingIndex;
	}


	inline const auto& getVkDescriptorSetLayout() const {
		return vkDescriptorSetLayout;
	}

	inline const auto& getVkDescriptorSet(uint index) const {
		return vkDescriptorSets[index];
	}
};
} // namespace Engine::Graphics