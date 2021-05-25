#include "ForwardRenderer.hpp"

#include "engine/managers/EntityManager.hpp"

#include <glm/glm.hpp>


namespace Engine {
int ForwardRenderer::init() {
	spdlog::info("Initializing ForwardRenderer...");

	assert(vkDevice != vk::Device());
	assert(outputSize != vk::Extent2D());

	objectRenderer.setThreadCount(threadCount);
	objectRenderer.init();

	terrainRenderer.init();


	if (GraphicsRendererBase::init()) {
		return 1;
	}

	return 0;
}


void ForwardRenderer::recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers, double dt) {

	glm::vec3 cameraPos;
	glm::vec3 cameraViewDir;

	EntityManager::forEach<TransformComponent, CameraComponent>([&](auto& transform, auto& camera) {
		// TODO: Check if active camera
		glm::vec4 viewVector = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
		viewVector			 = glm::rotate(transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)) * viewVector;
		viewVector			 = glm::rotate(transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)) * viewVector;
		viewVector			 = glm::rotate(transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f)) * viewVector;

		cameraPos	  = transform.position;
		cameraViewDir = glm::vec3(viewVector);

		uCameraBlock.viewMatrix =
			glm::lookAtLH(transform.position, transform.position + glm::vec3(viewVector.x, viewVector.y, viewVector.z),
						  glm::vec3(0.0f, 1.0f, 0.0f));

		uCameraBlock.projectionMatrix = camera.getProjectionMatrix();
	});

	uCameraBlock.invViewMatrix		 = glm::inverse(uCameraBlock.viewMatrix);
	uCameraBlock.invProjectionMatrix = glm::inverse(uCameraBlock.projectionMatrix);

	updateDescriptorSet(0, 0, &uCameraBlock);


	uDirectionalLightBlock.enabled = false;

	EntityManager::forEach<TransformComponent, LightComponent>([&](const auto& transform, const auto& light) {
		if (light.type == LightComponent::Type::DIRECTIONAL) {
			auto lightDirection = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
			lightDirection		= glm::rotate(transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)) * lightDirection;
			lightDirection		= glm::rotate(transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)) * lightDirection;
			lightDirection		= glm::rotate(transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f)) * lightDirection;

			uDirectionalLightBlock.enabled		  = true;
			uDirectionalLightBlock.direction	  = glm::vec3(uCameraBlock.viewMatrix * lightDirection);
			uDirectionalLightBlock.color		  = light.color;
			uDirectionalLightBlock.shadowMapIndex = light.shadowMapIndex;


			for (uint cascadeIndex = 0; cascadeIndex < directionalLightCascadeCount; cascadeIndex++) {
				float cascadeHalfSize = directionalLightCascadeBase * std::pow(2, cascadeIndex * 2 + 1);

				glm::vec3 position = cameraPos + cascadeHalfSize * directionalLightCascadeOffset * cameraViewDir;

				auto lightSpaceMatrix =
					glm::lookAtLH(position, position + glm::vec3(lightDirection), glm::vec3(0.0f, 1.0f, 0.0f));

				lightSpaceMatrix = glm::orthoLH_ZO(-cascadeHalfSize, cascadeHalfSize, -cascadeHalfSize, cascadeHalfSize,
												   -cascadeHalfSize * 4.0f, cascadeHalfSize) *
								   lightSpaceMatrix;

				uDirectionalLightMatrices[cascadeIndex] = lightSpaceMatrix;
			}
		}
	});

	updateDescriptorSet(1, 0, &uDirectionalLightBlock);
	updateDescriptorSet(1, 1, uDirectionalLightMatrices.data());


	const auto& terrainState = GlobalStateManager::get<TerrainState>();

	uTerrainBlock.size		= terrainState.size;
	uTerrainBlock.maxHeight = terrainState.maxHeight;

	uTerrainBlock.textureHeight = terrainState.heightMapHandle.getIndex();
	uTerrainBlock.textureNormal = terrainState.normalMapHandle.getIndex();

	uTerrainBlock.texelSize		= 1.0f / terrainState.size;
	uTerrainBlock.baseTessLevel = 16.0f;

	updateDescriptorSet(2, 0, &uTerrainBlock);


	const uint materialDescriptorSetId = descriptorSetArrays.size() + 1;
	Frustum frustum { uCameraBlock.projectionMatrix * uCameraBlock.viewMatrix };

	objectRenderer.drawObjects(vkPipelineLayout, vkPipelines.data(), pSecondaryCommandBuffers, materialDescriptorSetId,
							   &frustum, 1);

	terrainRenderer.drawTerrain(vkPipelineLayout, vkPipelines.data(), pSecondaryCommandBuffers, materialDescriptorSetId,
								glm::vec2(cameraPos.x, cameraPos.z), frustum);
}
} // namespace Engine