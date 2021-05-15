#include "PostFxRenderer.hpp"

#include "engine/managers/EntityManager.hpp"

#include "engine/utils/Generator.hpp"

#include <glm/glm.hpp>


namespace Engine {
int PostFxRenderer::init() {
	spdlog::info("Initializing PostFxRenderer...");

	assert(vkDevice != vk::Device());
	assert(outputSize != vk::Extent2D());


	mesh = MeshManager::createObject<StaticMesh>("generated_screen_triangle");
	Generator::screenTriangle(mesh);

	shaderHandle = GraphicsShaderManager::getHandle<PostFxShader>(mesh);


	if (GraphicsRendererBase::init()) {
		return 1;
	}


	// descriptorSetArrays[0].updateImages(1, 0, inputVkSamplers[0], inputVkImageViews[0]);

	return 0;
}


void PostFxRenderer::recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers, uint layerIndex,
												   uint descriptorSetIndex, double dt) {

	const auto& commandBuffer = pSecondaryCommandBuffers[0];


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