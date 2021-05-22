#include "ObjectRenderer.hpp"

#include "engine/managers/EntityManager.hpp"
#include "engine/managers/MaterialManager.hpp"
#include "engine/managers/MeshManager.hpp"


namespace Engine {
int ObjectRenderer::init() {
	assert(threadCount > 0);

	renderInfoCachePerThread.resize(threadCount);
	renderInfoIndicesPerThread.resize(threadCount);

	threadPool.init(drawObjectsThreadFunc, threadCount);

	drawObjectsThreadInfos.resize(threadCount * 2);

	return 0;
}


void ObjectRenderer::drawObjects(const vk::PipelineLayout pipelineLayout, const vk::Pipeline* pPipelines,
								 const vk::CommandBuffer* pSecondaryCommandBuffers, const uint materialDescriptorSetId,
								 const Frustum* includeFrustums, uint includeFrustumCount,
								 const Frustum* excludeFrustums, uint excludeFrustumCount) {

	for (uint i = 0; i < drawObjectsThreadInfos.size(); i++) {
		drawObjectsThreadInfos[i].renderer = this;

		drawObjectsThreadInfos[i].vkPipelineLayout		   = pipelineLayout;
		drawObjectsThreadInfos[i].pVkPipelines			   = pPipelines;
		drawObjectsThreadInfos[i].pSecondaryCommandBuffers = pSecondaryCommandBuffers;

		drawObjectsThreadInfos[i].includeFrustums	  = includeFrustums;
		drawObjectsThreadInfos[i].includeFrustumCount = includeFrustumCount;
		drawObjectsThreadInfos[i].excludeFrustums	  = excludeFrustums;
		drawObjectsThreadInfos[i].excludeFrustumCount = excludeFrustumCount;

		drawObjectsThreadInfos[i].fragmentIndex = i;
		drawObjectsThreadInfos[i].fragmentCount = drawObjectsThreadInfos.size();

		drawObjectsThreadInfos[i].materialDescriptorSetIndex = materialDescriptorSetId;

		threadPool.appendData(&drawObjectsThreadInfos[i]);
	}

	threadPool.waitForAll();
}


void ObjectRenderer::drawObjectsThreadFunc(uint threadIndex, void* pData) {
	DrawObjectsThreadInfo drawObjectsThreadInfo = *static_cast<DrawObjectsThreadInfo*>(pData);

	const auto& vkPipelineLayout = drawObjectsThreadInfo.vkPipelineLayout;
	const auto& pVkPipelines	 = drawObjectsThreadInfo.pVkPipelines;

	const auto& commandBuffer = drawObjectsThreadInfo.pSecondaryCommandBuffers[threadIndex];

	const auto& includeFrustums		= drawObjectsThreadInfo.includeFrustums;
	const auto& includeFrustumCount = drawObjectsThreadInfo.includeFrustumCount;
	const auto& excludeFrustums		= drawObjectsThreadInfo.excludeFrustums;
	const auto& excludeFrustumCount = drawObjectsThreadInfo.excludeFrustumCount;

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

			auto boundingSphere = meshInfo.boundingSphere.transform(transformMatrix);
			auto boundingBox	= meshInfo.boundingBox.transform(transformMatrix);

			bool shouldDraw = includeFrustumCount == 0;

			for (uint i = 0; i < includeFrustumCount; i++) {
				const auto& includeFrustum = includeFrustums[i];

				shouldDraw = shouldDraw || includeFrustum.intersects(boundingBox);
			}

			for (uint i = 0; i < excludeFrustumCount; i++) {
				const auto& excludeFrustum = excludeFrustums[i];

				shouldDraw = shouldDraw && !excludeFrustum.contains(boundingBox);
			}

			if (shouldDraw) {
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


	const auto& materialDescriptorSetIndex = drawObjectsThreadInfo.materialDescriptorSetIndex;

	for (const auto& [key, renderInfoIndex] : renderInfoIndices) {
		const auto& renderInfo = renderInfoCache[renderInfoIndex];

		if (lastPipelineIndex != renderInfo.pipelineIndex) {
			lastPipelineIndex = renderInfo.pipelineIndex;
			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pVkPipelines[renderInfo.pipelineIndex]);
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

			commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, vkPipelineLayout,
											 materialDescriptorSetIndex, 1, &renderInfo.material, 0, nullptr);
		}


		commandBuffer.drawIndexed(renderInfo.indexCount, 1, 0, 0, 0);
	}
}
} // namespace Engine