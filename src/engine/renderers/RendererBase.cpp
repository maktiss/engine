#include "RendererBase.hpp"


namespace Engine::Renderers {
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

	auto result = vkDevice.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &vkPipelineLayout);
	if (result != vk::Result::eSuccess) {
		spdlog::error("Failed to create pipeline layout. Error code: {} ({})", result, vk::to_string(result));
		return 1;
	}

	return 0;
}
} // namespace Engine::Renderers