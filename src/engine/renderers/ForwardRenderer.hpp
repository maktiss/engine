#pragma once

#include "RendererBase.hpp"


namespace Engine::Renderers {
class ForwardRenderer : public RendererBase {
public:
	int init() override {
		spdlog::info("Initializing ForwardRenderer...");

		assert(vkDevice != vk::Device());
		assert(outputSize != vk::Extent2D());

		outputs.resize(1);
		inputs.resize(0);

		outputDescriptions.resize(outputs.size());
		inputDescriptions.resize(inputs.size());

		outputDescriptions[0].format = vk::Format::eR8G8B8A8Srgb;
		outputDescriptions[0].usage	 = vk::ImageUsageFlagBits::eColorAttachment;

		vkAttachmentDescriptions.resize(outputs.size() + inputs.size());
		vkAttachmentDescriptions[0].format		   = outputDescriptions[0].format;
		vkAttachmentDescriptions[0].samples		   = vk::SampleCountFlagBits::e1;
		vkAttachmentDescriptions[0].loadOp		   = vk::AttachmentLoadOp::eDontCare;
		vkAttachmentDescriptions[0].storeOp		   = vk::AttachmentStoreOp::eStore;
		vkAttachmentDescriptions[0].stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
		vkAttachmentDescriptions[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

		// set by RendringSystem based on render graph
		vkAttachmentDescriptions[0].initialLayout = vk::ImageLayout::eUndefined;
		vkAttachmentDescriptions[0].finalLayout	  = vk::ImageLayout::eUndefined;


		vkAttachmentReferences.resize(outputs.size() + inputs.size());
		vkAttachmentReferences[0].attachment = 0;
		vkAttachmentReferences[0].layout	 = vk::ImageLayout::eColorAttachmentOptimal;

		VkPipelineBindPoint = vk::PipelineBindPoint::eGraphics;


		vkPipelineInputAssemblyStateCreateInfo.topology				  = vk::PrimitiveTopology::eTriangleList;
		vkPipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = false;

		vkViewport.x		= 0.0f;
		vkViewport.y		= 0.0f;
		vkViewport.width	= outputSize.width;
		vkViewport.height	= outputSize.height;
		vkViewport.minDepth = 0.0f;
		vkViewport.maxDepth = 1.0f;

		vkScissor.offset = vk::Offset2D(0, 0);
		vkScissor.extent = outputSize;

		vkPipelineRasterizationStateCreateInfo.depthClampEnable		   = false;
		vkPipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = false;
		vkPipelineRasterizationStateCreateInfo.polygonMode			   = vk::PolygonMode::eFill;
		vkPipelineRasterizationStateCreateInfo.cullMode				   = vk::CullModeFlagBits::eBack;
		vkPipelineRasterizationStateCreateInfo.frontFace			   = vk::FrontFace::eCounterClockwise;
		vkPipelineRasterizationStateCreateInfo.depthBiasEnable		   = false;
		vkPipelineRasterizationStateCreateInfo.lineWidth			   = 1.0f;

		vkPipelineMultisampleStateCreateInfo.sampleShadingEnable  = false;
		vkPipelineMultisampleStateCreateInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;

		vkPipelineColorBlendAttachmentState.colorWriteMask =
			vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
			vk::ColorComponentFlagBits::eA;
		vkPipelineColorBlendAttachmentState.blendEnable = false;


		return 0;
	}

	// int run(double dt, uint frameIndex) override {
	// 	auto commandBuffer = getCommandBuffer(frameIndex, 0);

	// 	vk::CommandBufferBeginInfo commandBufferBeginInfo {};

	// 	auto result = commandBuffer.begin(&commandBufferBeginInfo);
	// 	if (result != vk::Result::eSuccess) {
	// 		spdlog::error("[ForwardRenderer] Failed to record command buffer. Error code: {} ({})",
	// 					  result,
	// 					  vk::to_string(result));
	// 		return 1;
	// 	}

	// 	vk::RenderPassBeginInfo renderPassBeginInfo {};
	// 	renderPassBeginInfo.renderPass		  = vkRenderPass;
	// 	renderPassBeginInfo.framebuffer		  = vkFramebuffer;
	// 	renderPassBeginInfo.renderArea.extent = outputSize;
	// 	renderPassBeginInfo.renderArea.offset = vk::Offset2D(0, 0);

	// 	vk::ClearValue clearValue = { std::array<float, 4> { 1.0f, 0.0f, 0.0f, 1.0f } };

	// 	renderPassBeginInfo.clearValueCount = 1;
	// 	renderPassBeginInfo.pClearValues	= &clearValue;

	// 	commandBuffer.beginRenderPass(&renderPassBeginInfo, vk::SubpassContents::eInline);
	// 	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vkGraphicsPipelines[0]);

	// 	Engine::Managers::EntityManager::forEach<Engine::Components::Transform, Engine::Components::Model>(
	// 		[&commandBuffer](auto& transform, auto& model) {
	// 			const auto& meshInfo = Engine::Managers::MeshManager::getMeshInfo(model.meshHandles[0]);

	// 			auto vertexBuffer		 = meshInfo.vertexBuffer.getVkBuffer();
	// 			vk::DeviceSize offsets[] = { 0 };
	// 			commandBuffer.bindVertexBuffers(0, 1, &vertexBuffer, offsets);

	// 			auto indexBuffer = meshInfo.indexBuffer.getVkBuffer();
	// 			commandBuffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint16);

	// 			commandBuffer.drawIndexed(meshInfo.indexCount, 1, 0, 0, 0);
	// 		});

	// 	commandBuffer.endRenderPass();
	// 	result = commandBuffer.end();
	// 	if (result != vk::Result::eSuccess) {
	// 		spdlog::error("[ForwardRenderer] Failed to record command buffer. Error code: {} ({})",
	// 					  result,
	// 					  vk::to_string(result));
	// 		return 1;
	// 	}

	// 	return 0;
	// }


	// Records command buffer within renderpass
	void recordCommandBuffer(double dt, vk::CommandBuffer& commandBuffer) {
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vkGraphicsPipelines[0]);

		Engine::Managers::EntityManager::forEach<Engine::Components::Transform, Engine::Components::Model>(
			[&commandBuffer, pipelineLayout = vkPipelineLayout](auto& transform, auto& model) {
				transform.rotation.z += 0.001;
				auto transformMatrix = transform.getTransformMatrix();
				commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eAll, 0, 64, &transformMatrix);

				const auto& meshInfo	 = Engine::Managers::MeshManager::getMeshInfo(model.meshHandles[0]);
				const auto& materialInfo = Engine::Managers::MaterialManager::getMaterialInfo(model.materialHandles[0]);

				auto vertexBuffer		 = meshInfo.vertexBuffer.getVkBuffer();
				vk::DeviceSize offsets[] = { 0 };
				commandBuffer.bindVertexBuffers(0, 1, &vertexBuffer, offsets);

				auto indexBuffer = meshInfo.indexBuffer.getVkBuffer();
				commandBuffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint16);

				commandBuffer.bindDescriptorSets(
					vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, &materialInfo.descriptorSet, 0, nullptr);

				commandBuffer.drawIndexed(meshInfo.indexCount, 1, 0, 0, 0);
			});
	}

	const char* getRenderPassName() const override {
		return "RENDER_PASS_GEOMETRY";
	}
};
} // namespace Engine::Renderers