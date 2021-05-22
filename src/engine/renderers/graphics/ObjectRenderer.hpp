#pragma once

#include "engine/graphics/Frustum.hpp"

#include "engine/utils/ThreadPool.hpp"

#define VULKAN_HPP_NO_EXCEPTIONS 1
#include <vulkan/vulkan.hpp>

#include <map>
#include <vector>


namespace Engine {
class ObjectRenderer {
private:
	struct RenderInfo {
		uint pipelineIndex;

		vk::DescriptorSet material;

		vk::Buffer vertexBuffer;
		vk::Buffer indexBuffer;
		uint indexCount;

		glm::mat4 transformMatrix;
	};

	struct DrawObjectsThreadInfo {
		ObjectRenderer* renderer;

		vk::PipelineLayout vkPipelineLayout;
		const vk::Pipeline* pVkPipelines;
		const vk::CommandBuffer* pSecondaryCommandBuffers;

		uint materialDescriptorSetId;

		const Frustum* includeFrustums;
		uint includeFrustumCount;

		const Frustum* excludeFrustums;
		uint excludeFrustumCount;

		uint fragmentIndex;
		uint fragmentCount;

		uint materialDescriptorSetIndex;
	};


private:
	uint threadCount {};


	std::vector<std::vector<RenderInfo>> renderInfoCachePerThread {};
	std::vector<std::map<uint64_t, uint>> renderInfoIndicesPerThread {};

	ThreadPool threadPool {};
	std::vector<DrawObjectsThreadInfo> drawObjectsThreadInfos {};


public:
	int init();


	void setThreadCount(uint count) {
		threadCount = count;
	}


	void drawObjects(vk::PipelineLayout pipelineLayout, const vk::Pipeline* pPipelines,
					 const vk::CommandBuffer* pSecondaryCommandBuffers, const uint materialDescriptorSetId,
					 const Frustum* includeFrustums, uint includeFrustumCount, const Frustum* excludeFrustums = nullptr,
					 uint excludeFrustumCount = 0);


private:
	static void drawObjectsThreadFunc(uint threadIndex, void* pData);
};
} // namespace Engine