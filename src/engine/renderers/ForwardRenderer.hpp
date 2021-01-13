#pragma once

#include "RendererBase.hpp"


namespace Engine::Renderers {
class ForwardRenderer : public RendererBase {
public:
	int init() override {
		spdlog::info("Initializing ForwardRenderer...");

		assert(vkDevice != vk::Device());
		assert(outputSize != vk::Extent2D());

		outputs.resize(2);
		inputs.resize(0);

		outputDescriptions.resize(outputs.size());
		inputDescriptions.resize(inputs.size());

		vkClearValues.resize(outputs.size());
		vkClearValues[0].color = std::array { 0.0f, 0.0f, 0.0f, 0.0f };
		vkClearValues[1].depthStencil.depth = 1.0f;

		outputDescriptions[0].format = vk::Format::eR8G8B8A8Srgb;
		outputDescriptions[0].usage	 = vk::ImageUsageFlagBits::eColorAttachment;

		outputDescriptions[1].format = vk::Format::eD24UnormS8Uint;
		outputDescriptions[1].usage	 = vk::ImageUsageFlagBits::eDepthStencilAttachment;

		vkAttachmentDescriptions.resize(outputs.size() + inputs.size());

		vkAttachmentDescriptions[0].format		   = outputDescriptions[0].format;
		vkAttachmentDescriptions[0].samples		   = vk::SampleCountFlagBits::e1;
		vkAttachmentDescriptions[0].loadOp		   = vk::AttachmentLoadOp::eClear;
		vkAttachmentDescriptions[0].storeOp		   = vk::AttachmentStoreOp::eStore;
		vkAttachmentDescriptions[0].stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
		vkAttachmentDescriptions[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

		vkAttachmentDescriptions[1].format		   = outputDescriptions[1].format;
		vkAttachmentDescriptions[1].samples		   = vk::SampleCountFlagBits::e1;
		vkAttachmentDescriptions[1].loadOp		   = vk::AttachmentLoadOp::eClear;
		vkAttachmentDescriptions[1].storeOp		   = vk::AttachmentStoreOp::eStore;
		vkAttachmentDescriptions[1].stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
		vkAttachmentDescriptions[1].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

		// set by RendringSystem based on render graph
		vkAttachmentDescriptions[0].initialLayout = vk::ImageLayout::eUndefined;
		vkAttachmentDescriptions[0].finalLayout	  = vk::ImageLayout::eUndefined;


		vkAttachmentReferences.resize(outputs.size() + inputs.size());
		vkAttachmentReferences[0].attachment = 0;
		vkAttachmentReferences[0].layout	 = vk::ImageLayout::eColorAttachmentOptimal;
		vkAttachmentReferences[1].attachment = 1;
		vkAttachmentReferences[1].layout	 = vk::ImageLayout::eDepthStencilAttachmentOptimal;

		VkPipelineBindPoint = vk::PipelineBindPoint::eGraphics;


		vkPipelineInputAssemblyStateCreateInfo.topology				  = vk::PrimitiveTopology::eTriangleList;
		vkPipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = false;

		vkViewport.x		= 0.0f;
		vkViewport.y		= outputSize.height;
		vkViewport.width	= outputSize.width;
		vkViewport.height	= -(static_cast<int>(outputSize.height));
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

		vkPipelineDepthStencilStateCreateInfo.depthTestEnable = true;
		vkPipelineDepthStencilStateCreateInfo.depthWriteEnable = true;
		vkPipelineDepthStencilStateCreateInfo.depthCompareOp = vk::CompareOp::eLess;

		vkPipelineColorBlendAttachmentState.colorWriteMask =
			vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
			vk::ColorComponentFlagBits::eA;
		vkPipelineColorBlendAttachmentState.blendEnable = false;


		uniformBuffers.resize(3);
		uniformBuffers[0].init(vkDevice, vmaAllocator, 4);
		uniformBuffers[1].init(vkDevice, vmaAllocator, 256);
		uniformBuffers[2].init(vkDevice, vmaAllocator, 16);


		return 0;
	}


	// Records command buffer within renderpass
	void recordCommandBuffer(double dt, vk::CommandBuffer& commandBuffer) {
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vkGraphicsPipelines[0]);

		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
										 vkPipelineLayout,
										 0,
										 1,
										 &uniformBuffers[0].getVkDescriptorSet(),
										 0,
										 nullptr);
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
										 vkPipelineLayout,
										 1,
										 1,
										 &uniformBuffers[1].getVkDescriptorSet(),
										 0,
										 nullptr);
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
										 vkPipelineLayout,
										 2,
										 1,
										 &uniformBuffers[2].getVkDescriptorSet(),
										 0,
										 nullptr);

		struct {
			glm::mat4 viewMatrix;
			glm::mat4 projectionMatrix;

			glm::mat4 invViewMatrix;
			glm::mat4 invProjectionMatrix;
		} cameraBlock;

		Engine::Managers::EntityManager::forEach<Engine::Components::Transform, Engine::Components::Camera>(
			[&cameraBlock](auto& transform, auto& camera) {
				glm::vec4 viewVector = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
				viewVector = glm::rotate(transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)) * viewVector;
				viewVector = glm::rotate(transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)) * viewVector;
				viewVector = glm::rotate(transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f)) * viewVector;
				cameraBlock.viewMatrix = glm::lookAtLH(transform.position,
													   transform.position + glm::vec3(viewVector.x, viewVector.y, viewVector.z),
													   glm::vec3(0.0f, 1.0f, 0.0f));

				cameraBlock.projectionMatrix = camera.getProjectionMatrix();
			});

		cameraBlock.invViewMatrix		= glm::inverse(cameraBlock.viewMatrix);
		cameraBlock.invProjectionMatrix = glm::inverse(cameraBlock.projectionMatrix);

		uniformBuffers[1].update(&cameraBlock, sizeof(cameraBlock));


		Engine::Managers::EntityManager::forEach<Engine::Components::Transform, Engine::Components::Model>(
			[&commandBuffer, pipelineLayout = vkPipelineLayout](auto& transform, auto& model) {
				auto transformMatrix = transform.getTransformMatrix();
				commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eAll, 0, 64, &transformMatrix);

				const auto& meshInfo	 = Engine::Managers::MeshManager::getMeshInfo(model.meshHandles[0]);
				const auto& materialInfo = Engine::Managers::MaterialManager::getMaterialInfo(model.materialHandles[0]);

				auto vertexBuffer		 = meshInfo.vertexBuffer.getVkBuffer();
				vk::DeviceSize offsets[] = { 0 };
				commandBuffer.bindVertexBuffers(0, 1, &vertexBuffer, offsets);

				auto indexBuffer = meshInfo.indexBuffer.getVkBuffer();
				commandBuffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);

				commandBuffer.bindDescriptorSets(
					vk::PipelineBindPoint::eGraphics, pipelineLayout, 3, 1, &materialInfo.descriptorSet, 0, nullptr);

				commandBuffer.drawIndexed(meshInfo.indexCount, 1, 0, 0, 0);
			});
	}

	const char* getRenderPassName() const override {
		return "RENDER_PASS_GEOMETRY";
	}
};
} // namespace Engine::Renderers