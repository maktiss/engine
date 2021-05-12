#include "MaterialManager.hpp"


namespace Engine {
std::vector<MaterialManager::MaterialInfo> MaterialManager::materialInfos {};
std::vector<MaterialManager::AllocationInfo> MaterialManager::allocationInfos {};

std::vector<MaterialManager::DescriptorPoolInfo> MaterialManager::descriptorPoolInfos {};

vk::DescriptorSetLayout MaterialManager::vkDescriptorSetLayout {};

vk::Device MaterialManager::vkDevice {};
VmaAllocator MaterialManager::vmaAllocator {};


int MaterialManager::init() {
	spdlog::info("Initializing MaterialManager...");

	assert(vkDevice != vk::Device());
	assert(vmaAllocator != nullptr);


	// Create initial descriptor pool

	createDescriptorPool();


	// Create descriptor set layout

	vk::DescriptorSetLayoutBinding descriptorSetLayoutBinding {};

	// uniform buffer
	descriptorSetLayoutBinding.binding		   = 0;
	descriptorSetLayoutBinding.descriptorType  = vk::DescriptorType::eUniformBuffer;
	descriptorSetLayoutBinding.descriptorCount = 1;
	descriptorSetLayoutBinding.stageFlags	   = vk::ShaderStageFlagBits::eAll;

	vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {};
	descriptorSetLayoutCreateInfo.bindingCount = 1;
	descriptorSetLayoutCreateInfo.pBindings	   = &descriptorSetLayoutBinding;

	auto result = vkDevice.createDescriptorSetLayout(&descriptorSetLayoutCreateInfo, nullptr, &vkDescriptorSetLayout);
	if (result != vk::Result::eSuccess) {
		spdlog::error("[MaterialManager] Failed to create descriptor set layout. Error code: {} ({})", result,
					  vk::to_string(result));
		return 1;
	}

	if (ResourceManagerBase::init()) {
		return 1;
	}

	return 0;
};


void MaterialManager::postCreate(Handle& handle) {
	materialInfos.push_back({});
	allocationInfos.push_back({});

	auto& allocationInfo = allocationInfos[handle.getIndex()];

	auto& uniformBuffer = materialInfos[handle.getIndex()].uniformBuffer;
	auto& descriptorSet = materialInfos[handle.getIndex()].descriptorSet;


	// Create uniform buffer

	vk::BufferCreateInfo bufferCreateInfo {};
	bufferCreateInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;

	apply(handle, [&bufferCreateInfo](auto& material) {
		bufferCreateInfo.size = material.getMaterialUniformBlockSize();
	});

	VkBufferCreateInfo cBufferCreateInfo(bufferCreateInfo);

	VmaAllocationCreateInfo allocationCreateInfo {};
	allocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;


	VkBuffer buffer;
	auto result = vk::Result(vmaCreateBuffer(vmaAllocator, &cBufferCreateInfo, &allocationCreateInfo, &buffer,
											 &allocationInfo.vmaAllocation, nullptr));
	if (result != vk::Result::eSuccess) {
		spdlog::error("[MaterialManager] Failed to allocate uniform buffer memory. Error code: {} ({})", result,
					  vk::to_string(result));
		return;
	}

	uniformBuffer = vk::Buffer(buffer);


	// Get available pool index, and if all filled create a new one

	uint poolIndex = 0;
	for (poolIndex = 0; poolIndex < descriptorPoolInfos.size(); poolIndex++) {
		if (descriptorPoolInfos[poolIndex].descriptorSetCount < 1024) {
			break;
		}
	}
	if (poolIndex == descriptorPoolInfos.size()) {
		createDescriptorPool();
	}

	allocationInfo.descriptorPoolIndex = poolIndex;


	// Allocate descriptor set

	vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo {};
	descriptorSetAllocateInfo.descriptorPool	 = descriptorPoolInfos[poolIndex].vkDescriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	descriptorSetAllocateInfo.pSetLayouts		 = &vkDescriptorSetLayout;

	result = vkDevice.allocateDescriptorSets(&descriptorSetAllocateInfo, &descriptorSet);
	if (result != vk::Result::eSuccess) {
		spdlog::error("[MaterialManager] Failed to allocate descriptor set. Error code: {} ({})", result,
					  vk::to_string(result));

		return;
	}
	descriptorPoolInfos[poolIndex].descriptorSetCount++;


	vk::DescriptorBufferInfo descriptorBufferInfo {};
	descriptorBufferInfo.buffer = uniformBuffer;
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

void MaterialManager::update(Handle& handle) {
	auto& allocationInfo = allocationInfos[handle.getIndex()];


	void* pBufferData;
	auto result = vk::Result(vmaMapMemory(vmaAllocator, allocationInfo.vmaAllocation, &pBufferData));
	if (result != vk::Result::eSuccess) {
		spdlog::error("[MaterialManager] Failed to write uniform buffer memory. Error code: {} ({})", result,
					  vk::to_string(result));

		return;
	}

	apply(handle, [&pBufferData](auto& material) {
		material.writeBuffer(pBufferData);
	});
	vmaUnmapMemory(vmaAllocator, allocationInfo.vmaAllocation);
}


int MaterialManager::createDescriptorPool() {
	vk::DescriptorPoolSize descriptorPoolSize {};
	descriptorPoolSize.descriptorCount = 1;

	vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo {};
	descriptorPoolCreateInfo.poolSizeCount = 1;
	descriptorPoolCreateInfo.pPoolSizes	   = &descriptorPoolSize;
	descriptorPoolCreateInfo.maxSets	   = 1024;

	DescriptorPoolInfo descriptorPoolInfo;
	auto result =
		vkDevice.createDescriptorPool(&descriptorPoolCreateInfo, nullptr, &descriptorPoolInfo.vkDescriptorPool);
	if (result != vk::Result::eSuccess) {
		spdlog::error("[MaterialManager] Failed to create descriptor pool. Error code: {} ({})", result,
					  vk::to_string(result));
		return 1;
	}

	descriptorPoolInfos.push_back(descriptorPoolInfo);

	return 0;
}


void MaterialManager::destroy(uint32_t index) {
	auto& vmaAllocation = allocationInfos[index].vmaAllocation;
	if (vmaAllocation != nullptr) {
		vmaDestroyBuffer(vmaAllocator, materialInfos[index].uniformBuffer, vmaAllocation);
		vmaAllocation = nullptr;

		assert(descriptorPoolInfos[allocationInfos[index].descriptorPoolIndex].descriptorSetCount != 0);
		descriptorPoolInfos[allocationInfos[index].descriptorPoolIndex].descriptorSetCount--;
	}
}

void MaterialManager::dispose() {
	for (uint i = 0; i < materialInfos.size(); i++) {
		destroy(i);
	}
}
} // namespace Engine