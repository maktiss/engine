#include "ForwardRenderer.hpp"

#include "engine/managers/EntityManager.hpp"

#include <glm/glm.hpp>


namespace Engine {
int ForwardRenderer::init() {
	spdlog::info("Initializing ForwardRenderer...");

	assert(vkDevice != vk::Device());
	assert(outputSize != vk::Extent2D());


	if (ObjectRendererBase::init()) {
		return 1;
	}

	return 0;
}


void ForwardRenderer::recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers, uint layerIndex,
													uint descriptorSetIndex, double dt) {

	CameraBlock cameraBlock;

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

		cameraBlock.viewMatrix =
			glm::lookAtLH(transform.position, transform.position + glm::vec3(viewVector.x, viewVector.y, viewVector.z),
						  glm::vec3(0.0f, 1.0f, 0.0f));

		cameraBlock.projectionMatrix = camera.getProjectionMatrix();
	});

	cameraBlock.invViewMatrix		= glm::inverse(cameraBlock.viewMatrix);
	cameraBlock.invProjectionMatrix = glm::inverse(cameraBlock.projectionMatrix);

	descriptorSetArrays[0].updateBuffer(descriptorSetIndex, 0, &cameraBlock, sizeof(cameraBlock));


	void* pEnvironmentBlock;
	descriptorSetArrays[1].mapBuffer(descriptorSetIndex, 0, pEnvironmentBlock);

	EnvironmentBlockMap environmentBlockMap;

	// TODO: better/safer way of block mapping
	environmentBlockMap.useDirectionalLight = static_cast<bool*>(pEnvironmentBlock);

	environmentBlockMap.directionalLight.direction =
		reinterpret_cast<glm::vec3*>(reinterpret_cast<uint8_t*>(environmentBlockMap.useDirectionalLight) + 16);
	environmentBlockMap.directionalLight.color =
		reinterpret_cast<glm::vec3*>(reinterpret_cast<uint8_t*>(environmentBlockMap.directionalLight.direction) + 16);
	environmentBlockMap.directionalLight.shadowMapIndex =
		reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(environmentBlockMap.directionalLight.color) + 12);
	environmentBlockMap.directionalLight.baseLightSpaceMatrix = reinterpret_cast<glm::mat4*>(
		reinterpret_cast<uint8_t*>(environmentBlockMap.directionalLight.shadowMapIndex) + 4);
	environmentBlockMap.directionalLight.lightSpaceMatrices = reinterpret_cast<glm::mat4*>(
		reinterpret_cast<uint8_t*>(environmentBlockMap.directionalLight.baseLightSpaceMatrix) + 64);

	environmentBlockMap.pointLightClusters = reinterpret_cast<EnvironmentBlockMap::LightCluster*>(
		reinterpret_cast<uint8_t*>(environmentBlockMap.directionalLight.lightSpaceMatrices) +
		64 * directionalLightCascadeCount);
	environmentBlockMap.spotLightClusters = reinterpret_cast<EnvironmentBlockMap::LightCluster*>(
		reinterpret_cast<uint8_t*>(environmentBlockMap.pointLightClusters) +
		64 * clusterCountX * clusterCountY * clusterCountZ);


	*environmentBlockMap.useDirectionalLight = false;


	EntityManager::forEach<TransformComponent, LightComponent>([&](const auto& transform, const auto& light) {
		if (light.type == LightComponent::Type::DIRECTIONAL) {
			auto lightDirection = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
			lightDirection		= glm::rotate(transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)) * lightDirection;
			lightDirection		= glm::rotate(transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)) * lightDirection;
			lightDirection		= glm::rotate(transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f)) * lightDirection;

			*environmentBlockMap.useDirectionalLight			 = true;
			*environmentBlockMap.directionalLight.direction		 = glm::vec3(cameraBlock.viewMatrix * lightDirection);
			*environmentBlockMap.directionalLight.color			 = light.color;
			*environmentBlockMap.directionalLight.shadowMapIndex = light.shadowMapIndex;


			float cascadeHalfSize = directionalLightCascadeBase;

			auto lightSpaceMatrix =
				glm::lookAtLH(cameraPos, cameraPos + glm::vec3(lightDirection), glm::vec3(0.0f, 1.0f, 0.0f));

			lightSpaceMatrix = glm::orthoLH_ZO(-cascadeHalfSize, cascadeHalfSize, -cascadeHalfSize, cascadeHalfSize,
											   -cascadeHalfSize * 4.0f, cascadeHalfSize) *
							   lightSpaceMatrix;

			*environmentBlockMap.directionalLight.baseLightSpaceMatrix = lightSpaceMatrix;

			for (uint cascadeIndex = 0; cascadeIndex < directionalLightCascadeCount; cascadeIndex++) {
				float cascadeHalfSize = directionalLightCascadeBase * std::pow(2, cascadeIndex * 2 + 1);

				glm::vec3 position = cameraPos + cascadeHalfSize * directionalLightCascadeOffset * cameraViewDir;

				auto lightSpaceMatrix =
					glm::lookAtLH(position, position + glm::vec3(lightDirection), glm::vec3(0.0f, 1.0f, 0.0f));

				lightSpaceMatrix = glm::orthoLH_ZO(-cascadeHalfSize, cascadeHalfSize, -cascadeHalfSize, cascadeHalfSize,
												   -cascadeHalfSize * 4.0f, cascadeHalfSize) *
								   lightSpaceMatrix;

				environmentBlockMap.directionalLight.lightSpaceMatrices[cascadeIndex] = lightSpaceMatrix;
			}
		}
	});


	descriptorSetArrays[1].unmapBuffer(descriptorSetIndex, 0);

	Frustum frustum { cameraBlock.projectionMatrix * cameraBlock.viewMatrix };
	drawObjects(pSecondaryCommandBuffers, &frustum, 1);
}
} // namespace Engine