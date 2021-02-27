#include "UniqueUniformBuffer.hpp"

#include <spdlog/spdlog.h>


namespace Engine::Graphics {
int UniqueUniformBuffer::init(vk::Device device, VmaAllocator allocator, vk::DeviceSize size, uint count) {
	vkDevice	 = device;
	vmaAllocator = allocator;
	vkBufferSize = size;
	setCount	 = count;

	vkBuffers.resize(setCount);
	vmaAllocations.resize(setCount);

	vkDescriptorSets.resize(setCount);


	// Create descriptor set layout

	vk::DescriptorSetLayoutBinding descriptorSetLayoutBinding {};
	descriptorSetLayoutBinding.binding		   = 0;
	descriptorSetLayoutBinding.descriptorType  = vk::DescriptorType::eUniformBuffer;
	descriptorSetLayoutBinding.descriptorCount = 1;
	descriptorSetLayoutBinding.stageFlags	   = vk::ShaderStageFlagBits::eAll;

	vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {};
	descriptorSetLayoutCreateInfo.bindingCount = 1;
	descriptorSetLayoutCreateInfo.pBindings	   = &descriptorSetLayoutBinding;

	auto result = vkDevice.createDescriptorSetLayout(&descriptorSetLayoutCreateInfo, nullptr, &vkDescriptorSetLayout);
	if (result != vk::Result::eSuccess) {
		spdlog::error("Failed to create descriptor set layout. Error code: {} ({})", result, vk::to_string(result));
		return 1;
	}


	// Create descriptor pool

	vk::DescriptorPoolSize descriptorPoolSize {};
	descriptorPoolSize.descriptorCount = 1;

	vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo {};
	descriptorPoolCreateInfo.poolSizeCount = 1;
	descriptorPoolCreateInfo.pPoolSizes	   = &descriptorPoolSize;
	descriptorPoolCreateInfo.maxSets	   = setCount;

	result = vkDevice.createDescriptorPool(&descriptorPoolCreateInfo, nullptr, &vkDescriptorPool);
	if (result != vk::Result::eSuccess) {
		spdlog::error("Failed to create descriptor pool. Error code: {} ({})", result, vk::to_string(result));
		return 1;
	}


	for (uint index = 0; index < setCount; index++) {
		// Create uniform buffers

		vk::BufferCreateInfo bufferCreateInfo {};
		bufferCreateInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
		bufferCreateInfo.size  = vkBufferSize;

		VkBufferCreateInfo cBufferCreateInfo(bufferCreateInfo);

		VmaAllocationCreateInfo allocationCreateInfo {};
		allocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

		auto& buffer	 = vkBuffers[index];
		auto& allocation = vmaAllocations[index];

		VkBuffer cBuffer;
		result = vk::Result(
			vmaCreateBuffer(vmaAllocator, &cBufferCreateInfo, &allocationCreateInfo, &cBuffer, &allocation, nullptr));
		if (result != vk::Result::eSuccess) {
			spdlog::error("Failed to allocate uniform buffer memory. Error code: {} ({})", result,
						  vk::to_string(result));
			return 1;
		}

		buffer = vk::Buffer(cBuffer);


		// Allocate descriptor set

		vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo {};
		descriptorSetAllocateInfo.descriptorPool	 = vkDescriptorPool;
		descriptorSetAllocateInfo.descriptorSetCount = 1;
		descriptorSetAllocateInfo.pSetLayouts		 = &vkDescriptorSetLayout;

		auto& descriptorSet = vkDescriptorSets[index];

		result = vkDevice.allocateDescriptorSets(&descriptorSetAllocateInfo, &descriptorSet);
		if (result != vk::Result::eSuccess) {
			spdlog::error("Failed to allocate descriptor set. Error code: {} ({})", result, vk::to_string(result));

			return 1;
		}


		vk::DescriptorBufferInfo descriptorBufferInfo {};
		descriptorBufferInfo.buffer = buffer;
		descriptorBufferInfo.offset = 0;
		descriptorBufferInfo.range	= bufferCreateInfo.size;

		vk::WriteDescriptorSet writeDescriptorSet {};
		writeDescriptorSet.dstSet		   = descriptorSet;
		writeDescriptorSet.dstBinding	   = 0;
		writeDescriptorSet.dstArrayElement = 0;
		writeDescriptorSet.descriptorType  = vk::DescriptorType::eUniformBuffer;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.pBufferInfo	   = &descriptorBufferInfo;

		vkDevice.updateDescriptorSets(1, &writeDescriptorSet, 0, nullptr);
	}


	return 0;
}

int UniqueUniformBuffer::update(uint index, void* pData, uint64_t size) {
	assert(size <= vkBufferSize);

	const auto& vmaAllocation = vmaAllocations[index];

	void* pBufferData;
	auto result = vk::Result(vmaMapMemory(vmaAllocator, vmaAllocation, &pBufferData));
	if (result != vk::Result::eSuccess) {
		spdlog::error("Failed to write uniform buffer memory. Error code: {} ({})", result, vk::to_string(result));

		return 1;
	}

	memcpy(pBufferData, pData, size);
	vmaUnmapMemory(vmaAllocator, vmaAllocation);
	return 0;
}


void UniqueUniformBuffer::dispose() {
	for (uint index = 0; index < setCount; index++) {
		auto& vmaAllocation = vmaAllocations[index];
		auto& vkBuffer	  = vkBuffers[index];

		if (vmaAllocation != nullptr) {
			vmaDestroyBuffer(vmaAllocator, vkBuffer, vmaAllocation);
			vmaAllocation = nullptr;
		}
	}
}
} // namespace Engine::Graphics