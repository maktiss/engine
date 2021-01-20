#include "GraphicsRendererBase.hpp"

#include "engine/managers/MeshManager.hpp"
#include "engine/managers/ShaderManager.hpp"

#include <spdlog/spdlog.h>


namespace Engine::Renderers::Graphics {
int GraphicsRendererBase::init() {
	if (createRenderPass()) {
		return 1;
	}

	if (createFramebuffer()) {
		return 1;
	}

	if (createGraphicsPipelines()) {
		return 1;
	}

	return 0;
}


int GraphicsRendererBase::render(vk::CommandBuffer commandBuffer, double dt) {
	vk::CommandBufferBeginInfo commandBufferBeginInfo {};

	vk::RenderPassBeginInfo renderPassBeginInfo {};
	renderPassBeginInfo.renderPass		  = vkRenderPass;
	renderPassBeginInfo.framebuffer		  = vkFramebuffer;
	renderPassBeginInfo.renderArea.extent = outputSize;
	renderPassBeginInfo.renderArea.offset = vk::Offset2D(0, 0);

	auto clearValues					= getVkClearValues();
	renderPassBeginInfo.clearValueCount = clearValues.size();
	renderPassBeginInfo.pClearValues	= clearValues.data();


	// Begin command buffer

	auto result = commandBuffer.begin(&commandBufferBeginInfo);
	if (result != vk::Result::eSuccess) {
		spdlog::error("Failed to record command buffer. Error code: {} ({})", result, vk::to_string(result));
		return 1;
	}

	commandBuffer.beginRenderPass(&renderPassBeginInfo, vk::SubpassContents::eInline);

	// Command buffer is recorded by derived renderers
	recordCommandBuffer(dt, commandBuffer);

	commandBuffer.endRenderPass();
	result = commandBuffer.end();
	if (result != vk::Result::eSuccess) {
		spdlog::error("Failed to record command buffer. Error code: {} ({})", result, vk::to_string(result));
		return 1;
	}

	return 0;
}


int GraphicsRendererBase::createRenderPass() {
	auto attachmentReferences = getVkAttachmentReferences();

	assert(attachmentReferences.size() == (inputs.size() + outputs.size()));

	vk::SubpassDescription subpassDescription {};
	subpassDescription.pipelineBindPoint	= vk::PipelineBindPoint::eGraphics;
	subpassDescription.colorAttachmentCount = attachmentReferences.size();
	subpassDescription.pColorAttachments	= attachmentReferences.data();

	// Set depth attachment
	if (attachmentReferences.size() > 0) {
		auto& lastAttachmentReference = attachmentReferences[attachmentReferences.size() - 1];
		// TODO check for all depth layout variations
		if (lastAttachmentReference.layout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
			subpassDescription.colorAttachmentCount--;
			subpassDescription.pDepthStencilAttachment = &lastAttachmentReference;
		}
	}


	auto attachmentDescriptions = getVkAttachmentDescriptions();
	for (uint i = 0; i < attachmentDescriptions.size(); i++) {
		attachmentDescriptions[i].initialLayout = vkOutputInitialLayouts[i];
		attachmentDescriptions[i].finalLayout	= vkOutputFinalLayouts[i];
	}

	vk::RenderPassCreateInfo renderPassCreateInfo {};
	renderPassCreateInfo.attachmentCount = attachmentDescriptions.size();
	renderPassCreateInfo.pAttachments	 = attachmentDescriptions.data();
	renderPassCreateInfo.subpassCount	 = 1;
	renderPassCreateInfo.pSubpasses		 = &subpassDescription;

	auto result = vkDevice.createRenderPass(&renderPassCreateInfo, nullptr, &vkRenderPass);
	if (result != vk::Result::eSuccess) {
		spdlog::error("Failed to create render pass. Error code: {} ({})", result, vk::to_string(result));
		return 1;
	}

	return 0;
}

int GraphicsRendererBase::createFramebuffer() {
	assert(vkRenderPass != vk::RenderPass());

	std::vector<vk::ImageView> imageViews(outputs.size());

	for (uint i = 0; i < imageViews.size(); i++) {
		imageViews[i] = Engine::Managers::TextureManager::getTextureInfo(outputs[i]).imageView;
		assert(imageViews[i] != vk::ImageView());
	}

	vk::FramebufferCreateInfo framebufferCreateInfo {};
	framebufferCreateInfo.renderPass	  = vkRenderPass;
	framebufferCreateInfo.attachmentCount = imageViews.size();
	framebufferCreateInfo.pAttachments	  = imageViews.data();

	framebufferCreateInfo.width	 = outputSize.width;
	framebufferCreateInfo.height = outputSize.height;
	framebufferCreateInfo.layers = 1;

	auto result = vkDevice.createFramebuffer(&framebufferCreateInfo, nullptr, &vkFramebuffer);
	if (result != vk::Result::eSuccess) {
		spdlog::error("Failed to create framebuffer. Error code: {} ({})", result, vk::to_string(result));
		return 1;
	}

	return 0;
}

int GraphicsRendererBase::createGraphicsPipelines() {
	constexpr auto shaderTypeCount = Engine::Managers::ShaderManager::getTypeCount();
	constexpr auto meshTypeCount   = Engine::Managers::MeshManager::getTypeCount();

	auto renderPassIndex = Engine::Managers::ShaderManager::getRenderPassIndex(getRenderPassName());

	const auto& pipelineInputAssemblyStateCreateInfo = getVkPipelineInputAssemblyStateCreateInfo();
	const auto& viewport							 = getVkViewport();
	const auto& scissor								 = getVkScissor();
	const auto& pipelineRasterizationStateCreateInfo = getVkPipelineRasterizationStateCreateInfo();
	const auto& pipelineMultisampleStateCreateInfo	 = getVkPipelineMultisampleStateCreateInfo();
	const auto& pipelineDepthStencilStateCreateInfo	 = getVkPipelineDepthStencilStateCreateInfo();
	const auto& pipelineColorBlendAttachmentState	 = getVkPipelineColorBlendAttachmentState();

	vk::PipelineViewportStateCreateInfo pipelineViewportStateCreateInfo {};
	pipelineViewportStateCreateInfo.viewportCount = 1;
	pipelineViewportStateCreateInfo.pViewports	  = &viewport;
	pipelineViewportStateCreateInfo.scissorCount  = 1;
	pipelineViewportStateCreateInfo.pScissors	  = &scissor;

	vk::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo {};
	pipelineColorBlendStateCreateInfo.logicOpEnable	  = false;
	pipelineColorBlendStateCreateInfo.attachmentCount = 1;
	pipelineColorBlendStateCreateInfo.pAttachments	  = &pipelineColorBlendAttachmentState;


	// FIXME pipeline layout creation must be in RendererBase since it's not graphics exclusive

	auto descriptorSetLayouts = getVkDescriptorSetLayouts();
	// TODO don't like this solution think on something better?
	descriptorSetLayouts.push_back(Engine::Managers::MaterialManager::getVkDescriptorSetLayout());

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


	std::vector<vk::Pipeline> graphicsPipelines {};
	for (uint shaderTypeIndex = 0; shaderTypeIndex < shaderTypeCount; shaderTypeIndex++) {
		for (uint meshTypeIndex = 0; meshTypeIndex < meshTypeCount; meshTypeIndex++) {
			vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo {};

			auto vertexAttributeDescriptions =
				Engine::Managers::MeshManager::getVertexInputAttributeDescriptions(meshTypeIndex);
			pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = vertexAttributeDescriptions.size();
			pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions	   = vertexAttributeDescriptions.data();

			auto vertexBindingDescriptions =
				Engine::Managers::MeshManager::getVertexInputBindingDescriptions(meshTypeIndex);
			pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = vertexBindingDescriptions.size();
			pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions	 = vertexBindingDescriptions.data();

			auto flagCount = Engine::Managers::ShaderManager::getShaderFlagCount(shaderTypeIndex);

			for (uint signature = 0; signature < std::pow(2, flagCount); signature++) {
				uint32_t shaderStageCount = 0;
				vk::PipelineShaderStageCreateInfo pipelineShaderStageCreateInfos[6];

				auto shaderInfo = Engine::Managers::ShaderManager::getShaderInfo(renderPassIndex, shaderTypeIndex,
																				 meshTypeIndex, signature);
				for (uint shaderStageIndex = 0; shaderStageIndex < 6; shaderStageIndex++) {
					if (shaderInfo.shaderModules[shaderStageIndex] != vk::ShaderModule()) {
						switch (shaderStageIndex) {
						case 0:
							pipelineShaderStageCreateInfos[shaderStageCount].stage = vk::ShaderStageFlagBits::eVertex;
							break;
						case 1:
							pipelineShaderStageCreateInfos[shaderStageCount].stage = vk::ShaderStageFlagBits::eGeometry;
							break;
						case 2:
							pipelineShaderStageCreateInfos[shaderStageCount].stage =
								vk::ShaderStageFlagBits::eTessellationControl;
							break;
						case 3:
							pipelineShaderStageCreateInfos[shaderStageCount].stage =
								vk::ShaderStageFlagBits::eTessellationEvaluation;
							break;
						case 4:
							pipelineShaderStageCreateInfos[shaderStageCount].stage = vk::ShaderStageFlagBits::eFragment;
							break;
						case 5:
							pipelineShaderStageCreateInfos[shaderStageCount].stage = vk::ShaderStageFlagBits::eCompute;
							break;
						}
						pipelineShaderStageCreateInfos[shaderStageCount].module =
							shaderInfo.shaderModules[shaderStageIndex];
						pipelineShaderStageCreateInfos[shaderStageCount].pName = "main";
						shaderStageCount++;
					}
				}

				vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo {};
				graphicsPipelineCreateInfo.stageCount = shaderStageCount;
				graphicsPipelineCreateInfo.pStages	  = pipelineShaderStageCreateInfos;

				graphicsPipelineCreateInfo.pVertexInputState   = &pipelineVertexInputStateCreateInfo;
				graphicsPipelineCreateInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
				graphicsPipelineCreateInfo.pViewportState	   = &pipelineViewportStateCreateInfo;
				graphicsPipelineCreateInfo.pRasterizationState = &pipelineRasterizationStateCreateInfo;
				graphicsPipelineCreateInfo.pMultisampleState   = &pipelineMultisampleStateCreateInfo;
				graphicsPipelineCreateInfo.pDepthStencilState  = &pipelineDepthStencilStateCreateInfo;
				graphicsPipelineCreateInfo.pColorBlendState	   = &pipelineColorBlendStateCreateInfo;
				graphicsPipelineCreateInfo.pDynamicState	   = nullptr;

				graphicsPipelineCreateInfo.layout = vkPipelineLayout;

				graphicsPipelineCreateInfo.renderPass = vkRenderPass;
				graphicsPipelineCreateInfo.subpass	  = 0;

				vk::Pipeline pipeline;
				result = vkDevice.createGraphicsPipelines(nullptr, 1, &graphicsPipelineCreateInfo, nullptr, &pipeline);
				if (result != vk::Result::eSuccess) {
					spdlog::error("Failed to create graphics pipeline. Error code: {} ({})", result,
								  vk::to_string(result));
					return 1;
				}

				graphicsPipelines.push_back(pipeline);
			}
		}
	}
	vkPipelines = graphicsPipelines;


	return 0;
}
} // namespace Engine::Renderers::Graphics