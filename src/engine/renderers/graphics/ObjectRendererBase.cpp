#include "ObjectRendererBase.hpp"


namespace Engine {
void ObjectRendererBase::drawObjects(const vk::CommandBuffer* pSecondaryCommandBuffers) {
	// TODO: multithreading
	// TODO: frustum culling

	const auto& commandBuffer = pSecondaryCommandBuffers[0];

	uint entityIndex = 0;
	renderInfoCache.clear();
	renderInfoIndices.clear();

	EntityManager::forEach<TransformComponent, ModelComponent>([&](auto& transform, auto& model) {
		const auto& meshHandle	   = model.meshHandles[0];
		const auto& materialHandle = model.materialHandles[0];
		const auto& shaderHandle   = model.shaderHandles[0];

		const auto& meshInfo	 = MeshManager::getMeshInfo(meshHandle);
		const auto& materialInfo = MaterialManager::getMaterialInfo(materialHandle);

		uint64_t pipelineIndex = shaderHandle.getIndex();
		uint64_t materialIndex = materialHandle.getIndex();
		uint64_t meshIndex	   = meshHandle.getIndex();

		renderInfoIndices[(pipelineIndex << 54) + (materialIndex << 44) + (meshIndex << 34) + entityIndex] =
			renderInfoCache.size();

		renderInfoCache.push_back({
			shaderHandle.getIndex(),
			materialInfo.descriptorSet,
			meshInfo.vertexBuffer.getVkBuffer(),
			meshInfo.indexBuffer.getVkBuffer(),
			meshInfo.indexCount,
			transform.getTransformMatrix(),
		});

		entityIndex++;
	});


	auto lastPipelineIndex = -1;
	vk::Buffer lastVertexBuffer {};
	vk::DescriptorSet lastMaterial {};

	for (const auto& [key, renderInfoIndex] : renderInfoIndices) {
		const auto& renderInfo = renderInfoCache[renderInfoIndex];

		if (lastPipelineIndex != renderInfo.pipelineIndex) {
			lastPipelineIndex = renderInfo.pipelineIndex;
			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vkPipelines[renderInfo.pipelineIndex]);
		}

		commandBuffer.pushConstants(vkPipelineLayout, vk::ShaderStageFlagBits::eAll, 0, 64,
									&renderInfo.transformMatrix);


		if (lastVertexBuffer != renderInfo.vertexBuffer) {
			lastVertexBuffer = renderInfo.vertexBuffer;

			vk::DeviceSize offsets[] = { 0 };
			commandBuffer.bindVertexBuffers(0, 1, &renderInfo.vertexBuffer, offsets);
			commandBuffer.bindIndexBuffer(renderInfo.indexBuffer, 0, vk::IndexType::eUint32);
		}


		if (lastMaterial != renderInfo.material) {
			lastMaterial = renderInfo.material;

			commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, vkPipelineLayout, 3, 1,
											 &renderInfo.material, 0, nullptr);
		}


		commandBuffer.drawIndexed(renderInfo.indexCount, 1, 0, 0, 0);
	}
}
} // namespace Engine