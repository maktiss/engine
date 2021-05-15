#include "RendererBase.hpp"

#include <unordered_map>


namespace Engine {
int RendererBase::init() {
	if (createInputVkSamplers()) {
		return 1;
	}

	if (createInputVkImageViews()) {
		return 1;
	}

	if (defineDescriptorSets()) {
		return 1;
	}

	if (createVkPipelineLayout()) {
		return 1;
	}

	return 0;
}


void RendererBase::bindDescriptorSets(const vk::CommandBuffer& commandBuffer, vk::PipelineBindPoint bindPoint) {
	for (const auto& [setIndex, descriptorSetArray] : descriptorSetArrays) {
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, vkPipelineLayout, setIndex, 1,
										 &descriptorSetArray.getVkDescriptorSet(0), 0, nullptr);
	}
}


int RendererBase::defineDescriptorSets() {
	auto descriptorSetDescriptions = getDescriptorSetDescriptions();

	descriptorSetArrays.clear();

	for (const auto& descriptorSetDescription : descriptorSetDescriptions) {
		auto& descriptorSetArray = descriptorSetArrays[descriptorSetDescription.setIndex];

		descriptorSetArray.setBindingLayoutInfo(descriptorSetDescription.bindingIndex, descriptorSetDescription.type,
												descriptorSetDescription.size,
												descriptorSetDescription.descriptorCount);
	}

	for (auto& [setIndex, descriptorSetArray] : descriptorSetArrays) {
		descriptorSetArray.setElementCount(framesInFlightCount * getLayerCount());

		descriptorSetArray.setVkDevice(vkDevice);
		descriptorSetArray.setVmaAllocator(vmaAllocator);

		if (descriptorSetArray.init()) {
			return 1;
		}
	}

	return 0;
}


int RendererBase::createVkPipelineLayout() {
	auto descriptorSetLayouts = getVkDescriptorSetLayouts();

	// FIXME should be set by derived renderers
	vk::PushConstantRange pushConstantRange {};
	pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eAll;
	pushConstantRange.offset	 = 0;
	pushConstantRange.size		 = 64;

	vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
	pipelineLayoutCreateInfo.setLayoutCount = descriptorSetLayouts.size();
	pipelineLayoutCreateInfo.pSetLayouts	= descriptorSetLayouts.data();

	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges	= &pushConstantRange;

	if (testVkResult(vkDevice.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &vkPipelineLayout),
					 "Failed to create pipeline layout")) {
		return 1;
	}

	return 0;
}


int RendererBase::createInputVkSamplers() {
	const auto samplerCreateInfos = getInputVkSamplerCreateInfos();

	inputVkSamplers.clear();

	for (const auto& samplerCreateInfo : samplerCreateInfos) {
		vk::Sampler sampler {};

		if (testVkResult(vkDevice.createSampler(&samplerCreateInfo, nullptr, &sampler),
						 "Failed to create input image sampler")) {
			return 1;
		}

		inputVkSamplers.push_back(sampler);
	}

	return 0;
}

int RendererBase::createInputVkImageViews() {
	const auto imageViewCreateInfos = getInputVkImageViewCreateInfos();

	inputVkImageViews.clear();

	for (const auto& imageViewCreateInfo : imageViewCreateInfos) {
		vk::ImageView imageView {};

		if (testVkResult(vkDevice.createImageView(&imageViewCreateInfo, nullptr, &imageView),
						 "Failed to create input image view")) {
			return 1;
		}

		inputVkImageViews.push_back(imageView);
	}

	return 0;
}


void RendererBase::dispose() {
	for (auto& [setIndex, descriptorSetArray] : descriptorSetArrays) {
		descriptorSetArray.dispose();
	}
	descriptorSetArrays.clear();

	for (auto& sampler : inputVkSamplers) {
		vkDevice.destroySampler(sampler);
	}
	inputVkSamplers.clear();

	for (auto& imageView : inputVkImageViews) {
		vkDevice.destroyImageView(imageView);
	}
	inputVkImageViews.clear();
}
} // namespace Engine