#include "SkyboxRenderer.hpp"

#include "engine/managers/EntityManager.hpp"

#include <glm/glm.hpp>


namespace Engine {
int SkyboxRenderer::init() {
	spdlog::info("Initializing SkyboxRenderer...");

	assert(vkDevice != vk::Device());
	assert(outputSize != vk::Extent2D());


	// TODO: move to Engine::Generator
	// Generate skybox mesh

	boxMesh = MeshManager::createObject(0, "generated_box");
	boxMesh.apply([](auto& mesh) {
		auto& vertexBuffer = mesh.getVertexBuffer();
		vertexBuffer.resize(8);

		std::get<0>(vertexBuffer[0]) = glm::vec3(-1.0f, -1.0f, -1.0f);
		std::get<0>(vertexBuffer[1]) = glm::vec3(-1.0f, -1.0f, 1.0f);
		std::get<0>(vertexBuffer[2]) = glm::vec3(1.0f, -1.0f, 1.0f);
		std::get<0>(vertexBuffer[3]) = glm::vec3(1.0f, -1.0f, -1.0f);

		std::get<0>(vertexBuffer[4]) = glm::vec3(-1.0f, 1.0f, -1.0f);
		std::get<0>(vertexBuffer[5]) = glm::vec3(-1.0f, 1.0f, 1.0f);
		std::get<0>(vertexBuffer[6]) = glm::vec3(1.0f, 1.0f, 1.0f);
		std::get<0>(vertexBuffer[7]) = glm::vec3(1.0f, 1.0f, -1.0f);

		auto& indexBuffer = mesh.getIndexBuffer();

		indexBuffer = {
			0, 2, 1, 0, 3, 2, 5, 6, 7, 5, 7, 4, 0, 4, 7, 0, 7, 3, 3, 7, 6, 3, 6, 2, 2, 6, 5, 2, 5, 1, 1, 5, 4, 1, 4, 0,
		};
	});
	boxMesh.update();

	shaderHandle = GraphicsShaderManager::getHandle<SkyboxShader>(boxMesh);


	if (GraphicsRendererBase::init()) {
		return 1;
	}

	descriptorSetArrays[0].updateImage(0, 1, 0, inputVkSamplers[0], inputVkImageViews[0]);

	return 0;
}


void SkyboxRenderer::recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers, uint layerIndex,
												   double dt) {
	const auto& commandBuffer = pSecondaryCommandBuffers[0];

	bindDescriptorSets(commandBuffer, vk::PipelineBindPoint::eGraphics);


	struct CameraBlock {
		glm::mat4 viewProjectionMatrix;
	} cameraBlock;


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

	descriptorSetArrays[0].updateBuffer(0, 0, &cameraBlock, sizeof(cameraBlock));


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