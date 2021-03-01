#include "ShadowMapRenderer.hpp"

#include "engine/managers/EntityManager.hpp"

#include <glm/glm.hpp>


namespace Engine::Renderers::Graphics {
int ShadowMapRenderer::init() {
	spdlog::info("Initializing ShadowMapRenderer...");

	assert(vkDevice != vk::Device());
	assert(outputSize != vk::Extent2D());


	descriptorSetArrays.resize(3);
	
	descriptorSetArrays[0].setSetCount(getLayerCount());
	descriptorSetArrays[0].setBindingLayoutInfo(0, vk::DescriptorType::eUniformBuffer, 4);
	descriptorSetArrays[0].init(vkDevice, vmaAllocator);
	
	descriptorSetArrays[1].setSetCount(getLayerCount());
	descriptorSetArrays[1].setBindingLayoutInfo(0, vk::DescriptorType::eUniformBuffer, 256);
	descriptorSetArrays[1].init(vkDevice, vmaAllocator);

	descriptorSetArrays[2].setSetCount(getLayerCount());
	descriptorSetArrays[2].setBindingLayoutInfo(0, vk::DescriptorType::eUniformBuffer, 16);
	descriptorSetArrays[2].init(vkDevice, vmaAllocator);


	return GraphicsRendererBase::init();
}


void ShadowMapRenderer::recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers,
													  uint layerIndex, double dt) {
	const auto& commandBuffer = pSecondaryCommandBuffers[0];

	for (uint setIndex = 0; setIndex < descriptorSetArrays.size(); setIndex++) {
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, vkPipelineLayout, setIndex, 1,
										 &descriptorSetArrays[setIndex].getVkDescriptorSet(layerIndex), 0, nullptr);
	}


	struct {
		glm::mat4 viewMatrix;
		glm::mat4 projectionMatrix;

		glm::mat4 invViewMatrix;
		glm::mat4 invProjectionMatrix;
	} cameraBlock;

	const float cascadeHalfSizes[] = { 2.0f, 4.0f, 8.0f };
	const float cascadeHalfSize	   = cascadeHalfSizes[layerIndex];

	Engine::Managers::EntityManager::forEach<Engine::Components::Transform, Engine::Components::Light>(
		[cascadeHalfSize, &cameraBlock](const auto& transform, const auto& light) {
			if (light.castsShadows) {
				if (light.type == Engine::Components::Light::Type::DIRECTIONAL) {

					glm::vec4 viewVector = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
					viewVector			 = glm::rotate(transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)) * viewVector;
					viewVector			 = glm::rotate(transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)) * viewVector;
					viewVector			 = glm::rotate(transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f)) * viewVector;
					cameraBlock.viewMatrix = glm::lookAtLH(
						transform.position, transform.position + glm::vec3(viewVector.x, viewVector.y, viewVector.z),
						glm::vec3(0.0f, 1.0f, 0.0f));

					cameraBlock.projectionMatrix = glm::orthoLH_ZO(-cascadeHalfSize, cascadeHalfSize, -cascadeHalfSize,
																   cascadeHalfSize, -cascadeHalfSize, cascadeHalfSize);
				}
			}
		});

	cameraBlock.invViewMatrix		= glm::inverse(cameraBlock.viewMatrix);
	cameraBlock.invProjectionMatrix = glm::inverse(cameraBlock.projectionMatrix);

	descriptorSetArrays[1].updateBuffer(layerIndex, 0, &cameraBlock, sizeof(cameraBlock));


	Engine::Managers::EntityManager::forEach<Engine::Components::Transform, Engine::Components::Model>(
		[&commandBuffer, pipelineLayout = vkPipelineLayout, &pipelines = vkPipelines](auto& transform, auto& model) {
			const auto& meshInfo	 = Engine::Managers::MeshManager::getMeshInfo(model.meshHandles[0]);
			const auto& materialInfo = Engine::Managers::MaterialManager::getMaterialInfo(model.materialHandles[0]);

			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines[model.shaderHandles[0].getIndex()]);


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