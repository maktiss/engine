#include "GraphicsRendererBase.hpp"

#include "engine/managers/GraphicsShaderManager.hpp"
#include "engine/managers/MeshManager.hpp"

#include <spdlog/spdlog.h>


namespace Engine {
int GraphicsRendererBase::init() {
	if (RendererBase::init()) {
		return 1;
	}

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

		auto queryIndex = layerIndex * 2;

		commandBuffer.resetQueryPool(timestampQueryPool, queryIndex, 2);
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


		// Transition input image layouts

		if (layerIndex == (getLayerCount() - 1)) {
			for (uint inputIndex = 0; inputIndex < inputs.size(); inputIndex++) {
				const auto textureInfo = TextureManager::getTextureInfo(inputs[inputIndex]);

				vk::ImageMemoryBarrier imageMemoryBarrier {};
				imageMemoryBarrier.image						   = textureInfo.image;
				imageMemoryBarrier.subresourceRange.aspectMask	   = textureInfo.imageAspect;
				imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
				imageMemoryBarrier.subresourceRange.layerCount	   = textureInfo.arrayLayers;
				imageMemoryBarrier.subresourceRange.baseMipLevel   = 0;
				imageMemoryBarrier.subresourceRange.levelCount	   = textureInfo.mipLevels;
				imageMemoryBarrier.oldLayout					   = vkInputInitialLayouts[inputIndex];
				imageMemoryBarrier.newLayout					   = vkInputFinalLayouts[inputIndex];
				imageMemoryBarrier.srcAccessMask				   = {};
				imageMemoryBarrier.dstAccessMask				   = {};

				commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eBottomOfPipe,
											  vk::PipelineStageFlagBits::eBottomOfPipe, {}, 0, nullptr, 0, nullptr, 1,
											  &imageMemoryBarrier);
			}
		}


		commandBuffer.writeTimestamp(vk::PipelineStageFlagBits::eFragmentShader, timestampQueryPool, queryIndex + 1);

		result = commandBuffer.end();

		if (result != vk::Result::eSuccess) {
			spdlog::error("Failed to record command buffer. Error code: {} ({})", result, vk::to_string(result));
			return 1;
		}
	}

	return 0;
}


uint GraphicsRendererBase::getColorAttachmentCount() const {
	const auto attachmentReferences = getVkAttachmentReferences();

	// Depth attachment should always be last
	for (uint i = 0; i < attachmentReferences.size(); i++) {
		if (i != (attachmentReferences.size() - 1)) {
			// FIXME: check for all depth layout variations
			assert(attachmentReferences[i].layout != vk::ImageLayout::eDepthStencilAttachmentOptimal);
		}
	}

	auto colorAttachmentCount = getOutputCount();

	const auto& lastAttachmentReference = attachmentReferences[attachmentReferences.size() - 1];

	// FIXME: check for all depth layout variations
	if (lastAttachmentReference.layout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
		colorAttachmentCount--;
	}

	return colorAttachmentCount;
}


std::vector<vk::AttachmentReference> GraphicsRendererBase::getVkAttachmentReferences() const {
	const auto& outputInitialLayouts = getOutputInitialLayouts();

	std::vector<vk::AttachmentReference> attachmentReferences {};
	attachmentReferences.resize(outputInitialLayouts.size());

	for (uint i = 0; i < attachmentReferences.size(); i++) {
		attachmentReferences[i].attachment = i;
		attachmentReferences[i].layout	   = outputInitialLayouts[i];
	}

	return attachmentReferences;
}


int GraphicsRendererBase::createRenderPass() {
	auto attachmentReferences = getVkAttachmentReferences();

	assert(attachmentReferences.size() == outputs.size());

	vk::SubpassDescription subpassDescription {};
	subpassDescription.pipelineBindPoint	= vk::PipelineBindPoint::eGraphics;
	subpassDescription.colorAttachmentCount = attachmentReferences.size();
	subpassDescription.pColorAttachments	= attachmentReferences.data();


	// Set depth attachment
	if (getColorAttachmentCount() < getOutputCount()) {
		auto& lastAttachmentReference = attachmentReferences[attachmentReferences.size() - 1];

		subpassDescription.colorAttachmentCount--;
		subpassDescription.pDepthStencilAttachment = &lastAttachmentReference;
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
	const auto& pipelineColorBlendAttachmentStates	 = getVkPipelineColorBlendAttachmentStates();

	vk::PipelineViewportStateCreateInfo pipelineViewportStateCreateInfo {};
	pipelineViewportStateCreateInfo.viewportCount = 1;
	pipelineViewportStateCreateInfo.pViewports	  = &viewport;
	pipelineViewportStateCreateInfo.scissorCount  = 1;
	pipelineViewportStateCreateInfo.pScissors	  = &scissor;

	vk::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo {};
	pipelineColorBlendStateCreateInfo.logicOpEnable	  = false;
	pipelineColorBlendStateCreateInfo.attachmentCount = pipelineColorBlendAttachmentStates.size();
	pipelineColorBlendStateCreateInfo.pAttachments	  = pipelineColorBlendAttachmentStates.data();

	const std::array shaderStages = {
		vk::ShaderStageFlagBits::eVertex,
		vk::ShaderStageFlagBits::eGeometry,
		vk::ShaderStageFlagBits::eTessellationControl,
		vk::ShaderStageFlagBits::eTessellationEvaluation,
		vk::ShaderStageFlagBits::eFragment,
		vk::ShaderStageFlagBits::eCompute,
	};

	std::vector<vk::Pipeline> graphicsPipelines {};


	const auto specConstDescriptions = getSpecializationConstantDescriptions();

	std::vector<vk::SpecializationMapEntry> specializationMapEntries(specConstDescriptions.size());
	std::vector<uint8_t> specializationConstantBuffer {};

	for (uint i = 0; i < specConstDescriptions.size(); i++) {
		const auto& specConstDescription = specConstDescriptions[i];

		uint offset = specializationConstantBuffer.size();
		specializationConstantBuffer.resize(offset + specConstDescription.size);

		memcpy(&specializationConstantBuffer[offset], specConstDescription.pData, specConstDescription.size);

		auto& specializationMapEntry = specializationMapEntries[i];

		specializationMapEntry.constantID = specConstDescription.id;
		specializationMapEntry.offset	  = offset;
		specializationMapEntry.size		  = specConstDescription.size;
	}

	vk::SpecializationInfo specializationInfo {};
	specializationInfo.pData		 = specializationConstantBuffer.data();
	specializationInfo.dataSize		 = specializationConstantBuffer.size();
	specializationInfo.mapEntryCount = specializationMapEntries.size();
	specializationInfo.pMapEntries	 = specializationMapEntries.data();


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
					auto& pipelineShaderStageCreateInfo = pipelineShaderStageCreateInfos[shaderStageCount];

					auto& shaderModule = shaderInfo.shaderModules[shaderStageIndex];

					if (shaderModule != vk::ShaderModule()) {
						pipelineShaderStageCreateInfo.stage = shaderStages[shaderStageIndex];

						pipelineShaderStageCreateInfo.pSpecializationInfo = &specializationInfo;

						pipelineShaderStageCreateInfo.module = shaderModule;
						pipelineShaderStageCreateInfo.pName	 = "main";

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