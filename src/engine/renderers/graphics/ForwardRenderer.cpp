#include "ForwardRenderer.hpp"

#include "engine/managers/EntityManager.hpp"

#include <glm/glm.hpp>


namespace Engine::Renderers::Graphics {
int ForwardRenderer::init() {
	spdlog::info("Initializing ForwardRenderer...");

	assert(vkDevice != vk::Device());
	assert(outputSize != vk::Extent2D());


	vk::SamplerCreateInfo samplerCreateInfo {};
	samplerCreateInfo.minFilter				  = vk::Filter::eLinear;
	samplerCreateInfo.magFilter				  = vk::Filter::eLinear;
	samplerCreateInfo.addressModeU			  = vk::SamplerAddressMode::eClampToEdge;
	samplerCreateInfo.addressModeV			  = vk::SamplerAddressMode::eClampToEdge;
	samplerCreateInfo.addressModeW			  = vk::SamplerAddressMode::eClampToEdge;
	samplerCreateInfo.anisotropyEnable		  = false;
	samplerCreateInfo.maxAnisotropy			  = 0.0f;
	samplerCreateInfo.unnormalizedCoordinates = false;
	samplerCreateInfo.compareEnable			  = true;
	samplerCreateInfo.compareOp				  = vk::CompareOp::eLessOrEqual;
	samplerCreateInfo.mipmapMode			  = vk::SamplerMipmapMode::eLinear;
	samplerCreateInfo.mipLodBias			  = 0.0f;
	samplerCreateInfo.minLod				  = 0.0f;
	samplerCreateInfo.maxLod				  = 0.0f;

	vk::Sampler sampler;
	auto result = vkDevice.createSampler(&samplerCreateInfo, nullptr, &sampler);
	if (result != vk::Result::eSuccess) {
		spdlog::error("[ForwardRenderer] Failed to create image sampler. Error code: {} ({})", result,
					  vk::to_string(vk::Result(result)));
		return 1;
	}


	descriptorSetArrays.resize(3);
	
	descriptorSetArrays[0].setBindingCount(2);
	descriptorSetArrays[0].setBindingLayoutInfo(0, vk::DescriptorType::eUniformBuffer, 4);
	descriptorSetArrays[0].setBindingLayoutInfo(1, vk::DescriptorType::eCombinedImageSampler, 0);
	descriptorSetArrays[0].init(vkDevice, vmaAllocator);
	descriptorSetArrays[0].updateImage(0, 1, sampler, Engine::Managers::TextureManager::getTextureInfo(inputs[0]).imageView);
	
	descriptorSetArrays[1].setBindingLayoutInfo(0, vk::DescriptorType::eUniformBuffer, 256);
	descriptorSetArrays[1].init(vkDevice, vmaAllocator);

	descriptorSetArrays[2].setBindingLayoutInfo(0, vk::DescriptorType::eUniformBuffer, sizeof(EnvironmentBlock));
	descriptorSetArrays[2].init(vkDevice, vmaAllocator);


	return GraphicsRendererBase::init();
}


void ForwardRenderer::recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers, uint layerIndex,
													double dt) {
	const auto& commandBuffer = pSecondaryCommandBuffers[0];

	for (uint setIndex = 0; setIndex < descriptorSetArrays.size(); setIndex++) {
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, vkPipelineLayout, setIndex, 1,
										 &descriptorSetArrays[setIndex].getVkDescriptorSet(0), 0, nullptr);
	}


	struct {
		glm::mat4 viewMatrix;
		glm::mat4 projectionMatrix;

		glm::mat4 invViewMatrix;
		glm::mat4 invProjectionMatrix;
	} cameraBlock;

	Engine::Managers::EntityManager::forEach<Engine::Components::Transform, Engine::Components::Camera>(
		[&cameraBlock](auto& transform, auto& camera) {
			glm::vec4 viewVector   = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
			viewVector			   = glm::rotate(transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)) * viewVector;
			viewVector			   = glm::rotate(transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)) * viewVector;
			viewVector			   = glm::rotate(transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f)) * viewVector;
			cameraBlock.viewMatrix = glm::lookAtLH(
				transform.position, transform.position + glm::vec3(viewVector.x, viewVector.y, viewVector.z),
				glm::vec3(0.0f, 1.0f, 0.0f));

			cameraBlock.projectionMatrix = camera.getProjectionMatrix();
		});

	cameraBlock.invViewMatrix		= glm::inverse(cameraBlock.viewMatrix);
	cameraBlock.invProjectionMatrix = glm::inverse(cameraBlock.projectionMatrix);

	descriptorSetArrays[1].updateBuffer(0, 0, &cameraBlock, sizeof(cameraBlock));


	EnvironmentBlock environmentBlock;

	Engine::Managers::EntityManager::forEach<Engine::Components::Transform, Engine::Components::Light>(
		[&environmentBlock, &cameraBlock](const auto& transform, const auto& light) {
			if (light.type == Engine::Components::Light::Type::DIRECTIONAL) {
				glm::vec4 direction = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
				direction			= glm::rotate(transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)) * direction;
				direction			= glm::rotate(transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)) * direction;
				direction			= glm::rotate(transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f)) * direction;

				environmentBlock.directionalLight.direction = glm::vec3(cameraBlock.viewMatrix * direction);
				environmentBlock.directionalLight.color		= light.color;
				environmentBlock.directionalLight.shadowMapIndex = light.shadowMapIndex;

				for (uint cascadeIndex = 0; cascadeIndex < 3; cascadeIndex++) {
					auto& lightSpaceMatrix = environmentBlock.directionalLight.lightSpaceMatrices[cascadeIndex];
				
					// FIXME
					const float cascadeHalfSizes[] = { 2.0f, 4.0f, 8.0f };
					const float cascadeHalfSize	   = cascadeHalfSizes[cascadeIndex];

					lightSpaceMatrix = glm::lookAtLH(
						transform.position, transform.position + glm::vec3(direction),
						glm::vec3(0.0f, 1.0f, 0.0f));

					lightSpaceMatrix = glm::orthoLH_ZO(-cascadeHalfSize, cascadeHalfSize, -cascadeHalfSize,
																   cascadeHalfSize, -cascadeHalfSize, cascadeHalfSize) * lightSpaceMatrix;

					// lightSpaceMatrix *= cameraBlock.invViewMatrix;
				}
			}
		});

	descriptorSetArrays[2].updateBuffer(0, 0, &environmentBlock, sizeof(environmentBlock));


	Engine::Managers::EntityManager::forEach<Engine::Components::Transform, Engine::Components::Model>(
		[&commandBuffer, pipelineLayout = vkPipelineLayout, &graphicsPipelines = vkPipelines](auto& transform,
																							  auto& model) {
			const auto& meshInfo	 = Engine::Managers::MeshManager::getMeshInfo(model.meshHandles[0]);
			const auto& materialInfo = Engine::Managers::MaterialManager::getMaterialInfo(model.materialHandles[0]);

			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
									   graphicsPipelines[model.shaderHandles[0].getIndex()]);


			auto transformMatrix = transform.getTransformMatrix();
			commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eAll, 0, 64, &transformMatrix);

			auto vertexBuffer		 = meshInfo.vertexBuffer.getVkBuffer();
			vk::DeviceSize offsets[] = { 0 };
			commandBuffer.bindVertexBuffers(0, 1, &vertexBuffer, offsets);

			auto indexBuffer = meshInfo.indexBuffer.getVkBuffer();
			commandBuffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);

			commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 3, 1,
											 &materialInfo.descriptorSet, 0, nullptr);

			commandBuffer.drawIndexed(meshInfo.indexCount, 1, 0, 0, 0);
		});
}
} // namespace Engine::Renderers::Graphics