#include "IrradianceMapRenderer.hpp"

#include "engine/managers/EntityManager.hpp"

#include "engine/utils/Generator.hpp"

#include <glm/glm.hpp>


namespace Engine {
int IrradianceMapRenderer::init() {
	spdlog::info("Initializing IrradianceMapRenderer...");

	assert(vkDevice != vk::Device());
	assert(outputSize != vk::Extent2D());


	// Update camera block

	CameraBlock cameraBlock;

	glm::vec3 viewVectors[6] = {
		{ 1.0f, 0.0f, 0.0f },  { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f },
		{ 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f },  { 0.0f, 0.0f, -1.0f },
	};

	auto projectionMatrix = glm::perspectiveLH(glm::radians(90.0f), 1.0f, 0.1f, 1000.0f);

	for (uint i = 0; i < 6; i++) {
		auto up = glm::vec3(0.0f, 1.0f, 0.0f);
		if (i == 2) {
			up = glm::vec3(0.0f, 0.0f, -1.0f);
		} else if (i == 3) {
			up = glm::vec3(0.0f, 0.0f, 1.0f);
		}

		auto viewMatrix = glm::lookAtLH(glm::vec3(0.0f), viewVectors[i], up);

		cameraBlock.viewProjectionMatrices[i] = projectionMatrix * viewMatrix;
	}


	// Update samples block

	std::vector<glm::vec4> samples(irradianceMapSampleCount);
	Generator::fibonacciSphere(samples.data(), samples.size(), 2);


	// TODO: move to Engine::Generator
	// Generate box mesh

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

	shaderHandle = GraphicsShaderManager::getHandle<IrradianceShader>(boxMesh);


	if (GraphicsRendererBase::init()) {
		return 1;
	}

	descriptorSetArrays[0].updateImage(0, 0, 0, inputVkSamplers[0], inputVkImageViews[0]);
	descriptorSetArrays[1].updateBuffer(0, 0, &cameraBlock, sizeof(cameraBlock));
	descriptorSetArrays[2].updateBuffer(0, 0, samples.data(), sizeof(glm::vec4) * irradianceMapSampleCount);

	return 0;
}


void IrradianceMapRenderer::recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers,
														  uint layerIndex, double dt) {
	const auto& commandBuffer = pSecondaryCommandBuffers[0];

	bindDescriptorSets(commandBuffer, vk::PipelineBindPoint::eGraphics);


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