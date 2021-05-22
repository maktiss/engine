#include "DepthNormalRenderer.hpp"

#include "engine/managers/EntityManager.hpp"

#include <glm/glm.hpp>


namespace Engine {
int DepthNormalRenderer::init() {
	spdlog::info("Initializing DepthNormalRenderer...");

	assert(vkDevice != vk::Device());
	assert(outputSize != vk::Extent2D());

	objectRenderer.setThreadCount(threadCount);
	objectRenderer.init();

	terrainRenderer.init();


	return GraphicsRendererBase::init();
}


void DepthNormalRenderer::recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers, double dt) {

	glm::vec3 cameraPos;
	CameraBlock cameraBlock;

	EntityManager::forEach<TransformComponent, CameraComponent>([&](auto& transform, auto& camera) {
		cameraPos = transform.position;

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

	updateDescriptorSet(0, 0, &cameraBlock);


	const auto& terrainState = GlobalStateManager::get<TerrainState>();

	uTerrainBlock.size		= terrainState.size;
	uTerrainBlock.maxHeight = terrainState.maxHeight;
	uTerrainBlock.texelSize = 1.0f / terrainState.size;

	uTerrainBlock.textureHeight = terrainState.heightMapHandle.getIndex();
	uTerrainBlock.textureNormal = terrainState.normalMapHandle.getIndex();

	updateDescriptorSet(1, 0, &uTerrainBlock);


	const uint materialDescriptorSetId = descriptorSetArrays.size() + 1;
	Frustum frustum { cameraBlock.projectionMatrix * cameraBlock.viewMatrix };

	objectRenderer.drawObjects(vkPipelineLayout, vkPipelines.data(), pSecondaryCommandBuffers, materialDescriptorSetId,
							   &frustum, 1);

	terrainRenderer.drawTerrain(vkPipelineLayout, vkPipelines.data(), pSecondaryCommandBuffers, materialDescriptorSetId,
								glm::vec2(cameraPos.x, cameraPos.z), frustum);
}
} // namespace Engine