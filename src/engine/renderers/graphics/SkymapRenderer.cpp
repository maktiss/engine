#include "SkymapRenderer.hpp"

#include "engine/managers/EntityManager.hpp"

#include "engine/utils/Generator.hpp"

#include <glm/glm.hpp>


namespace Engine {
int SkymapRenderer::init() {
	spdlog::info("Initializing SkymapRenderer...");

	assert(vkDevice != vk::Device());
	assert(outputSize != vk::Extent2D());


	// Create sky sphere mesh

	const uint verticalVertexCount	 = 15;
	const uint horisontalVertexCount = 25;

	skySphereMesh = MeshManager::createObject<StaticMesh>("generated_sky_sphere");
	Generator::skySphereMesh(skySphereMesh, verticalVertexCount, horisontalVertexCount);

	shaderHandle = GraphicsShaderManager::getHandle<SkymapShader>(skySphereMesh);


	if (GraphicsRendererBase::init()) {
		return 1;
	}


	CameraBlock cameraBlock;
	Generator::cubeViewMatrices(cameraBlock.viewProjectionMatrices, 0.1f, 1000.0f);

	descriptorSetArrays[0].updateBuffers(0, &cameraBlock, sizeof(cameraBlock));

	return 0;
}


void SkymapRenderer::recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers, double dt) {

	const auto& commandBuffer = pSecondaryCommandBuffers[0];


	auto sunDirection = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);

	EntityManager::forEach<TransformComponent, LightComponent>([&](const auto& transform, auto& light) {
		if (light.castsShadows) {
			if (light.type == LightComponent::Type::DIRECTIONAL) {
				sunDirection = glm::rotate(transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)) * sunDirection;
				sunDirection = glm::rotate(transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)) * sunDirection;
				sunDirection = glm::rotate(transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f)) * sunDirection;
			}
		}
	});

	updateDescriptorSet(1, 0, &sunDirection);


	const auto& meshInfo = MeshManager::getMeshInfo(skySphereMesh);

	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vkPipelines[shaderHandle.getIndex()]);


	auto vertexBuffer		 = meshInfo.vertexBuffer.getVkBuffer();
	vk::DeviceSize offsets[] = { 0 };
	commandBuffer.bindVertexBuffers(0, 1, &vertexBuffer, offsets);

	auto indexBuffer = meshInfo.indexBuffer.getVkBuffer();
	commandBuffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);

	commandBuffer.drawIndexed(meshInfo.indexCount, 1, 0, 0, 0);
}
} // namespace Engine