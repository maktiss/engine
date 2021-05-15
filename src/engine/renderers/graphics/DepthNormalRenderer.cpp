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
														uint layerIndex, uint descriptorSetIndex, double dt) {

	CameraBlock cameraBlock;

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

	descriptorSetArrays[0].updateBuffer(descriptorSetIndex, 0, &cameraBlock, sizeof(cameraBlock));

	Frustum frustum { cameraBlock.projectionMatrix * cameraBlock.viewMatrix };
	drawObjects(pSecondaryCommandBuffers, &frustum, 1);
}
} // namespace Engine