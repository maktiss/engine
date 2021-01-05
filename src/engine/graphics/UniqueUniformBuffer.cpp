#include "UniqueUniformBuffer.hpp"

#include <spdlog/spdlog.h>


namespace Engine::Graphics {
int UniqueUniformBuffer::init(vk::Device device, VmaAllocator allocator, vk::DeviceSize size) {
	vkDevice	 = device;
	vmaAllocator = allocator;
	vkBufferSize = size;


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
	descriptorPoolCreateInfo.maxSets	   = 1;

	result = vkDevice.createDescriptorPool(&descriptorPoolCreateInfo, nullptr, &vkDescriptorPool);
	if (result != vk::Result::eSuccess) {
		spdlog::error("Failed to create descriptor pool. Error code: {} ({})", result, vk::to_string(result));
		return 1;
	}


	// Create uniform buffer

	vk::BufferCreateInfo bufferCreateInfo {};
	bufferCreateInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
	bufferCreateInfo.size  = vkBufferSize;

	VkBufferCreateInfo cBufferCreateInfo(bufferCreateInfo);

	VmaAllocationCreateInfo allocationCreateInfo {};
	allocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;


	VkBuffer buffer;
	result = vk::Result(
		vmaCreateBuffer(vmaAllocator, &cBufferCreateInfo, &allocationCreateInfo, &buffer, &vmaAllocation, nullptr));
	if (result != vk::Result::eSuccess) {
		spdlog::error("Failed to allocate uniform buffer memory. Error code: {} ({})", result, vk::to_string(result));
		return 1;
	}

	vkBuffer = vk::Buffer(buffer);


	// Allocate descriptor set

	vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo {};
	descriptorSetAllocateInfo.descriptorPool	 = vkDescriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	descriptorSetAllocateInfo.pSetLayouts		 = &vkDescriptorSetLayout;

	result = vkDevice.allocateDescriptorSets(&descriptorSetAllocateInfo, &vkDescriptorSet);
	if (result != vk::Result::eSuccess) {
		spdlog::error("Failed to allocate descriptor set. Error code: {} ({})", result, vk::to_string(result));

		return 1;
	}


	vk::DescriptorBufferInfo descriptorBufferInfo {};
	descriptorBufferInfo.buffer = vkBuffer;
	descriptorBufferInfo.offset = 0;
	descriptorBufferInfo.range	= bufferCreateInfo.size;

	vk::WriteDescriptorSet writeDescriptorSet {};
	writeDescriptorSet.dstSet		   = vkDescriptorSet;
	writeDescriptorSet.dstBinding	   = 0;
	writeDescriptorSet.dstArrayElement = 0;
	writeDescriptorSet.descriptorType  = vk::DescriptorType::eUniformBuffer;
	writeDescriptorSet.descriptorCount = 1;
	writeDescriptorSet.pBufferInfo	   = &descriptorBufferInfo;

	vkDevice.updateDescriptorSets(1, &writeDescriptorSet, 0, nullptr);


	return 0;
}

int UniqueUniformBuffer::update(void* pData, uint64_t size) {
	assert(size <= vkBufferSize);

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
	if (vmaAllocation != nullptr) {
		vmaDestroyBuffer(vmaAllocator, vkBuffer, vmaAllocation);
		vmaAllocation = nullptr;
	}
}
} // namespace Engine::Graphics