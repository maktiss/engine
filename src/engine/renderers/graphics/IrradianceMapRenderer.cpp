#include "IrradianceMapRenderer.hpp"

#include "engine/managers/EntityManager.hpp"

#include "engine/utils/Generator.hpp"

#include <glm/glm.hpp>


namespace Engine {
int IrradianceMapRenderer::init() {
	spdlog::info("Initializing IrradianceMapRenderer...");

	assert(vkDevice != vk::Device());
	assert(outputSize != vk::Extent2D());


	boxMesh = MeshManager::createObject<StaticMesh>("generated_sky_box");
	Generator::skyBoxMesh(boxMesh);

	shaderHandle = GraphicsShaderManager::getHandle<IrradianceShader>(boxMesh);


	if (GraphicsRendererBase::init()) {
		return 1;
	}


	CameraBlock cameraBlock;
	Generator::cubeViewMatrices(cameraBlock.viewProjectionMatrices, 0.1f, 1000.0f);

	std::vector<glm::vec4> samples(irradianceMapSampleCount);
	Generator::fibonacciSphere(samples.data(), samples.size(), 2);


	descriptorSetArrays[0].updateBuffers(0, &cameraBlock, sizeof(cameraBlock));
	descriptorSetArrays[1].updateBuffers(0, samples.data(), sizeof(glm::vec4) * irradianceMapSampleCount);

	return 0;
}


void IrradianceMapRenderer::recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers,
														  double dt) {

	const auto& commandBuffer = pSecondaryCommandBuffers[0];


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