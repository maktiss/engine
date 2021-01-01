#include "MaterialManager.hpp"


namespace Engine::Managers {
std::vector<MaterialManager::MaterialInfo> MaterialManager::materialInfos {};
std::vector<VmaAllocation> MaterialManager::allocationInfos {};

vk::DescriptorPool MaterialManager::vkDescriptorPool {};
vk::DescriptorSetLayout MaterialManager::vkDescriptorSetLayout {};

vk::Device MaterialManager::vkDevice {};
VmaAllocator MaterialManager::vmaAllocator {};


int MaterialManager::init() {
	spdlog::info("Initializing MaterialManager...");

	assert(vkDevice != vk::Device());
	assert(vmaAllocator != nullptr);

	vk::DescriptorPoolSize descriptorPoolSize {};
	descriptorPoolSize.descriptorCount = 1;

	vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo {};
	descriptorPoolCreateInfo.poolSizeCount = 1;
	descriptorPoolCreateInfo.pPoolSizes	   = &descriptorPoolSize;
	descriptorPoolCreateInfo.maxSets	   = 1;

	auto result = vkDevice.createDescriptorPool(&descriptorPoolCreateInfo, nullptr, &vkDescriptorPool);
	if (result != vk::Result::eSuccess) {
		spdlog::error(
			"[MaterialManager] Failed to create descriptor pool. Error code: {} ({})", result, vk::to_string(result));
		return 1;
	}

	vk::DescriptorSetLayoutBinding descriptorSetLayoutBinding {};
	descriptorSetLayoutBinding.binding		   = 4; // FIXME
	descriptorSetLayoutBinding.descriptorType  = vk::DescriptorType::eUniformBuffer;
	descriptorSetLayoutBinding.descriptorCount = 1;
	descriptorSetLayoutBinding.stageFlags	   = vk::ShaderStageFlagBits::eAll;

	vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {};
	descriptorSetLayoutCreateInfo.bindingCount = 1;
	descriptorSetLayoutCreateInfo.pBindings	   = &descriptorSetLayoutBinding;

	result = vkDevice.createDescriptorSetLayout(&descriptorSetLayoutCreateInfo, nullptr, &vkDescriptorSetLayout);
	if (result != vk::Result::eSuccess) {
		spdlog::error("[MaterialManager] Failed to create descriptor set layout. Error code: {} ({})",
					  result,
					  vk::to_string(result));
		return 1;
	}

	return ResourceManagerBase::init();
};


void MaterialManager::postCreate(Handle handle) {
	materialInfos.push_back({});
	allocationInfos.push_back({});
}

void MaterialManager::update(Handle handle) {
	destroy(handle.getIndex());

	vk::BufferCreateInfo bufferCreateInfo {};
	bufferCreateInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;

	apply(handle, [&bufferCreateInfo](auto& material) {
		bufferCreateInfo.size = material.getMaterialUniformBlockSize();
	});

	VkBufferCreateInfo cBufferCreateInfo(bufferCreateInfo);

	VmaAllocationCreateInfo allocationCreateInfo {};
	allocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

	auto& allocation = allocationInfos[handle.getIndex()];

	VkBuffer buffer;
	auto result = vk::Result(
		vmaCreateBuffer(vmaAllocator, &cBufferCreateInfo, &allocationCreateInfo, &buffer, &allocation, nullptr));
	if (result != vk::Result::eSuccess) {
		spdlog::error("[MaterialManager] Failed to allocate uniform buffer memory. Error code: {} ({})",
					  result,
					  vk::to_string(result));
		return;
	}

	auto& uniformBuffer = materialInfos[handle.getIndex()].uniformBuffer;
	auto& descriptorSet = materialInfos[handle.getIndex()].descriptorSet;

	uniformBuffer = vk::Buffer(buffer);


	// Allocate descriptor set

	vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo {};
	descriptorSetAllocateInfo.descriptorPool	 = vkDescriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	descriptorSetAllocateInfo.pSetLayouts		 = &vkDescriptorSetLayout;

	result =
		vkDevice.allocateDescriptorSets(&descriptorSetAllocateInfo, &descriptorSet);
	if (result != vk::Result::eSuccess) {
		spdlog::error(
			"[MaterialManager] Failed to allocate descriptor set. Error code: {} ({})", result, vk::to_string(result));

		return;
	}

	vk::DescriptorBufferInfo descriptorBufferInfo {};
	descriptorBufferInfo.buffer = uniformBuffer;
	descriptorBufferInfo.offset = 0;
	descriptorBufferInfo.range = bufferCreateInfo.size;

	vk::WriteDescriptorSet writeDescriptorSet {};
	writeDescriptorSet.dstSet = descriptorSet;
	writeDescriptorSet.dstBinding = 4;
	writeDescriptorSet.dstArrayElement = 0;
	writeDescriptorSet.descriptorType = vk::DescriptorType::eUniformBuffer;
	writeDescriptorSet.descriptorCount = 1;
	writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;

	vkDevice.updateDescriptorSets(1, &writeDescriptorSet, 0, nullptr);


	// TODO: split allocation from updating
	void* pBufferData;
	result = vk::Result(vmaMapMemory(vmaAllocator, allocation, &pBufferData));
	if (result != vk::Result::eSuccess) {
		spdlog::error("[MaterialManager] Failed to write uniform buffer memory. Error code: {} ({})",
					  result,
					  vk::to_string(result));

		return;
	}

	apply(handle, [&pBufferData](auto& material) {
		material.writeBuffer(pBufferData);
	});
	vmaUnmapMemory(vmaAllocator, allocation);
}


void MaterialManager::destroy(uint32_t index) {
	if (allocationInfos[index] != nullptr) {
		vmaDestroyBuffer(vmaAllocator, materialInfos[index].uniformBuffer, allocationInfos[index]);
		allocationInfos[index] = nullptr;
	}
}

void MaterialManager::dispose() {
	for (uint i = 0; i < materialInfos.size(); i++) {
		destroy(i);
	}
}
} // namespace Engine::Managers