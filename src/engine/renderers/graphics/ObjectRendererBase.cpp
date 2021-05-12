#include "ObjectRendererBase.hpp"


namespace Engine {
int ObjectRendererBase::init() {
	renderInfoCachePerThread.resize(threadCount);
	renderInfoIndicesPerThread.resize(threadCount);

	threadPool.init(drawObjectsThreadFunc, threadCount);

	drawObjectsThreadInfos.resize(threadCount * 2);

	return GraphicsRendererBase::init();
}


void ObjectRendererBase::drawObjects(const vk::CommandBuffer* pSecondaryCommandBuffers, const Frustum& frustum) {
	for (uint i = 0; i < drawObjectsThreadInfos.size(); i++) {
		drawObjectsThreadInfos[i].renderer				   = this;
		drawObjectsThreadInfos[i].pSecondaryCommandBuffers = pSecondaryCommandBuffers;
		drawObjectsThreadInfos[i].frustum				   = frustum;
		drawObjectsThreadInfos[i].fragmentIndex			   = i;
		drawObjectsThreadInfos[i].fragmentCount			   = drawObjectsThreadInfos.size();

		threadPool.appendData(&drawObjectsThreadInfos[i]);
	}

	// drawObjectsThreadFunc(0, &drawObjectsThreadInfo);

	threadPool.waitForAll();
}


void ObjectRendererBase::drawObjectsThreadFunc(uint threadIndex, void* pData) {
	DrawObjectsThreadInfo drawObjectsThreadInfo = *static_cast<DrawObjectsThreadInfo*>(pData);

	const auto& commandBuffer = drawObjectsThreadInfo.pSecondaryCommandBuffers[threadIndex];

	const auto& frustum		= drawObjectsThreadInfo.frustum;
	auto& renderInfoIndices = drawObjectsThreadInfo.renderer->renderInfoIndicesPerThread[threadIndex];
	auto& renderInfoCache	= drawObjectsThreadInfo.renderer->renderInfoCachePerThread[threadIndex];

	renderInfoIndices.clear();
	renderInfoCache.clear();

	uint entityIndex = 0;

	EntityManager::forEach<TransformComponent, ModelComponent>(
		[&](auto& transform, auto& model) {
			const auto transformMatrix = transform.getTransformMatrix();

			const auto& meshHandle	   = model.meshHandles[0];
			const auto& materialHandle = model.materialHandles[0];
			const auto& shaderHandle   = model.shaderHandles[0];

			const auto& meshInfo = MeshManager::getMeshInfo(meshHandle);

			if (frustum.contains(meshInfo.boundingBox.transform(transformMatrix))) {
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
					transformMatrix,
				});

				entityIndex++;
			}
		},
		drawObjectsThreadInfo.fragmentIndex, drawObjectsThreadInfo.fragmentCount);


	auto lastPipelineIndex = -1;
	vk::Buffer lastVertexBuffer {};
	vk::DescriptorSet lastMaterial {};

	for (const auto& [key, renderInfoIndex] : renderInfoIndices) {
		const auto& renderInfo = renderInfoCache[renderInfoIndex];

		if (lastPipelineIndex != renderInfo.pipelineIndex) {
			lastPipelineIndex = renderInfo.pipelineIndex;
			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
									   drawObjectsThreadInfo.renderer->vkPipelines[renderInfo.pipelineIndex]);
		}

		commandBuffer.pushConstants(drawObjectsThreadInfo.renderer->vkPipelineLayout, vk::ShaderStageFlagBits::eAll, 0,
									64, &renderInfo.transformMatrix);


		if (lastVertexBuffer != renderInfo.vertexBuffer) {
			lastVertexBuffer = renderInfo.vertexBuffer;

			vk::DeviceSize offsets[] = { 0 };
			commandBuffer.bindVertexBuffers(0, 1, &renderInfo.vertexBuffer, offsets);
			commandBuffer.bindIndexBuffer(renderInfo.indexBuffer, 0, vk::IndexType::eUint32);
		}


		if (lastMaterial != renderInfo.material) {
			lastMaterial = renderInfo.material;

			commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
											 drawObjectsThreadInfo.renderer->vkPipelineLayout, 3, 1,
											 &renderInfo.material, 0, nullptr);
		}


		commandBuffer.drawIndexed(renderInfo.indexCount, 1, 0, 0, 0);
	}
}
} // namespace Engine