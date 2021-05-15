#pragma once

#define VULKAN_HPP_NO_EXCEPTIONS 1
#include <vulkan/vulkan.hpp>

#include "vk_mem_alloc.h"

#include <vector>


namespace Engine {
class DescriptorSetArray {
private:
	vk::Device vkDevice {};
	VmaAllocator vmaAllocator {};

	struct BufferInfo {
		vk::Buffer buffer {};
		VmaAllocation allocation {};
	};

	// Buffer for each element for each binding
	std::vector<BufferInfo> bufferInfos {};

	struct BindingLayoutInfo {
		vk::DescriptorType descriptorType {};
		vk::DeviceSize size {};
		uint32_t descriptorCount = 1;
	};
	std::vector<BindingLayoutInfo> bindingLayoutInfos {};

	vk::DescriptorPool vkDescriptorPool {};
	vk::DescriptorSetLayout vkDescriptorSetLayout {};
	std::vector<vk::DescriptorSet> vkDescriptorSets { 1, vk::DescriptorSet() };


public:
	int init();

	int updateBuffer(uint elementIndex, uint bindingIndex, void* pData, uint64_t size);
	int updateImage(uint elementIndex, uint bindingIndex, uint descriptorIndex, vk::Sampler sampler,
					vk::ImageView imageView);

	int updateBuffers(uint bindingIndex, void* pData, uint64_t size);
	int updateImages(uint bindingIndex, uint descriptorIndex, vk::Sampler sampler, vk::ImageView imageView);

	int mapBuffer(uint elementIndex, uint bindingIndex, void*& pData);
	int unmapBuffer(uint elementIndex, uint bindingIndex);

	void dispose();


	inline void setVkDevice(vk::Device device) {
		vkDevice = device;
	}

	inline void setVmaAllocator(VmaAllocator allocator) {
		vmaAllocator = allocator;
	}


	inline void setElementCount(uint count) {
		vkDescriptorSets.resize(count);
	}

	inline uint getElementCount() const {
		return vkDescriptorSets.size();
	}


	inline void setBindingLayoutInfo(uint bindingIndex, vk::DescriptorType descriptorType, vk::DeviceSize size,
									 uint32_t descriptorCount = 1) {

		if (bindingIndex >= bindingLayoutInfos.size()) {
			bindingLayoutInfos.resize(bindingIndex + 1);
		}

		bindingLayoutInfos[bindingIndex] = { descriptorType, size, descriptorCount };
	}

	inline uint getBindingCount() const {
		return bindingLayoutInfos.size();
	}


	inline uint getBufferInfoIndex(uint elementIndex, uint bindingIndex) const {
		return elementIndex * getBindingCount() + bindingIndex;
	}


	inline const auto& getVkDescriptorSetLayout() const {
		return vkDescriptorSetLayout;
	}

	inline const auto& getVkDescriptorSet(uint elementIndex) const {
		return vkDescriptorSets[elementIndex];
	}
};
} // namespace Engine