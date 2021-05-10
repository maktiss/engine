#include "ShadowMapRenderer.hpp"

#include "engine/managers/EntityManager.hpp"

#include <glm/glm.hpp>


namespace Engine {
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

	glm::vec3 cameraPos;
	glm::vec3 cameraViewDir;

	EntityManager::forEach<TransformComponent, CameraComponent>([&](const auto& transform, const auto& camera) {
		// TODO: Check if active camera
		auto viewVector = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
		viewVector		= glm::rotate(transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)) * viewVector;
		viewVector		= glm::rotate(transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)) * viewVector;
		viewVector		= glm::rotate(transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f)) * viewVector;

		cameraPos	  = transform.position;
		cameraViewDir = glm::vec3(viewVector);
	});

	// FIXME
	const float cascadeHalfSizes[] = { 2.0f, 4.0f, 8.0f };
	const float cascadeHalfSize	   = cascadeHalfSizes[layerIndex];

	EntityManager::forEach<TransformComponent, LightComponent>([&](const auto& transform, auto& light) {
		if (light.castsShadows) {
			if (light.type == LightComponent::Type::DIRECTIONAL) {
				auto lightDirection = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
				lightDirection		= glm::rotate(transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)) * lightDirection;
				lightDirection		= glm::rotate(transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)) * lightDirection;
				lightDirection		= glm::rotate(transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f)) * lightDirection;

				glm::vec3 position = cameraPos + cascadeHalfSize * directionalLightCascadeOffset * cameraViewDir;

				cameraBlock.viewMatrix =
					glm::lookAtLH(position, position + glm::vec3(lightDirection), glm::vec3(0.0f, 1.0f, 0.0f));

				cameraBlock.projectionMatrix = glm::orthoLH_ZO(-cascadeHalfSize, cascadeHalfSize, -cascadeHalfSize,
															   cascadeHalfSize, -1000.0f, 1000.0f);

				light.shadowMapIndex = 0;
			}
		}
	});

	cameraBlock.invViewMatrix		= glm::inverse(cameraBlock.viewMatrix);
	cameraBlock.invProjectionMatrix = glm::inverse(cameraBlock.projectionMatrix);

	descriptorSetArrays[1].updateBuffer(layerIndex, 0, &cameraBlock, sizeof(cameraBlock));


	drawObjects(pSecondaryCommandBuffers, Frustum(cameraBlock.projectionMatrix * cameraBlock.viewMatrix));
}
} // namespace Engine