#include "VolumetricLightRenderer.hpp"

#include "engine/managers/EntityManager.hpp"

#include "engine/utils/Generator.hpp"

#include <glm/glm.hpp>


namespace Engine {
int VolumetricLightRenderer::init() {
	spdlog::info("Initializing VolumetricLightRenderer...");

	assert(vkDevice != vk::Device());
	assert(outputSize != vk::Extent2D());


	mesh = MeshManager::createObject<StaticMesh>("generated_screen_triangle");
	Generator::screenTriangle(mesh);

	shaderHandle = GraphicsShaderManager::getHandle<VolumetricLightShader>(mesh);


	if (GraphicsRendererBase::init()) {
		return 1;
	}

	return 0;
}


void VolumetricLightRenderer::recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers, double dt) {

	const auto& commandBuffer = pSecondaryCommandBuffers[0];


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

		auto viewMatrix = glm::lookAtLH(cameraPos, cameraPos + glm::vec3(viewVector.x, viewVector.y, viewVector.z),
										glm::vec3(0.0f, 1.0f, 0.0f));

		auto projectionMatrix = camera.getProjectionMatrix();

		cameraBlock.invViewMatrix		= glm::inverse(viewMatrix);
		cameraBlock.invProjectionMatrix = glm::inverse(projectionMatrix);
	});

	updateDescriptorSet(0, 0, &cameraBlock);

	glm::vec3 uSunDirection;

	EntityManager::forEach<TransformComponent, LightComponent>([&](const auto& transform, const auto& light) {
		if (light.type == LightComponent::Type::DIRECTIONAL) {
			auto lightDirection = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
			lightDirection		= glm::rotate(transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)) * lightDirection;
			lightDirection		= glm::rotate(transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)) * lightDirection;
			lightDirection		= glm::rotate(transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f)) * lightDirection;

			uSunDirection = -lightDirection;

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

	updateDescriptorSet(1, 0, uDirectionalLightMatrices.data());
	updateDescriptorSet(1, 1, &uSunDirection);


	const auto& meshInfo = MeshManager::getMeshInfo(mesh);

	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vkPipelines[shaderHandle.getIndex()]);


	auto vertexBuffer		 = meshInfo.vertexBuffer.getVkBuffer();
	vk::DeviceSize offsets[] = { 0 };
	commandBuffer.bindVertexBuffers(0, 1, &vertexBuffer, offsets);

	auto indexBuffer = meshInfo.indexBuffer.getVkBuffer();
	commandBuffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);

	commandBuffer.drawIndexed(meshInfo.indexCount, 1, 0, 0, 0);
}
} // namespace Engine