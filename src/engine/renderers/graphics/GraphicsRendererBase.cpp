#include "GraphicsRendererBase.hpp"

#include "engine/managers/GraphicsShaderManager.hpp"
#include "engine/managers/MeshManager.hpp"

#include <spdlog/spdlog.h>


namespace Engine {
int GraphicsRendererBase::init() {
	if (createRenderPass()) {
		return 1;
	}

	if (createFramebuffer()) {
		return 1;
	}

	if (createVkPipelineLayout()) {
		return 1;
	}

	if (createGraphicsPipelines()) {
		return 1;
	}

	return 0;
}


int GraphicsRendererBase::render(const vk::CommandBuffer* pPrimaryCommandBuffers,
								 const vk::CommandBuffer* pSecondaryCommandBuffers,
								 const vk::QueryPool& timestampQueryPool, double dt) {
	for (uint layerIndex = 0; layerIndex < getLayerCount(); layerIndex++) {
		vk::CommandBufferBeginInfo commandBufferBeginInfo {};

		vk::RenderPassBeginInfo renderPassBeginInfo {};
		renderPassBeginInfo.renderPass		  = vkRenderPass;
		renderPassBeginInfo.framebuffer		  = vkFramebuffers[layerIndex];
		renderPassBeginInfo.renderArea.extent = outputSize;
		renderPassBeginInfo.renderArea.offset = vk::Offset2D(0, 0);

		auto clearValues					= getVkClearValues();
		renderPassBeginInfo.clearValueCount = clearValues.size();
		renderPassBeginInfo.pClearValues	= clearValues.data();


		auto& commandBuffer = pPrimaryCommandBuffers[layerIndex];

		auto result = commandBuffer.begin(&commandBufferBeginInfo);
		if (result != vk::Result::eSuccess) {
			spdlog::error("Failed to record command buffer. Error code: {} ({})", result, vk::to_string(result));
			return 1;
		}

		auto queryIndex = layerIndex * getMultiviewLayerCount() * 2;

		commandBuffer.resetQueryPool(timestampQueryPool, queryIndex, getMultiviewLayerCount() * 2);
		commandBuffer.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, timestampQueryPool, queryIndex);


		commandBuffer.beginRenderPass(&renderPassBeginInfo, vk::SubpassContents::eSecondaryCommandBuffers);

		vk::CommandBufferInheritanceInfo commandBufferInheritanceInfo {};
		commandBufferInheritanceInfo.renderPass	 = vkRenderPass;
		commandBufferInheritanceInfo.framebuffer = vkFramebuffers[layerIndex];

		commandBufferBeginInfo.flags			= vk::CommandBufferUsageFlagBits::eRenderPassContinue;
		commandBufferBeginInfo.pInheritanceInfo = &commandBufferInheritanceInfo;

		auto pSecondaryCommandBuffersForLayer = pSecondaryCommandBuffers + layerIndex * threadCount;

		for (uint threadIndex = 0; threadIndex < threadCount; threadIndex++) {
			result = pSecondaryCommandBuffersForLayer[threadIndex].begin(&commandBufferBeginInfo);
			if (result != vk::Result::eSuccess) {
				spdlog::error("Failed to record secondary command buffer. Error code: {} ({})", result,
							  vk::to_string(result));
				return 1;
			}
		}

		recordSecondaryCommandBuffers(pSecondaryCommandBuffersForLayer, layerIndex, dt);

		for (uint threadIndex = 0; threadIndex < threadCount; threadIndex++) {
			result = pSecondaryCommandBuffersForLayer[threadIndex].end();
			if (result != vk::Result::eSuccess) {
				spdlog::error("Failed to record secondary command buffer. Error code: {} ({})", result,
							  vk::to_string(result));
				return 1;
			}
		}

		commandBuffer.executeCommands(threadCount, pSecondaryCommandBuffersForLayer);

		commandBuffer.endRenderPass();


		commandBuffer.writeTimestamp(vk::PipelineStageFlagBits::eFragmentShader, timestampQueryPool,
									 queryIndex + getMultiviewLayerCount());

		result = commandBuffer.end();
		if (result != vk::Result::eSuccess) {
			spdlog::error("Failed to record command buffer. Error code: {} ({})", result, vk::to_string(result));
			return 1;
		}
	}

	return 0;
}


int GraphicsRendererBase::createRenderPass() {
	auto attachmentReferences = getVkAttachmentReferences();

	assert(attachmentReferences.size() == outputs.size());

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

	const auto multiviewLayerCount = getMultiviewLayerCount();
	assert(multiviewLayerCount > 0);
	uint32_t viewMask = (1 << multiviewLayerCount) - 1;

	vk::RenderPassMultiviewCreateInfo renderPassMultiviewCreateInfo {};
	renderPassMultiviewCreateInfo.subpassCount = 1;
	renderPassMultiviewCreateInfo.pViewMasks   = &viewMask;

	vk::RenderPassCreateInfo renderPassCreateInfo {};
	renderPassCreateInfo.attachmentCount = attachmentDescriptions.size();
	renderPassCreateInfo.pAttachments	 = attachmentDescriptions.data();
	renderPassCreateInfo.subpassCount	 = 1;
	renderPassCreateInfo.pSubpasses		 = &subpassDescription;

	if (multiviewLayerCount > 1) {
		renderPassCreateInfo.pNext = &renderPassMultiviewCreateInfo;
	}

	auto result = vkDevice.createRenderPass(&renderPassCreateInfo, nullptr, &vkRenderPass);
	if (result != vk::Result::eSuccess) {
		spdlog::error("Failed to create render pass. Error code: {} ({})", result, vk::to_string(result));
		return 1;
	}

	return 0;
}

