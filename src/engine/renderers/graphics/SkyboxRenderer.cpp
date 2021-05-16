#include "SkyboxRenderer.hpp"

#include "engine/managers/EntityManager.hpp"

#include "engine/utils/Generator.hpp"

#include <glm/glm.hpp>


namespace Engine {
int SkyboxRenderer::init() {
	spdlog::info("Initializing SkyboxRenderer...");

	assert(vkDevice != vk::Device());
	assert(outputSize != vk::Extent2D());


	boxMesh = MeshManager::createObject<StaticMesh>("generated_sky_box");
	Generator::skyBoxMesh(boxMesh);

	shaderHandle = GraphicsShaderManager::getHandle<SkyboxShader>(boxMesh);


	if (GraphicsRendererBase::init()) {
		return 1;
	}

	return 0;
}


void SkyboxRenderer::recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers, double dt) {

	const auto& commandBuffer = pSecondaryCommandBuffers[0];


	CameraBlock cameraBlock;


	EntityManager::forEach<TransformComponent, CameraComponent>([&](auto& transform, auto& camera) {
		// TODO: Check if active camera
		glm::vec4 viewVector = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
		viewVector			 = glm::rotate(transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)) * viewVector;
		viewVector			 = glm::rotate(transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)) * viewVector;
		viewVector			 = glm::rotate(transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f)) * viewVector;

		auto viewMatrix =
			glm::lookAtLH(transform.position, transform.position + glm::vec3(viewVector.x, viewVector.y, viewVector.z),
						  glm::vec3(0.0f, 1.0f, 0.0f));

		cameraBlock.viewProjectionMatrix = camera.getProjectionMatrix() * viewMatrix;
	});

	updateDescriptorSet(0, 0, &cameraBlock);


	const auto& meshInfo = MeshManager::getMeshInfo(boxMesh);

	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vkPipelines[shaderHandle.getIndex()]);


	auto vertexBuffer		 = meshInfo.vertexBuffer.getVkBuffer();
	vk::DeviceSize offsets[] = { 0 };
	commandBuffer.bindVertexBuffers(0, 1, &vertexBuffer, offsets);

	auto indexBuffer = meshInfo.indexBuffer.getVkBuffer();
	commandBuffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);

	commandBuffer.drawIndexed(meshInfo.indexCount, 1, 0, 0, 0);
}
} // namespace Engine