#include "MaterialManager.hpp"


namespace Engine::Managers {
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

	std::array<vk::DescriptorSetLayoutBinding, 2> descriptorSetLayoutBindings {};

	// uniform buffer
	descriptorSetLayoutBindings[0].binding		   = 0;
	descriptorSetLayoutBindings[0].descriptorType  = vk::DescriptorType::eUniformBuffer;
	descriptorSetLayoutBindings[0].descriptorCount = 1;
	descriptorSetLayoutBindings[0].stageFlags	   = vk::ShaderStageFlagBits::eAll;

	// texture samplers array
	descriptorSetLayoutBindings[1].binding		   = 1;
	descriptorSetLayoutBindings[1].descriptorType  = vk::DescriptorType::eCombinedImageSampler;
	descriptorSetLayoutBindings[1].descriptorCount = 8;
	descriptorSetLayoutBindings[1].stageFlags	   = vk::ShaderStageFlagBits::eAll;

	vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {};
	descriptorSetLayoutCreateInfo.bindingCount = descriptorSetLayoutBindings.size();
	descriptorSetLayoutCreateInfo.pBindings	   = descriptorSetLayoutBindings.data();

	auto result = vkDevice.createDescriptorSetLayout(&descriptorSetLayoutCreateInfo, nullptr, &vkDescriptorSetLayout);
	if (result != vk::Result::eSuccess) {
		spdlog::error("[MaterialManager] Failed to create descriptor set layout. Error code: {} ({})",
					  result,
					  vk::to_string(result));
		return 1;
	}

	if (ResourceManagerBase::init()) {
		return 1;
	}

	return 0;
};


void MaterialManager::postCreate(Handle handle) {
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
	auto result = vk::Result(vmaCreateBuffer(
		vmaAllocator, &cBufferCreateInfo, &allocationCreateInfo, &buffer, &allocationInfo.vmaAllocation, nullptr));
	if (result != vk::Result::eSuccess) {
		spdlog::error("[MaterialManager] Failed to allocate uniform buffer memory. Error code: {} ({})",
					  result,
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
		spdlog::error(
			"[MaterialManager] Failed to allocate descriptor set. Error code: {} ({})", result, vk::to_string(result));

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

void MaterialManager::update(Handle handle) {
	auto& allocationInfo = allocationInfos[handle.getIndex()];


	// Update UBO

	void* pBufferData;
	auto result = vk::Result(vmaMapMemory(vmaAllocator, allocationInfo.vmaAllocation, &pBufferData));
	if (result != vk::Result::eSuccess) {
		spdlog::error("[MaterialManager] Failed to write uniform buffer memory. Error code: {} ({})",
					  result,
					  vk::to_string(result));

		return;
	}

	apply(handle, [&pBufferData](auto& material) {
		material.writeBuffer(pBufferData);
	});
	vmaUnmapMemory(vmaAllocator, allocationInfo.vmaAllocation);


	// Update samplers

	std::vector<vk::DescriptorImageInfo> descriptorImageInfos(8);
	apply(handle, [&descriptorImageInfos](auto& material) {
		for (uint i = 0; i < material.textureHandles.size(); i++) {
			auto textureInfo = Engine::Managers::TextureManager::getTextureInfo(material.textureHandles[i]);
			descriptorImageInfos[i].sampler		= textureInfo.sampler;
			descriptorImageInfos[i].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			descriptorImageInfos[i].imageView	= textureInfo.imageView;
		}

		auto fallbackTextureHandle = Engine::Managers::TextureManager::getHandle(0);
		auto fallbackTextureInfo   = Engine::Managers::TextureManager::getTextureInfo(fallbackTextureHandle);
		for (uint i = material.textureHandles.size(); i < 8; i++) {
			descriptorImageInfos[i].sampler		= fallbackTextureInfo.sampler;
			descriptorImageInfos[i].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			descriptorImageInfos[i].imageView	= fallbackTextureInfo.imageView;
		}
	});

	vk::WriteDescriptorSet writeDescriptorSet {};
	writeDescriptorSet.dstSet		   = materialInfos[handle.getIndex()].descriptorSet;
	writeDescriptorSet.dstBinding	   = 1;
	writeDescriptorSet.dstArrayElement = 0;
	writeDescriptorSet.descriptorType  = vk::DescriptorType::eCombinedImageSampler;
	writeDescriptorSet.descriptorCount = descriptorImageInfos.size();
	writeDescriptorSet.pImageInfo	   = descriptorImageInfos.data();

	vkDevice.updateDescriptorSets(1, &writeDescriptorSet, 0, nullptr);
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
		spdlog::error(
			"[MaterialManager] Failed to create descriptor pool. Error code: {} ({})", result, vk::to_string(result));
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
} // namespace Engine::Managers