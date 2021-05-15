#include "DepthNormalRenderer.hpp"

#include "engine/managers/EntityManager.hpp"

#include <glm/glm.hpp>


namespace Engine {
int DepthNormalRenderer::init() {
	spdlog::info("Initializing DepthNormalRenderer...");

	assert(vkDevice != vk::Device());
	assert(outputSize != vk::Extent2D());


	return ObjectRendererBase::init();
}


void DepthNormalRenderer::recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers,
														uint layerIndex, double dt) {
	// TODO: move to function
	for (uint threadIndex = 0; threadIndex < threadCount; threadIndex++) {
		const auto& commandBuffer = pSecondaryCommandBuffers[threadIndex];

		bindDescriptorSets(commandBuffer, vk::PipelineBindPoint::eGraphics);

		const auto textureDescriptorSet = TextureManager::getVkDescriptorSet();
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, vkPipelineLayout, 4, 1,
										 &textureDescriptorSet, 0, nullptr);
	}

	struct {
		glm::mat4 viewMatrix;
		glm::mat4 projectionMatrix;

		glm::mat4 invViewMatrix;
		glm::mat4 invProjectionMatrix;
	} cameraBlock;

	EntityManager::forEach<TransformComponent, CameraComponent>([&cameraBlock](auto& transform, auto& camera) {
		glm::vec4 viewVector = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
		viewVector			 = glm::rotate(transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)) * viewVector;
		viewVector			 = glm::rotate(transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)) * viewVector;
		viewVector			 = glm::rotate(transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f)) * viewVector;
		cameraBlock.viewMatrix =
			glm::lookAtLH(transform.position, transform.position + glm::vec3(viewVector.x, viewVector.y, viewVector.z),
						  glm::vec3(0.0f, 1.0f, 0.0f));

		cameraBlock.projectionMatrix = camera.getProjectionMatrix();
	});

	cameraBlock.invViewMatrix		= glm::inverse(cameraBlock.viewMatrix);
	cameraBlock.invProjectionMatrix = glm::inverse(cameraBlock.projectionMatrix);

	descriptorSetArrays[1].updateBuffer(0, 0, &cameraBlock, sizeof(cameraBlock));

	Frustum frustum { cameraBlock.projectionMatrix * cameraBlock.viewMatrix };
	drawObjects(pSecondaryCommandBuffers, &frustum, 1);
}
} // namespace Engine