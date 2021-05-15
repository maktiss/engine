#include "ShadowMapRenderer.hpp"

#include "engine/managers/EntityManager.hpp"

#include <glm/glm.hpp>


namespace Engine {
int ShadowMapRenderer::init() {
	spdlog::info("Initializing ShadowMapRenderer...");

	assert(vkDevice != vk::Device());
	assert(outputSize != vk::Extent2D());

	excludeFrustums.resize(getLayerCount() - 1);


	return ObjectRendererBase::init();
}


void ShadowMapRenderer::recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers,
													  uint layerIndex, double dt) {
	// TODO: move to function
	for (uint threadIndex = 0; threadIndex < threadCount; threadIndex++) {
		const auto& commandBuffer = pSecondaryCommandBuffers[threadIndex];

		bindDescriptorSets(commandBuffer, vk::PipelineBindPoint::eGraphics);
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


	uint excludeFrustumCount = 0;

	EntityManager::forEach<TransformComponent, LightComponent>([&](const auto& transform, auto& light) {
		if (light.castsShadows) {
			if (light.type == LightComponent::Type::DIRECTIONAL) {
				auto lightDirection = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
				lightDirection		= glm::rotate(transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)) * lightDirection;
				lightDirection		= glm::rotate(transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)) * lightDirection;
				lightDirection		= glm::rotate(transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f)) * lightDirection;


				excludeFrustumCount = 0;

				for (uint cascade = 0; cascade < getLayerCount(); cascade++) {
					float cascadeHalfSize = directionalLightCascadeBase * std::pow(2, cascade * 2 + 1);

					glm::vec3 position = cameraPos + cascadeHalfSize * directionalLightCascadeOffset * cameraViewDir;

					auto viewMatrix =
						glm::lookAtLH(position, position + glm::vec3(lightDirection), glm::vec3(0.0f, 1.0f, 0.0f));

					auto projectionMatrix = glm::orthoLH_ZO(-cascadeHalfSize, cascadeHalfSize, -cascadeHalfSize,
															cascadeHalfSize, -cascadeHalfSize * 4.0f, cascadeHalfSize);

					if (cascade == layerIndex) {
						cameraBlock.viewMatrix		 = viewMatrix;
						cameraBlock.projectionMatrix = projectionMatrix;
						break;

					} else {
						excludeFrustums[excludeFrustumCount] = projectionMatrix * viewMatrix;
						excludeFrustumCount++;
					}
				}

				light.shadowMapIndex = 0;
			}
		}
	});

	cameraBlock.invViewMatrix		= glm::inverse(cameraBlock.viewMatrix);
	cameraBlock.invProjectionMatrix = glm::inverse(cameraBlock.projectionMatrix);

	descriptorSetArrays[1].updateBuffer(layerIndex, 0, &cameraBlock, sizeof(cameraBlock));


	Frustum frustum { cameraBlock.projectionMatrix * cameraBlock.viewMatrix };
	drawObjects(pSecondaryCommandBuffers, &frustum, 1, excludeFrustums.data(), excludeFrustumCount);
}
} // namespace Engine