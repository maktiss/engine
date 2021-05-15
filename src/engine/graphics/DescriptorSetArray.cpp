#include "DescriptorSetArray.hpp"

#include <spdlog/spdlog.h>


namespace Engine {
int DescriptorSetArray::init() {
	dispose();

	assert(vkDevice != vk::Device());
	assert(vmaAllocator != nullptr);

	bufferInfos.resize(getElementCount() * getBindingCount());


	// Create descriptor set layout

	std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings(bindingLayoutInfos.size());

	for (uint index = 0; index < descriptorSetLayoutBindings.size(); index++) {
		descriptorSetLayoutBindings[index].binding		   = index;
		descriptorSetLayoutBindings[index].descriptorType  = bindingLayoutInfos[index].descriptorType;
		descriptorSetLayoutBindings[index].descriptorCount = bindingLayoutInfos[index].descriptorCount;
		descriptorSetLayoutBindings[index].stageFlags	   = vk::ShaderStageFlagBits::eAll;
	}

	vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {};
	descriptorSetLayoutCreateInfo.bindingCount = descriptorSetLayoutBindings.size();
	descriptorSetLayoutCreateInfo.pBindings	   = descriptorSetLayoutBindings.data();

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
	descriptorPoolCreateInfo.maxSets	   = getElementCount();

	result = vkDevice.createDescriptorPool(&descriptorPoolCreateInfo, nullptr, &vkDescriptorPool);
	if (result != vk::Result::eSuccess) {
		spdlog::error("Failed to create descriptor pool. Error code: {} ({})", result, vk::to_string(result));
		return 1;
	}


	for (uint elementIndex = 0; elementIndex < getElementCount(); elementIndex++) {
		// Allocate descriptor set

		vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo {};
		descriptorSetAllocateInfo.descriptorPool	 = vkDescriptorPool;
		descriptorSetAllocateInfo.descriptorSetCount = 1;
		descriptorSetAllocateInfo.pSetLayouts		 = &vkDescriptorSetLayout;

		auto& descriptorSet = vkDescriptorSets[elementIndex];

		result = vkDevice.allocateDescriptorSets(&descriptorSetAllocateInfo, &descriptorSet);
		if (result != vk::Result::eSuccess) {
			spdlog::error("Failed to allocate descriptor set. Error code: {} ({})", result, vk::to_string(result));

			return 1;
		}


		// Create and write buffers for each binding if needed
		for (uint bindingIndex = 0; bindingIndex < getBindingCount(); bindingIndex++) {
			const auto& bindingLayoutInfo = bindingLayoutInfos[bindingIndex];

			// TODO: dynamic buffer variations
			if (bindingLayoutInfo.descriptorType == vk::DescriptorType::eUniformBuffer ||
				bindingLayoutInfo.descriptorType == vk::DescriptorType::eStorageBuffer) {

				vk::BufferCreateInfo bufferCreateInfo {};
				bufferCreateInfo.size = bindingLayoutInfo.size;

				switch (bindingLayoutInfo.descriptorType) {
				case vk::DescriptorType::eUniformBuffer:
					bufferCreateInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
					break;
				case vk::DescriptorType::eStorageBuffer:
					bufferCreateInfo.usage = vk::BufferUsageFlagBits::eStorageBuffer;
					break;
				}

				VkBufferCreateInfo cBufferCreateInfo(bufferCreateInfo);

				VmaAllocationCreateInfo allocationCreateInfo {};
				allocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

				auto& bufferInfo = bufferInfos[getBufferInfoIndex(elementIndex, bindingIndex)];

				VkBuffer cBuffer;
				result = vk::Result(vmaCreateBuffer(vmaAllocator, &cBufferCreateInfo, &allocationCreateInfo, &cBuffer,
													&bufferInfo.allocation, nullptr));
				if (result != vk::Result::eSuccess) {
					spdlog::error("Failed to allocate buffer memory. Error code: {} ({})", result,
								  vk::to_string(result));
					return 1;
				}

				bufferInfo.buffer = vk::Buffer(cBuffer);


				// Write descriptor set

				vk::DescriptorBufferInfo descriptorBufferInfo {};
				descriptorBufferInfo.buffer = bufferInfo.buffer;
				descriptorBufferInfo.offset = 0;
				descriptorBufferInfo.range	= bufferCreateInfo.size;

				vk::WriteDescriptorSet writeDescriptorSet {};
				writeDescriptorSet.dstSet		   = descriptorSet;
				writeDescriptorSet.dstBinding	   = bindingIndex;
				writeDescriptorSet.dstArrayElement = 0;
				writeDescriptorSet.descriptorType  = bindingLayoutInfo.descriptorType;
				writeDescriptorSet.descriptorCount = 1;
				writeDescriptorSet.pBufferInfo	   = &descriptorBufferInfo;

				vkDevice.updateDescriptorSets(1, &writeDescriptorSet, 0, nullptr);
			}
		}
	}


	return 0;
}


int DescriptorSetArray::updateBuffer(uint elementIndex, uint bindingIndex, void* pData, uint64_t size) {
	assert(size <= bindingLayoutInfos[bindingIndex].size);

	const auto& allocation = bufferInfos[getBufferInfoIndex(elementIndex, bindingIndex)].allocation;

	void* pBufferData;
	auto result = vk::Result(vmaMapMemory(vmaAllocator, allocation, &pBufferData));
	if (result != vk::Result::eSuccess) {
		spdlog::error("Failed to write uniform buffer memory. Error code: {} ({})", result, vk::to_string(result));
		return 1;
	}

	memcpy(pBufferData, pData, size);
	vmaUnmapMemory(vmaAllocator, allocation);

	return 0;
}

int DescriptorSetArray::updateImage(uint elementIndex, uint bindingIndex, uint descriptorIndex, vk::Sampler sampler,
									vk::ImageView imageView) {

	assert(vkDevice != vk::Device());

	vk::DescriptorImageInfo descriptorImageInfo {};
	descriptorImageInfo.sampler		= sampler;
	descriptorImageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	descriptorImageInfo.imageView	= imageView;

	vk::WriteDescriptorSet writeDescriptorSet {};
	writeDescriptorSet.dstSet		   = vkDescriptorSets[elementIndex];
	writeDescriptorSet.dstBinding	   = bindingIndex;
	writeDescriptorSet.dstArrayElement = descriptorIndex;
	writeDescriptorSet.descriptorType  = bindingLayoutInfos[bindingIndex].descriptorType;
	writeDescriptorSet.descriptorCount = 1;
	writeDescriptorSet.pImageInfo	   = &descriptorImageInfo;

	vkDevice.updateDescriptorSets(1, &writeDescriptorSet, 0, nullptr);
	return 0;
}


int DescriptorSetArray::updateBuffers(uint bindingIndex, void* pData, uint64_t size) {
	for (uint elementIndex = 0; elementIndex < getElementCount(); elementIndex++) {
		if (updateBuffer(elementIndex, bindingIndex, pData, size)) {
			return 1;
		}
	}

	return 0;
}

int DescriptorSetArray::updateImages(uint bindingIndex, uint descriptorIndex, vk::Sampler sampler,
									 vk::ImageView imageView) {

	for (uint elementIndex = 0; elementIndex < getElementCount(); elementIndex++) {
		if (updateImage(elementIndex, bindingIndex, descriptorIndex, sampler, imageView)) {
			return 1;
		}
	}

	return 0;
}


int DescriptorSetArray::mapBuffer(uint elementIndex, uint bindingIndex, void*& pData) {
	const auto& allocation = bufferInfos[getBufferInfoIndex(elementIndex, bindingIndex)].allocation;

	auto result = vk::Result(vmaMapMemory(vmaAllocator, allocation, &pData));
	if (result != vk::Result::eSuccess) {
		spdlog::error("Failed to write uniform buffer memory. Error code: {} ({})", result, vk::to_string(result));
		return 1;
	}
	return 0;
}

int DescriptorSetArray::unmapBuffer(uint elementIndex, uint bindingIndex) {
	const auto& allocation = bufferInfos[getBufferInfoIndex(elementIndex, bindingIndex)].allocation;
	vmaUnmapMemory(vmaAllocator, allocation);
	return 0;
}


void DescriptorSetArray::dispose() {
	for (auto& bufferInfo : bufferInfos) {
		if (bufferInfo.allocation != nullptr) {
			vmaDestroyBuffer(vmaAllocator, bufferInfo.buffer, bufferInfo.allocation);
			bufferInfo.allocation = nullptr;
		}
	}

	if (vkDevice != vk::Device()) {
		if (vkDescriptorPool != vk::DescriptorPool()) {
			vkDevice.destroyDescriptorPool(vkDescriptorPool);
		}
		if (vkDescriptorSetLayout != vk::DescriptorSetLayout()) {
			vkDevice.destroyDescriptorSetLayout(vkDescriptorSetLayout);
		}
	}
}
} // namespace Engine