#include "ReflectionRenderer.hpp"

#include "engine/managers/EntityManager.hpp"

#include "engine/utils/Generator.hpp"

#include <glm/glm.hpp>


namespace Engine {
int ReflectionRenderer::init() {
	spdlog::info("Initializing ReflectionRenderer...");

	assert(vkDevice != vk::Device());
	assert(outputSize != vk::Extent2D());


	mesh = MeshManager::createObject<StaticMesh>("generated_screen_triangle");
	Generator::screenTriangle(mesh);

	shaderHandle = GraphicsShaderManager::getHandle<ReflectionShader>(mesh);


	if (GraphicsRendererBase::init()) {
		return 1;
	}

	return 0;
}


void ReflectionRenderer::recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers,
													   uint layerIndex, uint descriptorSetIndex, double dt) {

	const auto& commandBuffer = pSecondaryCommandBuffers[0];


	CameraBlock cameraBlock;

	EntityManager::forEach<TransformComponent, CameraComponent>([&](auto& transform, auto& camera) {
		// TODO: Check if active camera
		glm::vec4 viewVector = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
		viewVector			 = glm::rotate(transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)) * viewVector;
		viewVector			 = glm::rotate(transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)) * viewVector;
		viewVector			 = glm::rotate(transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f)) * viewVector;

		auto viewMatrix = glm::lookAtLH(glm::vec3(0.0f), glm::vec3(viewVector.x, viewVector.y, viewVector.z),
										glm::vec3(0.0f, 1.0f, 0.0f));

		auto projectionMatrix = camera.getProjectionMatrix();

		cameraBlock.invViewMatrix		= glm::inverse(viewMatrix);
		cameraBlock.invProjectionMatrix = glm::inverse(projectionMatrix);
	});


	descriptorSetArrays[0].updateBuffer(descriptorSetIndex, 0, &cameraBlock, sizeof(cameraBlock));


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