int GraphicsRendererBase::createFramebuffer() {
	assert(vkRenderPass != vk::RenderPass());

	auto layerCount			 = getLayerCount();
	auto multiviewLayerCount = getMultiviewLayerCount();

	vkFramebuffers.resize(layerCount);

	// Create framebuffer for each layer
	for (uint layerIndex = 0; layerIndex < layerCount; layerIndex++) {
		std::vector<vk::ImageView> imageViews(outputs.size());

		for (uint i = 0; i < imageViews.size(); i++) {
			const auto image = TextureManager::getTextureInfo(outputs[i]).image;
			vk::ImageView imageView {};

			vk::ImageViewCreateInfo imageViewCreateInfo {};
			imageViewCreateInfo.image = image;

			outputs[i].apply([&](const auto& texture) {
				imageViewCreateInfo.viewType						= vk::ImageViewType::e2D;
				imageViewCreateInfo.format							= texture.format;
				imageViewCreateInfo.subresourceRange.aspectMask		= texture.imageAspect;
				imageViewCreateInfo.subresourceRange.baseMipLevel	= 0;
				imageViewCreateInfo.subresourceRange.levelCount		= 1;
				imageViewCreateInfo.subresourceRange.baseArrayLayer = layerIndex * multiviewLayerCount;
				imageViewCreateInfo.subresourceRange.layerCount		= multiviewLayerCount;
			});

			auto result = vkDevice.createImageView(&imageViewCreateInfo, nullptr, &imageView);
			if (result != vk::Result::eSuccess) {
				spdlog::error("Failed to create image view. Error code: {} ({})", result,
							  vk::to_string(vk::Result(result)));
				return 1;
			}

			imageViews[i] = imageView;

			assert(imageViews[i] != vk::ImageView());
		}

		vk::FramebufferCreateInfo framebufferCreateInfo {};
		framebufferCreateInfo.renderPass	  = vkRenderPass;
		framebufferCreateInfo.attachmentCount = imageViews.size();
		framebufferCreateInfo.pAttachments	  = imageViews.data();

		framebufferCreateInfo.width	 = outputSize.width;
		framebufferCreateInfo.height = outputSize.height;
		framebufferCreateInfo.layers = 1;

		vk::Framebuffer framebuffer;
		auto result = vkDevice.createFramebuffer(&framebufferCreateInfo, nullptr, &framebuffer);
		if (result != vk::Result::eSuccess) {
			spdlog::error("Failed to create framebuffer. Error code: {} ({})", result, vk::to_string(result));
			return 1;
		}

		vkFramebuffers[layerIndex] = framebuffer;
	}

	return 0;
}

int GraphicsRendererBase::createGraphicsPipelines() {
	constexpr auto shaderTypeCount = GraphicsShaderManager::getTypeCount();
	constexpr auto meshTypeCount   = MeshManager::getTypeCount();

	auto renderPassIndex = GraphicsShaderManager::getRenderPassIndex(getRenderPassName());

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


	std::vector<vk::Pipeline> graphicsPipelines {};
	for (uint shaderTypeIndex = 0; shaderTypeIndex < shaderTypeCount; shaderTypeIndex++) {
		for (uint meshTypeIndex = 0; meshTypeIndex < meshTypeCount; meshTypeIndex++) {
			vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo {};

			auto vertexAttributeDescriptions = MeshManager::getVertexInputAttributeDescriptions(meshTypeIndex);
			pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = vertexAttributeDescriptions.size();
			pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions	   = vertexAttributeDescriptions.data();

			auto vertexBindingDescriptions = MeshManager::getVertexInputBindingDescriptions(meshTypeIndex);
			pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = vertexBindingDescriptions.size();
			pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions	 = vertexBindingDescriptions.data();

			auto signatureCount = GraphicsShaderManager::getShaderSignatureCount(shaderTypeIndex);

			for (uint signature = 0; signature < signatureCount; signature++) {
				uint32_t shaderStageCount = 0;
				vk::PipelineShaderStageCreateInfo pipelineShaderStageCreateInfos[6];

				auto shaderInfo =
					GraphicsShaderManager::getShaderInfo(renderPassIndex, shaderTypeIndex, meshTypeIndex, signature);
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
				auto result =
					vkDevice.createGraphicsPipelines(nullptr, 1, &graphicsPipelineCreateInfo, nullptr, &pipeline);
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
} // namespace Engine